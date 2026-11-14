import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

test('reload refusal requires Core disabled capability and a fresh exact closed diagnostic without effects',()=>{
 const result=spawnSync(process.platform==='win32'?'python':'python3',['-c',String.raw`
import ast,sys
source=ast.parse(open(sys.argv[1],encoding='utf-8').read())
def is_start(node):
 return isinstance(node,ast.If) and isinstance(node.test,ast.Compare) and isinstance(node.test.left,ast.Constant) and node.test.left.value=='reload: disabled: durable operation unavailable'
owner=next(node for node in ast.walk(source) if isinstance(node,ast.Try) and any(is_start(item) for item in node.body))
start=next(index for index,node in enumerate(owner.body) if is_start(node))
end=next(index for index,node in enumerate(owner.body[start:],start) if isinstance(node,ast.If) and any(isinstance(item,ast.Constant) and item.value=='reload or cancellation contract changed' for item in ast.walk(node)))
compiled=compile(ast.Module(body=owner.body[start:end+1],type_ignores=[]),'<actual-reload-assertions>','exec')
capability='reload: disabled: durable operation unavailable'
fixed='Runtime API unavailable: operator stage failed; check connection and retained operation state'
def run(before,rendered,after):
 calls=[];reads=[]
 class Terminal:
  def __init__(self):self.text=before
  def write(self,value):assert value=='l';calls.append(value);self.text+=rendered
  def wait(self,need,seconds,offset):
   assert need==(fixed,) and seconds==15 and offset==len(before)
   assert all(value in self.text[offset:] for value in need)
 term=Terminal();snapshots=iter([[{'operationId':'held-original'}],after])
 def operations(url,token):
  assert (url,token)==('owned-url','owned-token');reads.append(True);return next(snapshots)
 namespace={'term':term,'operations':operations,'url':'owned-url','token':'owned-token'}
 failure=None
 try:exec(compiled,namespace)
 except (AssertionError,RuntimeError) as original:failure=original
 return failure,calls,reads
same=[{'operationId':'held-original'}]
failure,calls,reads=run(capability,fixed,same)
assert failure is None and calls==['l'] and len(reads)==2
for before,rendered,after in (('',fixed,same),(capability,'Runtime API unavailable:',same),(capability,'reload is unavailable: Core does not provide a durable operation contract',same),(capability+fixed,'',same),(capability,fixed,[{'operationId':'new'}]),(capability,fixed+' Confirm reload',same),(capability+' z request Core cancellation',fixed,same),(capability+' z cancel',fixed,same)):
 failure,calls,reads=run(before,rendered,after)
 assert failure is not None
 if not before:assert calls==[] and reads==[]
print('closed-reload-refusal-pass')
`,fileURLToPath(new URL('./tui20-native-posix-five-action.py',import.meta.url))],{encoding:'utf8',env:{...process.env,PYTHONDONTWRITEBYTECODE:'1'}});
 assert.equal(result.status,0,result.stderr);
 assert.equal(result.stdout.trim(),'closed-reload-refusal-pass');
 assert.equal(result.stderr,'');
});
