import argparse
import json
import os
import select
import secrets
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
def receipt(stage, outcome, closed_reason, startup_boundary=None):
    result = {"stage": stage, "outcome": outcome, "closedReason": closed_reason}
    if startup_boundary is not None:
        if stage != "startup" or outcome != "error" or closed_reason not in TERMINAL_CLOSE_REASONS or startup_boundary not in STARTUP_BOUNDARIES:
            raise ValueError("invalid startup boundary")
        result["startupBoundary"] = startup_boundary
    return result
def fail(stage, reason=None, outcome_receipt=None):
    # The harness deliberately exposes no terminal text, path, or exception.
    # Every failure has the same closed metadata-only schema.
    safe_stage = stage if stage in RECEIPT_STAGES else "launch"
    result = {"ok": False, "stage": safe_stage, "receipt": outcome_receipt or receipt(safe_stage, "error", "stage_failed")}
    emit(result)
    return 1

class ShutdownRequested(Exception): pass


class TerminalClosed(Exception):
    def __init__(self, reason="terminal_unknown", startup_boundary="unclassified"):
        self.reason = reason if reason in RECEIPT_REASONS else "terminal_unknown"
        self.startup_boundary = startup_boundary if startup_boundary in STARTUP_BOUNDARIES else "unclassified"
        super().__init__(self.reason)

RECEIPT_NAME = "helper-outcome.json"
RECEIPT_STAGES = {"launch", "startup", "wait-reconnect", "reconnect", "navigation", "resize-observation", "exit"}
RECEIPT_OUTCOMES = {"normal", "error", "timeout"}
RECEIPT_REASONS = {"completed", "stage_failed", "timed_out", "shutdown_requested", "terminal_exited_zero", "terminal_exit_code_1", "terminal_exit_code_2", "terminal_exited_nonzero", "terminal_signaled", "terminal_unknown"}
TERMINAL_CLOSE_REASONS = {"terminal_exited_zero", "terminal_exit_code_1", "terminal_exit_code_2", "terminal_exited_nonzero", "terminal_signaled", "terminal_unknown"}
STARTUP_BOUNDARIES = {"unclassified", "api_url_invalid", "api_url_scheme", "api_url_userinfo", "api_url_query_or_fragment", "api_token_transport", "api_client_error", "program_run_error", "program_run_killed", "program_run_panic", "program_run_interrupted"}
STARTUP_MARKER_PREFIX = "\x1eSERVICE_LASSO_TUI_STARTUP_BOUNDARY:"
STARTUP_MARKER_SUFFIX = "\x1f"


def terminal_close_reason(process):
    # EOF is a stream observation, not a process-exit observation. Query only
    # the owned PTY's bounded lifecycle fields; never retain terminal output.
    try:
        if process.isalive():
            return None
        signal_status = getattr(process, "signalstatus", None)
        exit_status = getattr(process, "exitstatus", None)
    except Exception:
        return "terminal_unknown"
    if isinstance(signal_status, int) and not isinstance(signal_status, bool):
        if signal_status != 0:
            return "terminal_signaled"
    elif signal_status is not None:
        return "terminal_unknown"
    if isinstance(exit_status, int) and not isinstance(exit_status, bool):
        if exit_status == 0:
            return "terminal_exited_zero"
        if exit_status == 1:
            return "terminal_exit_code_1"
        if exit_status == 2:
            return "terminal_exit_code_2"
        return "terminal_exited_nonzero"
    return "terminal_unknown"

def startup_boundary_from_terminal_text(text, nonce):
    # The marker is emitted only by the TUI's explicit probe mode. Its fresh
    # nonce binds it to this owned attempt; no terminal text is retained or
    # emitted by this matcher.
    if not isinstance(text, str) or not isinstance(nonce, str):
        return "unclassified"
    for boundary in STARTUP_BOUNDARIES - {"unclassified"}:
        if STARTUP_MARKER_PREFIX + nonce + ":" + boundary + STARTUP_MARKER_SUFFIX in text:
            return boundary
    return "unclassified"

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

