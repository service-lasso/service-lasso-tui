import argparse
import ctypes
import json
import os
import platform
import select
import sys
import time

from winpty.enums import Backend
from winpty.ptyprocess import PtyProcess


def child_environment(api_url):
    allowed = ("APPDATA", "COMSPEC", "LOCALAPPDATA", "PATHEXT", "PATH", "SERVICE_LASSO_API_TOKEN", "SERVICE_LASSO_CONNECTIONS_CONFIG", "SYSTEMROOT", "TEMP", "TMP", "USERPROFILE", "WINDIR")
    environment = {key: os.environ[key] for key in allowed if os.environ.get(key)}
    environment["TERM"] = "xterm-256color"
    if api_url:
        environment["SERVICE_LASSO_API_URL"] = api_url
    return environment


def wait_for(process, expected, timeout_seconds):
    deadline = time.monotonic() + timeout_seconds
    text = ""
    while time.monotonic() < deadline:
        if all(value in text for value in expected):
            return True
        readable, _, _ = select.select([process], [], [], min(0.1, max(0, deadline - time.monotonic())))
        if readable:
            try:
                text += process.read()
            except EOFError:
                break
    return all(value in text for value in expected)


def emit(result):
    print(json.dumps(result, separators=(",", ":")))


def fail(stage, closed_reason=None):
    result = {"ok": False, "stage": stage}
    if closed_reason is not None:
        result["receipt"] = {"stage": "exit", "outcome": "error", "closedReason": closed_reason}
    emit(result)
    return 1

def observed_q_exit(process):
    try:
        waited = process.wait()
        signal_status = getattr(process, "signalstatus", None)
        exit_status = getattr(process, "exitstatus", None)
    except Exception:
        return "terminal_unknown"
    if isinstance(signal_status, int) and not isinstance(signal_status, bool):
        if signal_status != 0: return "terminal_signaled"
    elif signal_status is not None: return "terminal_unknown"
    if not isinstance(exit_status, int) or isinstance(exit_status, bool): return "terminal_unknown"
    if exit_status == 0:
        return "terminal_exited_zero" if isinstance(waited, int) and not isinstance(waited, bool) and waited == 0 else "terminal_unknown"
    return "terminal_exit_code_1" if exit_status == 1 else "terminal_exit_code_2" if exit_status == 2 else "terminal_exited_nonzero"


def architecture():
    return {"machine": platform.machine(), "pointerBits": ctypes.sizeof(ctypes.c_void_p) * 8}


def is_amd64_helper():
    details = architecture()
    machine = details["machine"].replace("_", "").replace("-", "").lower()
    return machine in ("amd64", "x8664") and details["pointerBits"] == 64


def probe(executable, mode, api_url):
    process = None
    stage = "launch"
    try:
        process = PtyProcess.spawn([executable], cwd=os.path.dirname(executable), env=child_environment(api_url), dimensions=(40, 120), backend=Backend.ConPTY)
        stage = "startup"
        expected = "Runtime identity:" if mode == "connected" else "Runtime API unavailable"
        if not wait_for(process, ("Service Lasso TUI", "q quit", expected), 20):
            return fail(stage)
        if mode == "connected":
            stage = "navigation"
            process.write("d?")
            if not wait_for(process, ("d dashboard", "esc back"), 5):
                return fail(stage)
        stage = "exit"
        process.write("q")
        deadline = time.monotonic() + 5
        while process.isalive() and time.monotonic() < deadline:
            try:
                readable, _, _ = select.select([process], [], [], 0.1)
                if readable:
                    process.read()
            except EOFError:
                break
        if process.isalive():
            return fail(stage)
        closed_reason = observed_q_exit(process)
        if closed_reason != "terminal_exited_zero": return fail(stage, closed_reason)
        emit({"ok": True, "mode": mode, "navigation": "help" if mode == "connected" else "not_applicable", "exit": "q", "receipt": {"stage": "exit", "outcome": "normal", "closedReason": "completed"}})
        return 0
    except Exception:
        return fail(stage)
    finally:
        if process is not None:
            try:
                process.close(force=True)
            except Exception:
                pass


def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--executable")
    parser.add_argument("--mode", choices=("unavailable", "connected"))
    parser.add_argument("--api-url")
    parser.add_argument("--architecture", action="store_true")
    args = parser.parse_args()
    if args.architecture:
        emit({"ok": is_amd64_helper(), "architecture": architecture()})
        return 0 if is_amd64_helper() else 1
    if not args.executable or not args.mode:
        return fail("setup")
    if args.mode == "connected" and not args.api_url:
        return fail("setup")
    return probe(args.executable, args.mode, args.api_url)


if __name__ == "__main__":
    sys.exit(main())
