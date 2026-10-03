"""Independent OS readback of the parent's directly held producer child."""
import json, os, sys
pid=int(sys.argv[1])
if sys.platform=="win32":
    from tui28_windows_identity import process_birth
    value=process_birth(pid)
elif sys.platform=="linux":
    with open("/proc/%d/stat"%pid,encoding="utf-8") as stream: fields=stream.read().rsplit(") ",1)[1].split()
    value={"platform":"linux","parentPID":int(fields[1]),"startTimeTicks":fields[19],"image":os.readlink("/proc/%d/exe"%pid)}
else: raise RuntimeError("scoped observation host denied")
print(json.dumps(value,separators=(",",":")))