def wait_for(process, expected, timeout, request_file=None, acknowledgement_file=None, token=None, startup_probe_nonce=None, clock=time.monotonic, select_fn=select.select, sleeper=time.sleep):
    deadline, text = clock() + timeout, ""
    while clock() < deadline:
        check_shutdown(process, request_file, acknowledgement_file, token)
        if all(value in text for value in expected): return text
        readable, _, _ = select_fn([process], [], [], min(0.1, max(0, deadline - clock())))
        if readable:
            try: text += process.read()
            except EOFError:
                closed_reason = terminal_close_reason(process)
                if closed_reason is not None:
                    raise TerminalClosed(closed_reason, startup_boundary_from_terminal_text(text, startup_probe_nonce))
                # pywinpty can surface a transient EOF while the owned process
                # is live. Preserve the existing deadline and sleep briefly so
                # a readable EOF cannot turn this bounded wait into a spin.
                sleeper(min(0.05, max(0, deadline - clock())))
    return text if all(value in text for value in expected) else None

def wait_for_file(file_name, timeout, process=None, request_file=None, acknowledgement_file=None, token=None, clock=time.monotonic, sleeper=time.sleep):
    deadline = clock() + timeout
    while clock() < deadline:
        if process is not None: check_shutdown(process, request_file, acknowledgement_file, token)
        if os.path.exists(file_name): return True
        sleeper(0.05)
    return False

def probe(executable, mode, api_url, ready_file, reconnect_file, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token, pty_process=PtyProcess, wait=wait_for, wait_file=wait_for_file, backend=CONPTY_BACKEND, startup_probe_nonce=None):
    process, stage = None, "launch"
    try:
        if pty_process is None: return fail(stage)
        startup_probe_nonce = startup_probe_nonce or secrets.token_hex(32)
        if len(startup_probe_nonce) != 64 or any(character not in "0123456789abcdef" for character in startup_probe_nonce):
            return fail(stage)
        # pywinpty resolves the command through the constrained child PATH.
        # Anchor the CI-built candidate before that constrained environment is
        # supplied, rather than depending on the caller's working directory.
        executable = os.path.abspath(executable)
        environment = {key: os.environ[key] for key in ("APPDATA", "COMSPEC", "LOCALAPPDATA", "PATHEXT", "PATH", "SYSTEMROOT", "TEMP", "TMP", "USERPROFILE", "WINDIR") if os.environ.get(key)}
        environment.update({"TERM": "xterm-256color", "SERVICE_LASSO_API_URL": api_url, "SERVICE_LASSO_STARTUP_PROBE_NONCE": startup_probe_nonce})
        process = pty_process.spawn([executable], cwd=os.path.dirname(executable), env=environment, dimensions=(40, 120), backend=backend)
        expected = "Runtime API unavailable"
        stage = "startup"
        if not wait(process, ("Service Lasso TUI", "q quit", expected), 20, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token, startup_probe_nonce):
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
        if not wait(process, ("Runtime identity:", "Services needing attention"), 20, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token, startup_probe_nonce):
            return fail(stage, "connected-screen-not-observed", receipt(stage, "timeout", "timed_out"))
        stage = "navigation"; process.write("d?")
        if not wait(process, ("n narrow", "r reconnect"), 5, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token, startup_probe_nonce):
            return fail(stage, outcome_receipt=receipt(stage, "timeout", "timed_out"))
        # Capture a screen redraw after the actual ConPTY resize. The expected
        # contextual-help screen proves the candidate kept its current view.
        stage = "resize-observation"; process.setwinsize(24, 50)
        if not wait(process, ("Service Lasso TUI", "n narrow", "r reconnect"), 5, shutdown_request_file, shutdown_acknowledgement_file, shutdown_token, startup_probe_nonce):
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
    except TerminalClosed as closed:
        receipt_stage = stage if stage in RECEIPT_STAGES else "launch"
        return fail(receipt_stage, outcome_receipt=receipt(receipt_stage, "error", closed.reason, closed.startup_boundary if receipt_stage == "startup" else None))
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
