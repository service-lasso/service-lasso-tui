import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

test('invalid credential observes exact authentication denial without selecting services or requesting a mutation',()=>{
 const result=spawnSync(process.platform==='win32'?'python':'python3',['-c',String.raw`
import ast,sys
source=ast.parse(open(sys.argv[1],encoding='utf-8').read())
loop=next(node for node in ast.walk(source) if isinstance(node,ast.For) and isinstance(node.target,ast.Tuple) and [item.id for item in node.target.elts if isinstance(item,ast.Name)]==['profile','label','expected','audit_expected'])
original=ast.literal_eval(loop.iter)
assert original==(('missing','missing-credential',2,0),('invalid','invalid-credential',0,0),('denied','scope-denied',0,5))
# Execute only the actual invalid-case source control flow against recorders.
# These recorders are not a runtime, credential issuer or native-positive proof.
loop.iter=ast.Tuple(elts=[loop.iter.elts[1]],ctx=ast.Load())
events=[];operation_reads=[];audit_reads=[]
class Terminal:
 def __init__(self,*args):
  assert args[2]=='invalid';events.append('open')
 def wait(self,need,seconds):
  assert need==('Runtime authentication denied:',) and seconds==30
  events.append('authentication-denied')
 def write(self,value):raise AssertionError('invalid credential attempted keyboard mutation')
 def close(self,expected):
  assert expected==0;events.append('close');return 'terminal_exited_zero'
def detail(term):raise AssertionError('invalid credential attempted unavailable service detail')
def operations(url,token):operation_reads.append((url,token));return []
def audit_count(url,token):audit_reads.append((url,token));return 0
class Args:executable='unused-source-fixture'
namespace={'Terminal':Terminal,'detail':detail,'operations':operations,'audit_count':audit_count,
 'diagnostic_stage':lambda label:events.append(label),'outcome':{'terminals':[]},'adverse':[],
 'url':'fixture','token':'fixture','launch_fd':None,'args':Args(),'env':{'SERVICE_LASSO_INVALID_TOKEN':'PRIVATE_SENTINEL'}}
exec(compile(ast.fix_missing_locations(ast.Module(body=[loop],type_ignores=[])),'<invalid-case-source>','exec'),namespace)
assert events==['invalid-credential-enter','open','authentication-denied','close','invalid-credential-return']
assert len(operation_reads)==2 and len(audit_reads)==2
assert namespace['outcome']['terminals']==[{'terminal':'invalid-credential','exit':'terminal_exited_zero'}]
assert namespace['adverse']==[{'case':'invalid-credential','beforeOperationCount':0,'afterOperationCount':0,'noOperation':True,'coreDeniedAuditBefore':0,'coreDeniedAuditAfter':0,'coreDeniedAuditDelta':0}]
print('invalid-credential-control-flow-pass')
`,fileURLToPath(new URL('./tui20-native-posix-five-action.py',import.meta.url))],{encoding:'utf8',env:{...process.env,PYTHONDONTWRITEBYTECODE:'1'}});
 assert.equal(result.status,0,result.stderr);
 assert.equal(result.stdout.trim(),'invalid-credential-control-flow-pass');
 assert.equal(result.stderr,'');
});
