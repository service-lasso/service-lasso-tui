import argparse
import json
import os
import select
import sys
import time

from winpty.enums import Backend
from winpty.ptyprocess import PtyProcess


def emit(result): print(json.dumps(result, separators=(",", ":")))
def fail(stage): emit({"ok": False, "stage": stage}); return 1

def wait_for(process, expected, timeout):
    deadline, text = time.monotonic() + timeout, ""
    while time.monotonic() < deadline:
        if all(value in text for value in expected): return True
        readable, _, _ = select.select([process], [], [], min(0.1, max(0, deadline - time.monotonic())))
        if readable:
            try: text += process.read()
            except EOFError: break
    return all(value in text for value in expected)

def probe(executable, mode, api_url):
    process, stage = None, "launch"
    try:
        environment = {key: os.environ[key] for key in ("APPDATA", "COMSPEC", "LOCALAPPDATA", "PATHEXT", "PATH", "SYSTEMROOT", "TEMP", "TMP", "USERPROFILE", "WINDIR") if os.environ.get(key)}
        environment.update({"TERM": "xterm-256color", "SERVICE_LASSO_API_URL": api_url})
        process = PtyProcess.spawn([executable], cwd=os.path.dirname(executable), env=environment, dimensions=(40, 120), backend=Backend.ConPTY)
        expected = "Runtime identity:" if mode == "connected" else "Runtime API unavailable"
        stage = "startup"
        if not wait_for(process, ("Service Lasso TUI", "q quit", expected), 20): return fail(stage)
        narrow_resize = False
        if mode == "connected":
            stage = "navigation"; process.write("d?")
            if not wait_for(process, ("n narrow",), 5): return fail(stage)
            # A real ConPTY resize to 50 columns is the narrow-layout input.
            # Bubble Tea receives this as a WindowSizeMsg; the TUI's model
            # tests cover its visible narrow-state transition without keeping
            # terminal screen bytes in this release evidence.
            stage = "resize-request"; process.setwinsize(24, 50)
            narrow_resize = True
        stage = "exit"; process.write("q")
        deadline = time.monotonic() + 5
        while process.isalive() and time.monotonic() < deadline:
            try:
                readable, _, _ = select.select([process], [], [], 0.1)
                if readable: process.read()
            except EOFError: break
        if process.isalive(): return fail(stage)
        emit({"ok": True, "mode": mode, "navigation": ["d", "?"] if mode == "connected" else [], "narrowResize": narrow_resize, "exit": "q"}); return 0
    except Exception: return fail(stage)
    finally:
        if process is not None:
            try: process.close(force=True)
            except Exception: pass

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument("--executable"); parser.add_argument("--mode", choices=("unavailable", "connected")); parser.add_argument("--api-url")
args = parser.parse_args()
sys.exit(probe(args.executable, args.mode, args.api_url) if args.executable and args.mode and args.api_url else fail("setup"))
