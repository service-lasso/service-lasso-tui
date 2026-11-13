"""Native Windows process identity, retained handles and actual wait observations."""
import ctypes, os
from ctypes import wintypes

def process_birth(pid):
    if os.name != "nt": raise RuntimeError("Windows identity unavailable")
    kernel=ctypes.WinDLL("kernel32",use_last_error=True)
    kernel.OpenProcess.argtypes=[wintypes.DWORD,wintypes.BOOL,wintypes.DWORD]; kernel.OpenProcess.restype=wintypes.HANDLE
    kernel.CloseHandle.argtypes=[wintypes.HANDLE]
    kernel.GetProcessTimes.argtypes=[wintypes.HANDLE,*([ctypes.POINTER(wintypes.FILETIME)]*4)]
    kernel.QueryFullProcessImageNameW.argtypes=[wintypes.HANDLE,wintypes.DWORD,wintypes.LPWSTR,ctypes.POINTER(wintypes.DWORD)]
    kernel.GetExitCodeProcess.argtypes=[wintypes.HANDLE,ctypes.POINTER(wintypes.DWORD)]
    handle=kernel.OpenProcess(0x1400,False,pid)
    if not handle:
        if ctypes.get_last_error()==87: raise ProcessLookupError("process absent")
        raise OSError(ctypes.get_last_error(),"process identity unavailable")
    try:
        status=wintypes.DWORD()
        if not kernel.GetExitCodeProcess(handle,ctypes.byref(status)): raise OSError("exit identity unavailable")
        if status.value != 259: raise ProcessLookupError("process exited")
        creation,exit_time,kernel_time,user_time=(wintypes.FILETIME() for _ in range(4))
        if not kernel.GetProcessTimes(handle,*(ctypes.byref(value) for value in (creation,exit_time,kernel_time,user_time))): raise OSError("birth unavailable")
        image=ctypes.create_unicode_buffer(32768); length=wintypes.DWORD(len(image))
        if not kernel.QueryFullProcessImageNameW(handle,0,image,ctypes.byref(length)): raise OSError("image unavailable")
        class BasicInformation(ctypes.Structure):
            _fields_=[("exitStatus",ctypes.c_void_p),("peb",ctypes.c_void_p),("affinity",ctypes.c_void_p),("priority",ctypes.c_void_p),("pid",ctypes.c_void_p),("parent",ctypes.c_void_p)]
        info=BasicInformation(); returned=wintypes.ULONG()
        query=ctypes.WinDLL("ntdll").NtQueryInformationProcess
        query.argtypes=[wintypes.HANDLE,wintypes.ULONG,ctypes.c_void_p,wintypes.ULONG,ctypes.POINTER(wintypes.ULONG)]; query.restype=ctypes.c_long
        if query(handle,0,ctypes.byref(info),ctypes.sizeof(info),ctypes.byref(returned))!=0 or info.pid!=pid: raise OSError("parent observation unavailable")
        return {"platform":"win32","parentPID":info.parent,"creationFileTime":str((creation.dwHighDateTime<<32)|creation.dwLowDateTime),"image":image.value}
    finally: kernel.CloseHandle(handle)
