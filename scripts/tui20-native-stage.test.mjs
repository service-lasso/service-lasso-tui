import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

// Inspect the complete source AST and execute only the isolated output helper;
// never import the harness or start a native owner/runtime/terminal.
test('native progress diagnostics contain only fixed source labels and flush every line',()=>{
 const result=spawnSync(process.platform==='win32'?'python':'python3',['-c',String.raw`
import ast,sys,types
source=open(sys.argv[1],encoding='utf-8').read()
tree=ast.parse(source)
expected={'terminal-normal-close-enter','terminal-normal-close-return','terminal-raw-pid-recovery-enter','terminal-recovery-enter','bound-replacement-probe-enter','bound-replacement-probe-return','missing-credential-enter','missing-credential-return','invalid-credential-enter','invalid-credential-return','denied-credential-enter','denied-credential-return','allowed-five-actions-enter','allowed-five-actions-return','allowed-action-enter','allowed-action-return','reconnect-enter','reconnect-return','primary-recovery-persist-enter','primary-recovery-persist-return','action-preview-enter','action-preview-return','action-submit-enter','action-readback-enter','action-readback-return'}
calls=[node for node in ast.walk(tree) if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='diagnostic_stage']
assert calls
labels=[]
for call in calls:
 assert len(call.args)==1 and not call.keywords
 assert isinstance(call.args[0],ast.Constant) and isinstance(call.args[0].value,str)
 labels.append(call.args[0].value)
assert set(labels)==expected
function=next(node for node in tree.body if isinstance(node,ast.FunctionDef) and node.name=='diagnostic_stage')
class Capture:
 def __init__(self): self.lines=[];self.flushes=0
 def write(self,line): self.lines.append(line)
 def flush(self): self.flushes+=1
capture=Capture();namespace={'sys':types.SimpleNamespace(stderr=capture),'DIAGNOSTIC_ORIGINALS':[]}
exec(compile(ast.Module(body=[function],type_ignores=[]),'<isolated-stage>','exec'),namespace)
for label in sorted(expected): namespace['diagnostic_stage'](label)
assert capture.flushes==len(expected)
assert capture.lines==['tui20-native-owner-stage '+label+'\n' for label in sorted(expected)]
assert all('PRIVATE_SENTINEL' not in line for line in capture.lines)
first=OSError('private write');later=ValueError('private flush')
class Broken:
 def write(self,line): raise first
 def flush(self): raise later
namespace['sys'].stderr=Broken()
namespace['diagnostic_stage']('terminal-normal-close-enter')
assert namespace['DIAGNOSTIC_ORIGINALS']==[first,later]
# A broken diagnostic stream cannot suppress the real close/recovery methods.
terminal=next(node for node in tree.body if isinstance(node,ast.ClassDef) and node.name=='Terminal')
methods=[node for node in terminal.body if isinstance(node,ast.FunctionDef) and node.name in ('retain_lifecycle_original','request_quit_once','close_master_once','recovery_escape_once','wait_draining_until_observed','close','recover_until_observed')]
closes=[];events=[]
namespace['FAILED_PREVIEW_TITLES']=('Runtime authentication denied:','Runtime permission denied:','Core action unavailable:','Operation reconciliation required:','Operation recovery storage unavailable:','Runtime API unavailable:')
namespace.update({'os':types.SimpleNamespace(close=lambda descriptor:closes.append(descriptor)),
 'time':types.SimpleNamespace(monotonic=lambda:0,sleep=lambda value:None),'terminal_exit_reason':lambda result:'terminal_exited_zero','subprocess':types.SimpleNamespace(TimeoutExpired=TimeoutError),
 'ChildReapedUnowned':type('ChildReapedUnowned',(Exception,),{})})
exec(compile(ast.fix_missing_locations(ast.Module(body=[ast.ClassDef(name='TestTerminal',bases=[],keywords=[],body=methods,decorator_list=[])],type_ignores=[])),'<isolated-terminal>','exec'),namespace)
class Child:
 returncode=None;reaped_unowned=False
 def poll(self): events.append('poll');return self.returncode
term=namespace['TestTerminal']();term.child=Child();term.master=123;term.label='fixture'
def quit(value):
 events.append(value);term.child.returncode=0;return 1
term.write=quit
assert term.close()=='terminal_exited_zero'
assert events==['poll','q','poll'] and closes==[123]
assert term.recover_until_observed()['observed'] is True
assert events==['poll','q','poll','poll','poll'] and closes==[123]
assert namespace['DIAGNOSTIC_ORIGINALS']==[first,later]*4
print('fixed-stage-diagnostics-pass')
`,fileURLToPath(new URL('./tui20-native-posix-five-action.py',import.meta.url))],{encoding:'utf8',env:{...process.env,SERVICE_LASSO_API_TOKEN:'PRIVATE_SENTINEL',PYTHONDONTWRITEBYTECODE:'1'}});
 assert.equal(result.status,0,result.stderr);
 assert.equal(result.stdout.trim(),'fixed-stage-diagnostics-pass');
 assert.equal(result.stderr,'');
});
