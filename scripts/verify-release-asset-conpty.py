import argparse
import json
import os
import select
import stat
import sys
import time

try:
    from winpty.enums import Backend
    from winpty.ptyprocess import PtyProcess
except ImportError:
    # Keep bounded receipt tests portable; a real probe still fails closed when
    # the pinned Windows-only dependency is unavailable.
    Backend = None
    PtyProcess = None

CONPTY_BACKEND = Backend.ConPTY if Backend is not None else None


def emit(result): print(json.dumps(result, separators=(",", ":")))
def receipt(stage, outcome, closed_reason):
    return {"stage": stage, "outcome": outcome, "closedReason": closed_reason}
def fail(stage, reason=None, outcome_receipt=None):
    # The harness deliberately exposes no terminal text, path, or exception.
    # Every failure has the same closed metadata-only schema.
    safe_stage = stage if stage in RECEIPT_STAGES else "launch"
    result = {"ok": False, "stage": safe_stage, "receipt": outcome_receipt or receipt(safe_stage, "error", "stage_failed")}
    emit(result)
    return 1

class ShutdownRequested(Exception): pass
class TerminalClosed(Exception): pass

RECEIPT_NAME = "helper-outcome.json"
RECEIPT_STAGES = {"launch", "startup", "wait-reconnect", "reconnect", "navigation", "resize-observation", "exit"}
RECEIPT_OUTCOMES = {"normal", "error", "timeout"}
RECEIPT_REASONS = {"completed", "stage_failed", "timed_out", "terminal_closed", "shutdown_requested"}

def acknowledge_shutdown(process, acknowledgement_file, token):
    try:
        process.close(force=True)
        if process.isalive(): return False
        with open(acknowledgement_file, "x", encoding="utf-8") as marker: marker.write(token + "\n")
        return True
    except Exception:
        return False

def check_shutdown(process, request_file, acknowledgement_file, token):
    if request_file and os.path.exists(request_file):
        try:
            with open(request_file, "r", encoding="utf-8") as request: requested = request.read()
        except Exception:
            requested = ""
        if requested == token + "\n" and acknowledge_shutdown(process, acknowledgement_file, token): raise ShutdownRequested()
        raise RuntimeError("cleanup-unconfirmed")

def wait_for(process, expected, timeout, request_file=None, acknowledgement_file=None, token=None, clock=time.monotonic, select_fn=select.select):
    deadline, text = clock() + timeout, ""
    while clock() < deadline:
        check_shutdown(process, request_file, acknowledgement_file, token)
        if all(value in text for value in expected): return text
        readable, _, _ = select_fn([process], [], [], min(0.1, max(0, deadline - clock())))
        if readable:
            try: text += process.read()
            except EOFError: raise TerminalClosed()
    return text if all(value in text for value in expected) else None

def wait_for_file(file_name, timeout, process=None, request_file=None, acknowledgement_file=None, token=None, clock=time.monotonic, sleeper=time.sleep):
    deadline = clock() + timeout
    while clock() < deadline:
        if process is not None: check_shutdown(process, request_file, acknowledgement_file, token)
        if os.path.exists(file_name): return True
        sleeper(0.05)
    return False

