import argparse
import hashlib
import json
import os
import select
import time
import urllib.request

from winpty.enums import Backend
from winpty.ptyprocess import PtyProcess


def write_json(path, value):
    with open(path, "w", encoding="utf-8", newline="") as handle:
        json.dump(value, handle, separators=(",", ":"))


def append_terminal(path, label, chunk):
    with open(path, "a", encoding="utf-8", newline="") as handle:
        handle.write("\n--- " + label + " ---\n")
        handle.write(chunk)


def read_chunk(process, transcript_path, label):
    try:
        chunk = process.read()
    except EOFError:
        return False
    append_terminal(transcript_path, label, chunk)
    return chunk


def wait_for(process, expected, timeout, transcript, transcript_path, label, from_index=0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if all(item in transcript[0][from_index:] for item in expected):
            return
        readable, _, _ = select.select([process], [], [], 0.1)
        if readable:
            chunk = read_chunk(process, transcript_path, label)
            if not chunk:
                break
            transcript[0] += chunk
    raise RuntimeError("missing terminal state: " + ", ".join(expected))


def environment(tokens, connections):
    allowed = ("APPDATA", "COMSPEC", "LOCALAPPDATA", "PATHEXT", "PATH", "SYSTEMROOT", "TEMP", "TMP", "USERPROFILE", "WINDIR")
    env = {key: os.environ[key] for key in allowed if os.environ.get(key)}
    env.update({
        "TERM": "xterm-256color",
        "SERVICE_LASSO_API_TOKEN": tokens["token"],
        "SERVICE_LASSO_DENIED_TOKEN": tokens["deniedToken"],
        "SERVICE_LASSO_INVALID_TOKEN": "tui20-invalid-token",
        "SERVICE_LASSO_CONNECTIONS_CONFIG": connections,
    })
    return env


def open_terminal(executable, profile, env):
    return PtyProcess.spawn([executable, "--profile", profile], cwd=os.path.dirname(executable), env=env, dimensions=(44, 150), backend=Backend.ConPTY)


def detail(process, transcript, transcript_path, label, service_id):
    process.write("/")
    process.write(service_id)
    process.write("\r")
    wait_for(process, (service_id,), 20, transcript, transcript_path, label)
    process.write("\r")
    wait_for(process, ("Lifecycle:", service_id), 20, transcript, transcript_path, label)


def action(process, transcript, transcript_path, key, name):
    start = len(transcript[0])
    process.write(key)
    wait_for(process, ("Core preview: " + name, "Confirm " + name), 45, transcript, transcript_path, "allowed", start)
    frozen_before = transcript[0]
    process.write("rj?q")
    time.sleep(0.3)
    if "Confirm " + name not in transcript[0] or "Core preview: " + name not in transcript[0]:
        raise RuntimeError("confirmation changed by blocked input")
    process.write("y")
    process.write("r")
    wait_for(process, ("Core operation", "succeeded."), 90, transcript, transcript_path, "allowed", start)
    if transcript[0].count("Core operation") < frozen_before.count("Core operation") + 1:
        raise RuntimeError("operation result was not rendered")


def close_terminal(process):
    if not process.isalive():
        return
    process.write("q")
    deadline = time.monotonic() + 10
    while process.isalive() and time.monotonic() < deadline:
        time.sleep(0.1)
    if process.isalive():
        raise RuntimeError("terminal did not exit")


def operation_records(api_url, token):
    request = urllib.request.Request(api_url + "/api/operator/lifecycle/operations", headers={"Authorization": "Bearer " + token})
    with urllib.request.urlopen(request, timeout=15) as response:
        return json.load(response)["operations"]


def fixture_audit(records):
    return [{
        "operationId": record.get("operationId"),
        "action": record.get("action"),
        "targets": record.get("targets"),
        "status": record.get("status"),
        "outcome": record.get("outcome"),
        "cancellationSupported": record.get("cancellationSupported"),
    } for record in records if record.get("targets") == ["tui20-fixture"]]


def assert_five_records(records):
    audit = fixture_audit(records)
    expected = ["service_restart", "service_stop", "service_start", "service_configure", "service_install"]
    if len(audit) != 5 or sorted(record["action"] for record in audit) != sorted(expected):
        raise RuntimeError("expected exactly one durable record for each fixture action")
    if any(not record["operationId"] for record in audit):
        raise RuntimeError("operation audit omitted an operation ID")
    if any(record["cancellationSupported"] for record in audit):
        raise RuntimeError("fixture unexpectedly advertised cancellation")
    if any(record.get("targets") == ["tui20-unrelated"] for record in records):
        raise RuntimeError("unrelated fixture was targeted")
    return audit


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--executable", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--core-commit", required=True)
    args = parser.parse_args()
    transcript_path = os.path.join(args.root, "native-terminal.txt")
    receipt_path = os.path.join(args.root, "native-exit-receipt.json")
    with open(os.path.join(args.root, "private-token.json"), encoding="utf-8") as handle:
        tokens = json.load(handle)
    with open(os.path.join(args.root, "connections.json"), encoding="utf-8") as handle:
        api_url = json.load(handle)["profiles"]["native"]["url"]
    with open(os.path.join(args.root, "ready.json"), encoding="utf-8") as handle:
        ready = json.load(handle)
    if ready.get("coreCommit") != args.core_commit:
        raise RuntimeError("Core source identity did not match the requested commit")
    with open(args.executable, "rb") as handle:
        binary_sha256 = hashlib.sha256(handle.read()).hexdigest()
    env = environment(tokens, os.path.join(args.root, "connections.json"))
    outcome = {"outcome": "failed", "sourceCommit": args.source_commit, "coreCommit": args.core_commit, "binarySHA256": binary_sha256}
    terminals = []
    try:
        missing = open_terminal(args.executable, "missing", env); terminals.append(missing)
        missing_transcript = [""]
        wait_for(missing, ('connection profile "missing" credential is unavailable',), 30, missing_transcript, transcript_path, "missing-credential")
        close_terminal(missing); terminals.remove(missing)

        invalid = open_terminal(args.executable, "invalid", env); terminals.append(invalid)
        invalid_transcript = [""]
        wait_for(invalid, ("Runtime API unavailable:",), 30, invalid_transcript, transcript_path, "invalid-credential")
        close_terminal(invalid); terminals.remove(invalid)

        denied = open_terminal(args.executable, "denied", env); terminals.append(denied)
        denied_transcript = [""]
        wait_for(denied, ("Runtime identity:",), 30, denied_transcript, transcript_path, "denied")
        detail(denied, denied_transcript, transcript_path, "denied", "tui20-fixture")
        denied.write("i")
        wait_for(denied, ("Runtime API unavailable:",), 30, denied_transcript, transcript_path, "denied")
        close_terminal(denied); terminals.remove(denied)

        full = open_terminal(args.executable, "native", env); terminals.append(full)
        transcript = [""]
        wait_for(full, ("Runtime identity:",), 30, transcript, transcript_path, "allowed")
        detail(full, transcript, transcript_path, "allowed", "tui20-fixture")
        for key, name in (("i", "install"), ("c", "config"), ("s", "start"), ("x", "stop"), ("R", "restart")):
            action(full, transcript, transcript_path, key, name)
        records_before_reload = operation_records(api_url, tokens["token"])
        full.write("l")
        wait_for(full, ("reload is unavailable",), 15, transcript, transcript_path, "allowed")
        records_after_reload = operation_records(api_url, tokens["token"])
        if len(records_after_reload) != len(records_before_reload):
            raise RuntimeError("unavailable reload created an operation")
        if "z cancel" in transcript[0]:
            raise RuntimeError("cancellation was advertised despite Core readback")
        full.write("r")
        wait_for(full, ("Lifecycle:",), 20, transcript, transcript_path, "allowed")
        close_terminal(full); terminals.remove(full)

        reconnect = open_terminal(args.executable, "native", env); terminals.append(reconnect)
        reconnect_transcript = [""]
        wait_for(reconnect, ("Runtime identity:",), 30, reconnect_transcript, transcript_path, "reconnect")
        detail(reconnect, reconnect_transcript, transcript_path, "reconnect", "tui20-fixture")
        close_terminal(reconnect); terminals.remove(reconnect)

        records_after_reconnect = operation_records(api_url, tokens["token"])
        audit = assert_five_records(records_after_reconnect)
        if len(records_after_reconnect) != len(records_after_reload):
            raise RuntimeError("reconnect replayed an operation")
        write_json(os.path.join(args.root, "operation-audit.json"), {"operations": audit})
        outcome.update({"outcome": "succeeded", "actions": ["install", "config", "start", "stop", "restart"], "denial": "permission_denied", "missingCredential": True, "invalidCredential": True, "reloadDenied": True, "cancelAdvertised": False, "reconnectNoReplay": True, "fixtureOperationCount": len(audit), "unrelatedTargetPresent": False})
    except Exception as error:
        outcome["error"] = str(error)
        raise
    finally:
        for terminal in terminals:
            if terminal.isalive():
                terminal.close(force=True)
        write_json(receipt_path, outcome)


if __name__ == "__main__":
    main()
