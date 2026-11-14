import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

test('legacy keyboard macros use standalone mode/confirmation keys and fresh selected-detail acknowledgement',()=>{
 const result=spawnSync(process.platform==='win32'?'python':'python3',['-c',String.raw`
import ast,sys,types
source=ast.parse(open(sys.argv[1],encoding='utf-8').read())
functions=[node for node in source.body if isinstance(node,ast.FunctionDef) and node.name in ('detail','action')]
clock=[0];stages=[]
operation_reads=[]
def operations(url,token):
 assert (url,token)==('actual-owner-url','actual-owner-token');operation_reads.append(True);return []
namespace={'time':types.SimpleNamespace(sleep=lambda value:None,monotonic=lambda:clock[0]),'diagnostic_stage':lambda label:stages.append(label),'operations':operations}
exec(compile(ast.Module(body=functions,type_ignores=[]),'<actual-keyboard-macros>','exec'),namespace)
class Detail:
 def __init__(self):self.text='old Lifecycle: other old tui20-fixture';self.events=[];self.mode='dashboard'
 def write(self,value):
  self.events.append(('write',value))
  if value=='/':
   assert self.mode=='dashboard';self.mode='search';self.text+=' new Search: '
  elif value=='tui20-fixture\r':
   assert self.mode=='search';self.mode='selected';self.text+=' filtered tui20-fixture'
  elif value=='\r':
   assert self.mode=='selected';self.mode='detail';self.text+=' new Lifecycle: fixture tui20-fixture'
  else:raise AssertionError('batched mode key or unexpected keyboard input')
 def wait(self,need,seconds,start=0):
  self.events.append(('wait',need,seconds,start))
  assert all(value in self.text[start:] for value in need)
term=Detail();namespace['detail'](term)
assert term.mode=='detail'
assert [event[1] for event in term.events if event[0]=='write']==['/','tui20-fixture\r','\r']
assert term.events[1][1]==('Search:',) and term.events[1][3]>0
assert term.events[-1][1]==('Lifecycle:','tui20-fixture')
assert term.events[-1][3]>term.events[1][3]
class Action:
 def __init__(self):self.text='';self.writes=[];self.phase_at_write=[];self.reads=0
 def write(self,value):
  self.writes.append(value);self.phase_at_write.append(self.action_phase)
  if value=='i':self.text+='Core preview: install Confirm install'
  elif value=='r\rj\r?\rq\r':pass
  elif value=='y':pass
  else:raise AssertionError('confirmation batched with another printable key')
 def wait(self,need,seconds,start=0):assert all(value in self.text[start:] for value in need)
 def read(self):
  self.reads+=1
  if self.action_phase=='readback-await':self.text+='Core operation succeeded.'
term=Action();namespace['action'](term,'i','install','actual-owner-url','actual-owner-token')
assert term.writes==['i','r\rj\r?\rq\r','y'] and term.reads==2
assert len(operation_reads)==2
assert term.phase_at_write==['preview-await','confirmation-ready','submission-attempted']
assert stages==['action-preview-enter','action-preview-return','action-submit-enter','action-readback-enter','action-readback-return']
assert term.action_phase=='readback-complete'
# A preview redraw can repeat an old error and an old operation result. Neither
# is a post-confirmation event: the first iteration must read, not retry/return.
namespace['operations']=operations
class HistoricalPreview(Action):
 def write(self,value):
  super().write(value)
  if value=='i':self.text+='Runtime API unavailable: old Core operation old succeeded.'
term=HistoricalPreview();namespace['action'](term,'i','install','actual-owner-url','actual-owner-token')
assert term.writes==['i','r\rj\r?\rq\r','y'] and term.reads==2
class FreshReadError(Action):
 def write(self,value):
  if value=='r':
   assert self.action_phase=='readback-await';self.writes.append(value);return
  super().write(value)
 def read(self):
  self.reads+=1
  if self.action_phase=='readback-await':
   self.text+='Runtime API unavailable: fresh' if self.reads==2 else 'Core operation fresh succeeded.'
term=FreshReadError();namespace['action'](term,'i','install','actual-owner-url','actual-owner-token')
assert term.writes==['i','r\rj\r?\rq\r','y','r'] and term.reads==3
# A pre-submit preview failure is identified before any confirmation byte.
class RefusedPreview(Action):
 def wait(self,*args):raise RuntimeError('controlled preview refusal')
term=RefusedPreview()
try:namespace['action'](term,'i','install','actual-owner-url','actual-owner-token');raise AssertionError('preview refusal accepted')
except RuntimeError:pass
assert term.writes==['i'] and term.action_phase=='preview-await'
for before,after in (([],[{'operationId':'new-operation'}]),([{'operationId':'old'}],[{'operationId':'changed'}]),([{'operationId':None}],[{'operationId':None},{'operationId':None}])):
 term=Action();snapshots=iter([before,after]);namespace['operations']=lambda *_:next(snapshots)
 try:
  namespace['action'](term,'i','install','actual-owner-url','actual-owner-token')
  raise AssertionError('unconfirmed operation accepted')
 except RuntimeError:pass
 assert term.writes==['i','r\rj\r?\rq\r'] and term.action_phase=='confirmation-ready'
print('ordered-keyboard-macros-pass')
`,fileURLToPath(new URL('./tui20-native-posix-five-action.py',import.meta.url))],{encoding:'utf8',env:{...process.env,PYTHONDONTWRITEBYTECODE:'1'}});
 assert.equal(result.status,0,result.stderr);
 assert.equal(result.stdout.trim(),'ordered-keyboard-macros-pass');
 assert.equal(result.stderr,'');
});