def probe(executable, mode, api_url, ready_file, reconnect_file, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token, pty_process=PtyProcess, wait=wait_for, wait_file=wait_for_file, backend=CONPTY_BACKEND):
    process, stage = None, "launch"
    try:
        if pty_process is None: return fail(stage)
        # pywinpty resolves the command through the constrained child PATH.
        # Anchor the CI-built candidate before that constrained environment is
        # supplied, rather than depending on the caller's working directory.
        executable = os.path.abspath(executable)
        environment = {key: os.environ[key] for key in ("APPDATA", "COMSPEC", "LOCALAPPDATA", "PATHEXT", "PATH", "SYSTEMROOT", "TEMP", "TMP", "USERPROFILE", "WINDIR") if os.environ.get(key)}
        environment.update({"TERM": "xterm-256color", "SERVICE_LASSO_API_URL": api_url})
        process = pty_process.spawn([executable], cwd=os.path.dirname(executable), env=environment, dimensions=(40, 120), backend=backend)
        expected = "Runtime API unavailable"
        stage = "startup"
        if not wait(process, ("Service Lasso TUI", "q quit", expected), 20, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token):
            return fail(stage, outcome_receipt=receipt(stage, "timeout", "timed_out"))
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
        if not ready_file or not reconnect_file:
            return fail("setup", outcome_receipt=receipt("startup", "error", "stage_failed"))
        with open(ready_file, "x", encoding="utf-8") as marker: marker.write("unavailable-rendered\n")
        stage = "wait-reconnect"
        if not wait_file(reconnect_file, 20, process, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token):
            return fail(stage, outcome_receipt=receipt(stage, "timeout", "timed_out"))
        # Keep this exact extracted process alive across the unavailable-to-ready
        # transition, then exercise the documented reconnect key in that process.
        stage = "reconnect"
        try:
            check_shutdown(process, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token)
            process.write("r")
        except Exception:
            return fail(stage, "pty-write-failed", receipt(stage, "error", "stage_failed"))
        if not wait(process, ("Runtime identity:", "Services needing attention"), 20, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token):
            return fail(stage, "connected-screen-not-observed", receipt(stage, "timeout", "timed_out"))
        stage = "navigation"; process.write("d?")
        if not wait(process, ("n narrow", "r reconnect"), 5, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token):
            return fail(stage, outcome_receipt=receipt(stage, "timeout", "timed_out"))
        # Capture a screen redraw after the actual ConPTY resize. The expected
        # contextual-help screen proves the candidate kept its current view.
        stage = "resize-observation"; process.setwinsize(24, 50)
        if not wait(process, ("Service Lasso TUI", "n narrow", "r reconnect"), 5, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token):
            return fail(stage, outcome_receipt=receipt(stage, "timeout", "timed_out"))
        narrow_resize = "help-screen-rendered-after-50-columns"
        stage = "exit"; process.write("q")
        deadline = time.monotonic() + 5
        while process.isalive() and time.monotonic() < deadline:
            try:
                readable, _, _ = select.select([process], [], [], 0.1)
                if readable: process.read()
            except EOFError: break
        if process.isalive():
            return fail(stage, outcome_receipt=receipt(stage, "timeout", "timed_out"))
        emit({"ok": True, "mode": mode, "reconnect": "r", "navigation": ["d", "?"], "narrowResize": narrow_resize, "exit": "q", "receipt": receipt(stage, "normal", "completed")}); return 0
    except ShutdownRequested:
        receipt_stage = stage if stage in RECEIPT_STAGES else "exit"
        return fail(receipt_stage, "shutdown-requested", receipt(receipt_stage, "error", "shutdown_requested"))
    except TerminalClosed:
        receipt_stage = stage if stage in RECEIPT_STAGES else "launch"
        return fail(receipt_stage, outcome_receipt=receipt(receipt_stage, "error", "terminal_closed"))
    except Exception:
        receipt_stage = stage if stage in RECEIPT_STAGES else "launch"
        return fail(stage, "pty-operation-failed" if stage == "reconnect" else None, receipt(receipt_stage, "error", "stage_failed"))
    finally:
        if process is not None:
            try: process.close(force=True)
            except Exception: pass

def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--executable"); parser.add_argument("--mode", choices=("unavailable", "reconnect")); parser.add_argument("--api-url"); parser.add_argument("--ready-file"); parser.add_argument("--reconnect-file"); parser.add_argument("--shutdown-request-file"); parser.add_argument("--shutdown-acknowledgement-file"); parser.add_argument("--shutdown-token")
    args = parser.parse_args()
    return probe(args.executable, args.mode, args.api_url, args.ready_file, args.reconnect_file, args.shutdown_request_file, args.shutdown_acknowledgement_file, args.shutdown_token) if args.executable and args.mode and args.api_url else fail("setup")

if __name__ == "__main__":
    sys.exit(main())
