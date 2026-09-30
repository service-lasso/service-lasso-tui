import argparse
import json
import os
import select
import sys
import time

from winpty.enums import Backend
from winpty.ptyprocess import PtyProcess


def child_environment(api_url):
    allowed = ("APPDATA", "COMSPEC", "LOCALAPPDATA", "PATHEXT", "PATH", "SYSTEMROOT", "TEMP", "TMP", "USERPROFILE", "WINDIR")
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


def fail(stage):
    emit({"ok": False, "stage": stage})
    return 1


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
        emit({"ok": True, "mode": mode, "navigation": "help" if mode == "connected" else "not_applicable", "exit": "q"})
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
    parser.add_argument("--executable", required=True)
    parser.add_argument("--mode", choices=("unavailable", "connected"), required=True)
    parser.add_argument("--api-url")
    args = parser.parse_args()
    if args.mode == "connected" and not args.api_url:
        return fail("setup")
    return probe(args.executable, args.mode, args.api_url)


if __name__ == "__main__":
    sys.exit(main())
