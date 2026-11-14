import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

const observer=fileURLToPath(new URL('./tui20-native-owner-observer.py',import.meta.url));
const helper=fileURLToPath(new URL('./tui20-native-posix-five-action.py',import.meta.url));
const python=process.platform==='win32'?'python':'python3';
function check(source){
 const result=spawnSync(python,['-c',source,observer,helper],{encoding:'utf8',env:{...process.env,PYTHONDONTWRITEBYTECODE:'1'}});
 assert.equal(result.status,0,result.stderr);
 assert.equal(result.stderr,'');
 assert.equal(result.stdout.trim(),'private-stage-capture-pass');
}

test('observer forwards only closed literal stages before real child exit while retaining exact stderr and once-close',()=>check(String.raw`
import ast,importlib.util,io,subprocess,sys,threading
spec=importlib.util.spec_from_file_location('stage_observer',sys.argv[1]);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
helper=ast.parse(open(sys.argv[2],encoding='utf-8').read())
labels=[node.args[0].value for node in ast.walk(helper) if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='diagnostic_stage']
assert module.OWNER_STAGE_LINES==frozenset(('tui20-native-owner-stage '+label+'\n').encode('ascii') for label in labels)
marker=b'tui20-native-owner-stage allowed-action-enter\n'
payload=b'PRIVATE_SENTINEL\xff\n'+b'x'*10000+marker+b'not-'+marker+marker
class Forward:
 def __init__(self): self.lines=[];self.flushes=0;self.visible=threading.Event()
 def write(self,line): self.lines.append(line);self.visible.set();return len(line)
 def flush(self): self.flushes+=1
class Original:
 def __init__(self,stream): self.original=stream;self.buffer=stream.buffer;self.closes=0
 def close(self): self.closes+=1;self.original.close()
forward=Forward()
child=subprocess.Popen([sys.executable,'-c','import os,sys;os.write(2,'+repr(payload)+');sys.stdin.buffer.readline()'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
original=Original(child.stderr);child.stderr=original
capture=module.OwnerStderrCapture(original,forward);child.tui20_stderr_capture=capture
try:
 capture.start()
 assert forward.visible.wait(3),'marker unavailable while child retained'
 assert child.poll() is None,'marker arrived only after child exit'
 assert forward.lines==[marker.decode('ascii')]
finally:
 child.stdin.write('close\n');child.stdin.flush();child.stdin.close()
 assert child.wait(timeout=3)==0
 module.close_streams(child)
 module.close_streams(child)
assert b''.join(capture.raw)==payload
assert capture.eof and capture.closed and capture.finished
assert capture.originals==[] and original.closes==1
assert forward.lines==[marker.decode('ascii')] and forward.flushes==1
assert all('PRIVATE_SENTINEL' not in line for line in forward.lines)
print('private-stage-capture-pass')
`));

test('capture retains read, forwarding and once-close originals without printing private failures',()=>check(String.raw`
import importlib.util,io,sys
spec=importlib.util.spec_from_file_location('stage_observer',sys.argv[1]);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class PrivateFailure(OSError):
 def __str__(self): raise AssertionError('private original converted')
first=PrivateFailure();later=PrivateFailure();close_error=PrivateFailure()
class BadRead:
 def read1(self,count): raise first
class Original:
 def __init__(self,buffer,fail_close=False): self.buffer=buffer;self.closes=0;self.fail_close=fail_close
 def close(self):
  self.closes+=1
  if self.fail_close: raise close_error
class BadForward:
 def write(self,line): raise first
 def flush(self): raise later
read=Original(BadRead(),True);capture=module.OwnerStderrCapture(read,io.StringIO());capture.start();capture.finish();capture.finish()
assert capture.originals[0]['original'] is first
assert capture.originals[1]['original'] is close_error
assert read.closes==1 and not capture.closed and not capture.eof
marker=b'tui20-native-owner-stage terminal-recovery-enter\n'
stream=Original(io.BytesIO(marker));capture=module.OwnerStderrCapture(stream,BadForward());capture.start();capture.finish()
assert [item['original'] for item in capture.originals]==[first,later]
assert b''.join(capture.raw)==marker and capture.eof and capture.closed and stream.closes==1
print('private-stage-capture-pass')
`));
