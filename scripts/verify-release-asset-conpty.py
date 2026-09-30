import argparse
import json
import os
import select
import sys
import time

from winpty.enums import Backend
from winpty.ptyprocess import PtyProcess


def emit(result): print(json.dumps(result, separators=(",", ":")))
def fail(stage, reason=None):
    result = {"ok": False, "stage": stage}
    if reason:
        result["reason"] = reason
    emit(result)
    return 1

class ShutdownRequested(Exception): pass

def acknowledge_shutdown(process, acknowledgement_file):
    try:
        process.close(force=True)
        if process.isalive(): return False
        with open(acknowledgement_file, "x", encoding="utf-8") as marker: marker.write("closed\n")
        return True
    except Exception:
        return False

def check_shutdown(process, request_file, acknowledgement_file):
    if request_file and os.path.exists(request_file):
        if acknowledge_shutdown(process, acknowledgement_file): raise ShutdownRequested()
        raise RuntimeError("cleanup-unconfirmed")

def wait_for(process, expected, timeout, request_file=None, acknowledgement_file=None):
    deadline, text = time.monotonic() + timeout, ""
    while time.monotonic() < deadline:
        check_shutdown(process, request_file, acknowledgement_file)
        if all(value in text for value in expected): return text
        readable, _, _ = select.select([process], [], [], min(0.1, max(0, deadline - time.monotonic())))
        if readable:
            try: text += process.read()
            except EOFError: break
    return text if all(value in text for value in expected) else None

def wait_for_file(file_name, timeout, process=None, request_file=None, acknowledgement_file=None):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process is not None: check_shutdown(process, request_file, acknowledgement_file)
        if os.path.exists(file_name): return True
        time.sleep(0.05)
    return False

def probe(executable, mode, api_url, ready_file, reconnect_file, shutdown_request_file, shutdown_acknowledgement_file):
    process, stage = None, "launch"
    try:
        environment = {key: os.environ[key] for key in ("APPDATA", "COMSPEC", "LOCALAPPDATA", "PATHEXT", "PATH", "SYSTEMROOT", "TEMP", "TMP", "USERPROFILE", "WINDIR") if os.environ.get(key)}
        environment.update({"TERM": "xterm-256color", "SERVICE_LASSO_API_URL": api_url})
        process = PtyProcess.spawn([executable], cwd=os.path.dirname(executable), env=environment, dimensions=(40, 120), backend=Backend.ConPTY)
        expected = "Runtime API unavailable"
        stage = "startup"
        if not wait_for(process, ("Service Lasso TUI", "q quit", expected), 20, shutdown_request_file, shutdown_acknowledgement_file): return fail(stage)
        if mode == "unavailable":
            stage = "exit"; process.write("q")
            deadline = time.monotonic() + 5
            while process.isalive() and time.monotonic() < deadline:
                try:
                    readable, _, _ = select.select([process], [], [], 0.1)
                    if readable: process.read()
                except EOFError: break
            if process.isalive(): return fail(stage)
            emit({"ok": True, "mode": mode, "exit": "q"}); return 0
        if not ready_file or not reconnect_file: return fail("setup")
        with open(ready_file, "x", encoding="utf-8") as marker: marker.write("unavailable-rendered\n")
        stage = "wait-reconnect"
        if not wait_for_file(reconnect_file, 20, process, shutdown_request_file, shutdown_acknowledgement_file): return fail(stage)
        # Keep this exact extracted process alive across the unavailable-to-ready
        # transition, then exercise the documented reconnect key in that process.
        stage = "reconnect"
        try:
            check_shutdown(process, shutdown_request_file, shutdown_acknowledgement_file)
            process.write("r")
        except Exception: return fail(stage, "pty-write-failed")
        if not wait_for(process, ("Runtime identity:", "Services needing attention"), 20, shutdown_request_file, shutdown_acknowledgement_file): return fail(stage, "connected-screen-not-observed")
        stage = "navigation"; process.write("d?")
        if not wait_for(process, ("n narrow", "r reconnect"), 5, shutdown_request_file, shutdown_acknowledgement_file): return fail(stage)
        # Capture a screen redraw after the actual ConPTY resize. The expected
        # contextual-help screen proves the candidate kept its current view.
        stage = "resize-observation"; process.setwinsize(24, 50)
        if not wait_for(process, ("Service Lasso TUI", "n narrow", "r reconnect"), 5, shutdown_request_file, shutdown_acknowledgement_file): return fail(stage)
        narrow_resize = "help-screen-rendered-after-50-columns"
        stage = "exit"; process.write("q")
        deadline = time.monotonic() + 5
        while process.isalive() and time.monotonic() < deadline:
            try:
                readable, _, _ = select.select([process], [], [], 0.1)
                if readable: process.read()
            except EOFError: break
        if process.isalive(): return fail(stage)
        emit({"ok": True, "mode": mode, "reconnect": "r", "navigation": ["d", "?"], "narrowResize": narrow_resize, "exit": "q"}); return 0
    except ShutdownRequested: return fail("cleanup", "shutdown-requested")
    except Exception: return fail(stage, "pty-operation-failed" if stage == "reconnect" else None)
    finally:
        if process is not None:
            try: process.close(force=True)
            except Exception: pass

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument("--executable"); parser.add_argument("--mode", choices=("unavailable", "reconnect")); parser.add_argument("--api-url"); parser.add_argument("--ready-file"); parser.add_argument("--reconnect-file"); parser.add_argument("--shutdown-request-file"); parser.add_argument("--shutdown-acknowledgement-file")
args = parser.parse_args()
sys.exit(probe(args.executable, args.mode, args.api_url, args.ready_file, args.reconnect_file, args.shutdown_request_file, args.shutdown_acknowledgement_file) if args.executable and args.mode and args.api_url else fail("setup"))
