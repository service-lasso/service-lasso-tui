import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,chmod,readFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {spawn} from 'node:child_process';
import {parseOptions,validateConnection,suitePassed} from './run-local-e2e.mjs';

test('all cannot pass compatibility alone or v2 unavailable',()=>{
 assert.equal(suitePassed('all',[{mode:'compat',status:'passed'},{mode:'v2',status:'blocked'}]),false);
 assert.equal(suitePassed('compat',[{mode:'compat',status:'passed'}]),true);
 assert.equal(suitePassed('v2',[{mode:'v2',status:'blocked'}]),false);
});
test('explicit source revision and supported suites are mandatory',()=>{
 assert.throws(()=>parseOptions([]));assert.throws(()=>parseOptions(['--core-dir','/tmp/core','--expected-core-revision','develop']));
 assert.equal(parseOptions(['--core-dir','/tmp/core','--expected-core-revision','a'.repeat(40),'--suite','compat']).suite,'compat');
 assert.throws(()=>parseOptions(['--core-dir','/tmp/core','--expected-core-revision','a'.repeat(40),'--suite','compat','--suite','all']));
});
test('private connection rejects broad permissions and remote token destinations',async()=>{
 const root=await mkdtemp(path.join(tmpdir(),'tui-e2e-input-'));try{
  const file=path.join(root,'connection.json');await writeFile(file,JSON.stringify({url:'http://127.0.0.1:12345',token:'x'.repeat(32)}),{mode:0o600});assert.equal((await validateConnection(file)).token.length,32);
  if(process.platform!=='win32'){await chmod(file,0o644);await assert.rejects(validateConnection(file));await chmod(file,0o600);}
  await writeFile(file,JSON.stringify({url:'http://remote.invalid',token:'x'.repeat(32)}));await assert.rejects(validateConnection(file));
 }finally{await rm(root,{recursive:true});}
});
test('actual synthetic terminal waits readiness and reaps after stream EOF before exit',{skip:process.platform==='win32'},async()=>{
 const root=await mkdtemp(path.join(tmpdir(),'tui-e2e-terminal-'));try{
  const executable=path.join(root,'terminal.py');
  await writeFile(executable,`#!/usr/bin/env python3
import os,sys,time,tty
tty.setraw(0)
time.sleep(.25)
print('Service Lasso  operator/v2\\nError: 404 Not Found',flush=True)
while True:
 b=os.read(0,1)
 if b==b'q':
  for fd in [0,1,2]:os.close(fd)
  time.sleep(.25)
  os._exit(0)
`,{mode:0o700});
  const connection=path.join(root,'connection.json');await writeFile(connection,JSON.stringify({url:'http://127.0.0.1:12345',token:'synthetic-terminal-fixture-only'}),{mode:0o600});
  const output=path.join(root,'proof');const driver=path.resolve('scripts/local-e2e-pty.py');
  const exit=await new Promise((resolve,reject)=>{const child=spawn('python3',[driver,'--binary',executable,'--connection-file',connection,'--output',output,'--mode','v2'],{stdio:['ignore','pipe','pipe']});child.on('error',reject);child.on('close',resolve);});
  assert.equal(exit,2);const result=JSON.parse(await readFile(path.join(output,'result.json')));
  assert.equal(result.status,'blocked');assert.equal(result.exitCode,0);assert.equal(result.actualWaitpid,true);assert.equal(result.interruptedForCleanup,false);
  assert.equal(result.readyBeforeKeys,true);assert.equal(result.tokenLeaked,false);assert.ok(result.qToReapSeconds>=.2);assert.ok(result.qToReapSeconds<2);
  const ready=result.events.find(row=>Object.hasOwn(row,'readyAtSeconds'));const key=result.events.find(row=>row.key==='q');assert.ok(key.atSeconds>=ready.readyAtSeconds);
 }finally{await rm(root,{recursive:true});}
});
