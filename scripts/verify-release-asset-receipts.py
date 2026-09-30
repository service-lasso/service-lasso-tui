"""Windows-only rooted receipt writer for the release-asset ConPTY harness.

The process owns the temporary attempt directory and receipt file handles for
the whole probe.  NT paths below the held parent are opened relative to that
parent with OBJ_DONT_REPARSE and FILE_OPEN_REPARSE_POINT, so a pathname swap
cannot redirect receipt creation or publication.
"""
import argparse
import ctypes
from ctypes import wintypes
import json
import os
import secrets
import sys

if os.name != "nt":
    raise SystemExit("Windows receipt writer only")

ntdll = ctypes.WinDLL("ntdll")
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

OBJ_CASE_INSENSITIVE = 0x40
OBJ_DONT_REPARSE = 0x1000
FILE_LIST_DIRECTORY = 0x0001
FILE_WRITE_DATA = 0x0002
SYNCHRONIZE = 0x00100000
FILE_SHARE_READ = 0x00000001
FILE_SHARE_WRITE = 0x00000002
FILE_CREATE = 2
FILE_DIRECTORY_FILE = 0x00000001
FILE_SYNCHRONOUS_IO_NONALERT = 0x00000020
FILE_OPEN_REPARSE_POINT = 0x00200000
FILE_ATTRIBUTE_NORMAL = 0x80
STATUS_OBJECT_NAME_COLLISION = 0xC0000035
INVALID_HANDLE_VALUE = wintypes.HANDLE(-1).value

class UNICODE_STRING(ctypes.Structure):
    _fields_ = [("Length", wintypes.USHORT), ("MaximumLength", wintypes.USHORT), ("Buffer", wintypes.LPWSTR)]
class OBJECT_ATTRIBUTES(ctypes.Structure):
    _fields_ = [("Length", wintypes.ULONG), ("RootDirectory", wintypes.HANDLE), ("ObjectName", ctypes.POINTER(UNICODE_STRING)), ("Attributes", wintypes.ULONG), ("SecurityDescriptor", wintypes.LPVOID), ("SecurityQualityOfService", wintypes.LPVOID)]
class IO_STATUS_BLOCK(ctypes.Structure):
    _fields_ = [("Status", wintypes.LONG), ("Information", ctypes.c_size_t)]

ntdll.NtCreateFile.argtypes = [ctypes.POINTER(wintypes.HANDLE), wintypes.ULONG, ctypes.POINTER(OBJECT_ATTRIBUTES), ctypes.POINTER(IO_STATUS_BLOCK), wintypes.LPVOID, wintypes.ULONG, wintypes.ULONG, wintypes.ULONG, wintypes.ULONG, wintypes.LPVOID, wintypes.ULONG]
ntdll.NtCreateFile.restype = wintypes.LONG
kernel32.WriteFile.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
kernel32.SetFilePointerEx.argtypes = [wintypes.HANDLE, ctypes.c_longlong, ctypes.POINTER(ctypes.c_longlong), wintypes.DWORD]

def nt_open(name, root, access, disposition, options):
    value = ctypes.create_unicode_buffer(name)
    string = UNICODE_STRING(len(name) * 2, (len(name) + 1) * 2, ctypes.cast(value, wintypes.LPWSTR))
    attrs = OBJECT_ATTRIBUTES(ctypes.sizeof(OBJECT_ATTRIBUTES), root, ctypes.pointer(string), OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, None, None)
    handle, iosb = wintypes.HANDLE(), IO_STATUS_BLOCK()
    status = ntdll.NtCreateFile(ctypes.byref(handle), access, ctypes.byref(attrs), ctypes.byref(iosb), None, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, disposition, options | FILE_OPEN_REPARSE_POINT, None, 0)
    if status < 0: raise OSError(status, "NtCreateFile rejected receipt path")
    return handle

def close(handle):
    if handle and handle.value not in (None, INVALID_HANDLE_VALUE): kernel32.CloseHandle(handle)

def secure_root(base_root):
    # The base path is opened once with reparse rejection. Every created child
    # then uses its live handle, never a reconstructed path.
    # NtCreateFile accepts the native DOS namespace form when no RootDirectory
    # is supplied. Descendants below this first held directory never use a DOS
    # pathname again.
    base = nt_open("\\??\\" + os.path.abspath(base_root), None, FILE_LIST_DIRECTORY | SYNCHRONIZE, 1, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)
    try:
        for _ in range(32):
            leaf = "service-lasso-tui-release-asset-" + secrets.token_hex(16)
            try: return leaf, nt_open(leaf, base, FILE_LIST_DIRECTORY | SYNCHRONIZE, FILE_CREATE, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)
            except OSError as error:
                if error.errno != STATUS_OBJECT_NAME_COLLISION: raise
        raise RuntimeError("receipt root collision limit")
    finally: close(base)

def valid(receipt):
    return isinstance(receipt, dict) and set(receipt) == {"stage", "outcome", "closedReason"} and receipt["stage"] in {"launch", "startup", "wait-reconnect", "reconnect", "navigation", "resize-observation", "exit", "helper-exit"} and receipt["outcome"] in {"normal", "error", "timeout"} and receipt["closedReason"] in {"completed", "stage_failed", "timed_out", "shutdown_requested", "helper_exit_nonzero", "helper_exit_signal", "helper_exit_spawn_error"}

def write(handle, receipt):
    data = json.dumps(receipt, separators=(",", ":")).encode("utf-8")
    pos = ctypes.c_longlong()
    if not kernel32.SetFilePointerEx(handle, 0, ctypes.byref(pos), 0): raise OSError(ctypes.get_last_error(), "SetFilePointerEx")
    if not kernel32.SetEndOfFile(handle): raise OSError(ctypes.get_last_error(), "SetEndOfFile")
    written = wintypes.DWORD()
    if not kernel32.WriteFile(handle, data, len(data), ctypes.byref(written), None) or written.value != len(data): raise OSError(ctypes.get_last_error(), "WriteFile")
    if not kernel32.FlushFileBuffers(handle): raise OSError(ctypes.get_last_error(), "FlushFileBuffers")

def emit(value):
    print(json.dumps(value, separators=(",", ":")), flush=True)

def main():
    parser = argparse.ArgumentParser(add_help=False); parser.add_argument("--base-root", required=True); args = parser.parse_args()
    leaf, root = secure_root(args.base_root)
    helper = node = None
    try:
        helper = nt_open("helper-outcome.json", root, FILE_WRITE_DATA | SYNCHRONIZE, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT)
        node = nt_open("node-exit-outcome.json", root, FILE_WRITE_DATA | SYNCHRONIZE, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT)
        emit({"ok": True, "event": "ready", "root": os.path.join(os.path.abspath(args.base_root), leaf)})
        for line in sys.stdin:
            try: request = json.loads(line)
            except Exception: emit({"ok": False, "event": "invalid"}); continue
            if request == {"op": "close"}: emit({"ok": True, "event": "closed"}); return
            if not isinstance(request, dict):
                emit({"ok": False, "event": "invalid"}); continue
            sink, receipt = request.get("sink"), request.get("receipt")
            if request.get("op") != "write" or sink not in {"helper", "node"} or not valid(receipt): emit({"ok": False, "event": "invalid"}); continue
            write(helper if sink == "helper" else node, receipt); emit({"ok": True, "event": "written", "sink": sink})
    finally:
        close(helper); close(node); close(root)

if __name__ == "__main__": main()
