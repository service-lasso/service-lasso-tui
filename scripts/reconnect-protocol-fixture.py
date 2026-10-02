"""Source contract fixture: actual Python main/producer, controlled renderer.

This never claims compiled ConPTY/Core/native acceptance. The real identity
acquirer hashes actual temporary bytes; the direct constructor matcher consumes
the same framed stdout contract through a controlled subprocess result.
"""
import importlib.util
import hashlib
import os
import sys
import tempfile
from unittest.mock import patch

script = os.path.join(os.path.dirname(__file__), "verify-release-asset-conpty.py")
spec = importlib.util.spec_from_file_location("protocol_producer", script)
producer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(producer)

class Terminal:
    exitstatus = 0
    signalstatus = None
    def write(self, value): pass
    def setwinsize(self, rows, columns): pass
    def isalive(self): return False
    def wait(self): return 0
    def close(self, force=True): pass

def main():
    with tempfile.TemporaryDirectory() as root:
        executable = os.path.join(root, "candidate.exe")
        binary = b"real hashed protocol fixture bytes"
        with open(executable, "wb") as stream: stream.write(binary)
        identity = {"sourceCommit": "a" * 40, "binarySHA256": hashlib.sha256(binary).hexdigest()}
        original = producer.discriminate_constructor_then_probe
        acquisitions = []
        def acquire(*args):
            result = producer.acquire_candidate_identity(*args)
            acquisitions.append(result[1])
            return result
        class Pty:
            @staticmethod
            def spawn(*args, **kwargs):
                assert len(acquisitions) == 1 and not acquisitions[0].file.closed
                return Terminal()
        def direct(executable, held_identity):
            assert held_identity == identity and not acquisitions[0].file.closed
            def runner(args, **kwargs):
                nonce = kwargs["env"]["SERVICE_LASSO_STARTUP_PROBE_NONCE"]
                class Completed:
                    returncode = 2
                    stderr = (producer.STARTUP_MARKER_PREFIX + nonce + ":api_url_invalid:" + identity["sourceCommit"] + ":" + identity["binarySHA256"] + producer.STARTUP_MARKER_SUFFIX).encode()
                return Completed()
            return producer.direct_constructor_assertion(executable, held_identity, runner=runner)
        def actual_entry(*args, **kwargs):
            return original(*args, **kwargs, pty_process=Pty, wait=lambda *_a: "rendered", wait_file=lambda *_a: True, backend=None, identity_acquirer=acquire, direct_assertion=direct)
        argv = [script, "--executable", executable, "--mode", "reconnect", "--api-url", "http://127.0.0.1:1", "--source-commit", identity["sourceCommit"], "--expected-executable-sha256", identity["binarySHA256"], "--ready-file", os.path.join(root, "ready"), "--reconnect-file", os.path.join(root, "reconnect")]
        with patch.object(producer, "discriminate_constructor_then_probe", actual_entry), patch.object(sys, "argv", argv):
            result = producer.main()
        assert len(acquisitions) == 1 and acquisitions[0].file.closed
        return result

if __name__ == "__main__": sys.exit(main())
