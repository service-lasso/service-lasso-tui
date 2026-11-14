import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const helper=fileURLToPath(new URL('./tui20-native-posix-five-action.py',import.meta.url));
function check(source){
 const result=spawnSync('python3',['-c',source,helper],{encoding:'utf8',env:{...process.env,PYTHONDONTWRITEBYTECODE:'1'}});
 assert.equal(result.status,0,result.stderr);
 // Fixed stage lines only; private terminal bytes and original failures stay private.
 assert.ok(result.stderr.split('\n').filter(Boolean).every(line=>line.startsWith('tui20-native-owner-stage ')));
 assert.equal(result.stdout.trim(),'natural-terminal-recovery-pass');
}
const setup=String.raw`
import json,os,pathlib,pty,runpy,sys,tempfile,tty,types
from unittest.mock import patch
h=runpy.run_path(sys.argv[1]);Terminal=h['Terminal'];BoundProcess=h['BoundProcess']
def terminal():
 master,slave=pty.openpty();tty.setraw(slave);pid=os.fork()
 if pid==0:
  os.close(master)
  while os.read(slave,1)!=b'q': pass
  body=b'private-terminal-body-'*65536
  while body:
   written=os.write(slave,body);body=body[written:]
  os.close(slave);os._exit(0)
 os.close(slave)
 term=Terminal.__new__(Terminal);term.master=master;term.child=BoundProcess(pid)
 term.text='';term.label='fixture';term.exit_reason=None;term.read_observations=[]
 return term
`;
test('normal q drains a real PTY beyond kernel capacity and observes actual child exit with once-only q/close',{skip:process.platform!=='linux'},()=>check(setup+String.raw`
term=terminal();writes=[];write=term.write
term.write=lambda value:writes.append(value) or write(value)
assert term.close()=='terminal_exited_zero'
assert term.child.returncode==0 and len(term.text)>65536
assert writes==['q'] and term.quit_original['returned']==1
assert term.master_close_original['completed']
with patch.object(h['time'],'monotonic',lambda:term.close_expires+100):
 assert term.close()=='terminal_exited_zero'
try:term.close(2);raise AssertionError('cached exit bypassed expected status')
except RuntimeError:pass
assert writes==['q']
try:os.fstat(term.master);raise AssertionError('PTY retained after observed close')
except OSError:pass
print('natural-terminal-recovery-pass')
`));
test('failed assertion persists before normal q and remains failed after real pressure/drain/natural reaping',{skip:process.platform!=='linux'},()=>check(setup+String.raw`
term=terminal();write=term.write;writes=[]
with tempfile.TemporaryDirectory() as root:
 outcome={'outcome':'failed','terminals':[]}
 def request(value):
  persisted=json.loads(pathlib.Path(root,'native-exit-receipt.json').read_text())
  assert persisted['outcome']=='failed' and persisted['failureReason']=='native_harness_assertion_failed'
  writes.append(value);return write(value)
 term.write=request
 original=RuntimeError('private-first-assertion');observation={'bodyPrimaryPresent':True,'bodyPrimary':original}
 assert h['retain_primary_and_recover'](root,outcome,term)
 assert observation['bodyPrimary'] is original
 assert outcome['outcome']=='failed' and outcome['recoveryObservation']['observed']
 assert json.loads(pathlib.Path(root,'native-exit-receipt.json').read_text())['outcome']=='failed'
 assert term.child.returncode==0 and len(term.text)>65536 and writes==['q']
 assert term.master_close_original['completed']
print('natural-terminal-recovery-pass')
`));
test('same normal close expiry is never renewed and later quit/read/close errors do not replace failed primary',{skip:process.platform!=='linux'},()=>check(setup+String.raw`
from unittest.mock import patch
term=Terminal.__new__(Terminal);term.master=123;term.label='fixture';term.exit_reason=None;term.read_observations=[]
term.child=types.SimpleNamespace(returncode=None,reaped_unowned=False,poll=lambda:None)
writes=[];term.write=lambda value:writes.append(value) or 1
clock=iter([0,11,12])
with patch.object(h['time'],'monotonic',lambda:next(clock)):
 for unused in range(2):
  try:term.close();raise AssertionError('unobserved child accepted')
  except h['TerminalUnresolved']:pass
assert writes==['q'] and term.close_expires==10
assert not hasattr(term,'master_close_original') and term.child.returncode is None
first=OSError('private-quit');later=OSError('private-read');last=OSError('private-close');original=RuntimeError('private-primary')
term=Terminal.__new__(Terminal);term.master=124;term.label='fixture';term.exit_reason=None;term.read_observations=[]
count=[0]
def poll():
 count[0]+=1
 return 0 if count[0]>2 else None
term.child=types.SimpleNamespace(returncode=None,reaped_unowned=False,poll=poll)
def failed_write(value):raise first
def failed_read(seconds):raise later
def failed_close(descriptor):raise last
term.write=failed_write;term.read=failed_read
with tempfile.TemporaryDirectory() as root,patch.object(h['os'],'close',failed_close):
 outcome={'outcome':'failed','terminals':[]};observation={'bodyPrimary':original}
 assert h['retain_primary_and_recover'](root,outcome,term)
 assert observation['bodyPrimary'] is original and outcome['outcome']=='failed'
 assert [item['original'] for item in term.lifecycle_originals]==[first,later,last]
 assert term.quit_original['attempted'] and not term.master_close_original['completed']
 term.close_master_once();assert len(term.lifecycle_originals)==3
print('natural-terminal-recovery-pass')
`));
