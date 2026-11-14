import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

test('denied lifecycle preflight admits only permission-stage title and retains failed actual audit counts',()=>{
 const result=spawnSync(process.platform==='win32'?'python':'python3',['-c',String.raw`
import ast,sys
source=ast.parse(open(sys.argv[1],encoding='utf-8').read())
loop=next(node for node in ast.walk(source) if isinstance(node,ast.For) and isinstance(node.target,ast.Tuple) and [item.id for item in node.target.elts if isinstance(item,ast.Name)]==['profile','label','expected','audit_expected'])
assert ast.literal_eval(loop.iter)==(('missing','missing-credential',2,0),('invalid','invalid-credential',0,0),('denied','scope-denied',0,5))
root=next(node for node in ast.walk(source) if isinstance(node,ast.Assign) and len(node.targets)==1 and isinstance(node.targets[0],ast.Subscript) and isinstance(node.targets[0].value,ast.Name) and node.targets[0].value.id=='outcome' and isinstance(node.targets[0].slice,ast.Constant) and node.targets[0].slice.value=='adverseAudit')
loop.iter=ast.Tuple(elts=[loop.iter.elts[2]],ctx=ast.Load())
compiled=compile(ast.fix_missing_locations(ast.Module(body=[root,loop],type_ignores=[])),'<denied-case-source>','exec')
def attempt(observed,audit_delta):
 events=[];audits=[]
 class Terminal:
  def __init__(self,*args):assert args[2]=='denied';events.append('open')
  def wait(self,need,seconds):
   assert seconds==30
   if need==('Runtime identity:',):events.append('runtime-identity');return
   assert need==('Core action unavailable:',) and observed==need[0]
   events.append('permission-refused')
  def write(self,value):assert value=='i';events.append('preflight-i')
  def close(self,expected):assert expected==0;events.append('close');return 'terminal_exited_zero'
 def audit_count(url,token):audits.append(True);return 0 if len(audits)==1 else audit_delta
 class Args:executable='unused-source-fixture'
 namespace={'Terminal':Terminal,'detail':lambda term:events.append('public-service-detail'),
  'operations':lambda *_:[],'audit_count':audit_count,'diagnostic_stage':lambda label:events.append(label),
  'outcome':{'outcome':'failed','terminals':[]},'adverse':[],'url':'fixture','token':'fixture',
  'launch_fd':None,'args':Args(),'env':{'SERVICE_LASSO_DENIED_TOKEN':'PRIVATE_SENTINEL'}}
 failed=None
 try:exec(compiled,namespace)
 except (AssertionError,RuntimeError) as original:failed=original
 assert namespace['outcome']['adverseAudit'] is namespace['adverse']
 assert namespace['outcome']['outcome']=='failed'
 return namespace,events,failed
namespace,events,failed=attempt('Core action unavailable:',5)
assert failed is None
assert events==['denied-credential-enter','open','runtime-identity','public-service-detail','preflight-i','permission-refused','close','denied-credential-return']
assert namespace['adverse'][0]['coreDeniedAuditDelta']==5 and namespace['adverse'][0]['noOperation']
for observed in ('Runtime authentication denied:','Runtime API unavailable:'):
 namespace,events,failed=attempt(observed,5)
 assert isinstance(failed,AssertionError)
 assert namespace['adverse']==[] and namespace['outcome']['terminals']==[]
# Unexpected real counts stay failed and remain reachable before the original
# strict assertion; recording a failed prefix grants no acceptance or success.
namespace,events,failed=attempt('Core action unavailable:',10)
assert isinstance(failed,RuntimeError)
assert namespace['outcome']['adverseAudit'][0]['coreDeniedAuditDelta']==10
assert namespace['outcome']['outcome']=='failed'
print('denied-credential-control-flow-pass')
`,fileURLToPath(new URL('./tui20-native-posix-five-action.py',import.meta.url))],{encoding:'utf8',env:{...process.env,PYTHONDONTWRITEBYTECODE:'1'}});
 assert.equal(result.status,0,result.stderr);
 assert.equal(result.stdout.trim(),'denied-credential-control-flow-pass');
 assert.equal(result.stderr,'');
});
