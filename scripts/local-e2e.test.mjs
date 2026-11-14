import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,chmod,readFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {spawn} from 'node:child_process';
import {parseOptions,validateConnection,suitePassed,assertSourceReceipt,assertSelectedTools,assertReadyPaths,command} from './run-local-e2e.mjs';

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
  const file=path.join(root,'connection.json');await writeFile(file,JSON.stringify({url:'http://127.0.0.1:12345',token:'x'.repeat(32)}),{mode:0o600});assert.equal((await validateConnection(file,async()=>({verified:true}))).token.length,32);
  if(process.platform!=='win32'){await chmod(file,0o644);await assert.rejects(validateConnection(file,async()=>({verified:true})));await chmod(file,0o600);}
  await writeFile(file,JSON.stringify({url:'http://remote.invalid',token:'x'.repeat(32)}));await assert.rejects(validateConnection(file,async()=>({verified:true})));
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
  const exit=await new Promise((resolve,reject)=>{const child=spawn(process.env.TUI_E2E_TEST_PYTHON??(process.platform==='win32'?'python':'python3'),['-B',driver,'--binary',executable,'--connection-file',connection,'--output',output,'--mode','v2'],{stdio:['ignore','pipe','pipe']});child.on('error',reject);child.on('close',resolve);});
  assert.equal(exit,2);const result=JSON.parse(await readFile(path.join(output,'result.json')));
  assert.equal(result.status,'blocked');assert.equal(result.exitCode,0);assert.equal(result.actualWaitpid,true);assert.equal(result.interruptedForCleanup,false);
  assert.equal(result.readyBeforeKeys,true);assert.equal(result.tokenLeaked,false);assert.ok(result.qToReapSeconds>=.2);assert.ok(result.qToReapSeconds<2);
  const ready=result.events.find(row=>Object.hasOwn(row,'readyAtSeconds'));const key=result.events.find(row=>row.key==='q');assert.ok(key.atSeconds>=ready.readyAtSeconds);
 }finally{await rm(root,{recursive:true});}
});

test('stale or unqualified Core receipts fail before private connection use',()=>{
 const receipt={schemaVersion:1,coreRevision:'a'.repeat(40),coreTree:'b'.repeat(40),classification:'source-built-ordinary-smoke',cleanSource:true,coreArtifactSHA256:'c'.repeat(64),sourceInputSHA256:'d'.repeat(64),artifactInventorySHA256:'e'.repeat(64),initializationAPI:true,privacy:{verified:true,policy:process.platform==='win32'?'current-owner-and-system':'current-owner-mode',protected:true,proofSHA256:'3'.repeat(64)},build:{command:[process.execPath,'/tools/npm-cli.js','run','build'],exitCode:0,signal:null},tools:{node:'v22.23.2',nodeSHA256:'1'.repeat(64),npmCLI:{path:'/tools/npm-cli.js',sha256:'2'.repeat(64),packageVersion:'10'},npm:'10',typescript:'5',compilerSHA256:'f'.repeat(64)},installedDependencies:{dependency:{}}};
 assert.doesNotThrow(()=>assertSourceReceipt(receipt,'a'.repeat(40)));
 for(const mutation of [row=>row.classification='unqualified-artifact',row=>row.cleanSource=false,row=>row.initializationAPI=false,row=>row.build.exitCode=1,row=>delete row.artifactInventorySHA256,row=>delete row.tools.compilerSHA256,row=>row.privacy.verified=false,row=>delete row.tools.nodeSHA256,row=>delete row.tools.npmCLI.sha256]){const changed=structuredClone(receipt);mutation(changed);assert.throws(()=>assertSourceReceipt(changed,'a'.repeat(40)));}
});
test('foreign ready paths deny before any private credential read',async()=>{
 const root=await mkdtemp(path.join(tmpdir(),'tui-e2e-path-'));try{
 const connectionFile=path.join(root,'connection.json'),receiptFile=path.join(root,'receipt.json');await writeFile(connectionFile,'{}');await writeFile(receiptFile,'{}');
 await assertReadyPaths({connectionFile,receiptFile},root);await assert.rejects(assertReadyPaths({connectionFile:path.join(root,'foreign.json'),receiptFile},root));
 }finally{await rm(root,{recursive:true});}
});
test('bounded command rejects timeout and waits actual child close',async()=>{
 const root=await mkdtemp(path.join(tmpdir(),'tui-e2e-timeout-'));try{
 await assert.rejects(command([process.execPath,'-e','setInterval(()=>{},1000)'],{cwd:root,env:process.env,log:path.join(root,'private.log'),timeout:100}));
 assert.ok((await readFile(path.join(root,'private.log'))).length<1024);
 }finally{await rm(root,{recursive:true});}
});

test('actual command capture uses supplied private writer and retains default capture behavior',async()=>{
 const root=await mkdtemp(path.join(tmpdir(),'tui-e2e-writer-'));try{
  const log=path.join(root,'injected.log'),observed=[];
  const output=await command([process.execPath,'-e',"process.stdout.write('controlled-out');process.stderr.write('controlled-err')"],{cwd:root,env:process.env,log,writePrivate:async(file,bytes)=>observed.push({file,bytes:Buffer.from(bytes)})});
  assert.equal(observed.length,1);assert.equal(observed[0].file,log);assert.equal(observed[0].bytes.toString(),output);assert.ok(output.includes('controlled-out'));assert.ok(output.includes('controlled-err'));await assert.rejects(readFile(log),{code:'ENOENT'});
  const fallback=path.join(root,'default.log');await command([process.execPath,'-e',"process.stdout.write('default-capture')"],{cwd:root,env:process.env,log:fallback});assert.equal(await readFile(fallback,'utf8'),'default-capture');
 }finally{await rm(root,{recursive:true});}
});

test('Core receipts bind actual selected Node and npm path bytes and package version',()=>{
 const selected={nodeSHA256:'a'.repeat(64),npmCLI:{path:'/actual/npm/bin/npm-cli.js',sha256:'b'.repeat(64),packageVersion:'11.0.0'}};
 const receipt={tools:structuredClone(selected)};assert.doesNotThrow(()=>assertSelectedTools(receipt,selected));
 for(const mutate of [row=>row.tools.nodeSHA256='c'.repeat(64),row=>row.tools.npmCLI.path='/foreign/npm-cli.js',row=>row.tools.npmCLI.sha256='d'.repeat(64),row=>row.tools.npmCLI.packageVersion='10.0.0']){const changed=structuredClone(receipt);mutate(changed);assert.throws(()=>assertSelectedTools(changed,selected));}
});
