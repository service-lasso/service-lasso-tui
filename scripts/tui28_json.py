"""Duplicate-key and UTF-8 admission for the separate scoped native route."""
import json
LIMIT=1024*1024

def pairs(items):
    result={}
    for key,value in items:
        if key in result: raise ValueError("duplicate native JSON key")
        result[key]=value
    return result

def constant(value): raise ValueError("nonfinite native JSON number")

def strict_loads(value):
    if isinstance(value,bytes): value=value.decode("utf-8",errors="strict")
    if not isinstance(value,str) or value.startswith("\ufeff") or len(value.encode("utf-8"))>LIMIT: raise ValueError("native JSON quota or encoding")
    return json.loads(value,object_pairs_hook=pairs,parse_constant=constant)

def strict_load(stream): return strict_loads(stream.read(LIMIT+1))
