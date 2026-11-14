// Source/refusal and real-child capture fixtures only. No authentic profile,
// native constructor, generated main or native-positive build is fabricated.
import nodeTest from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,mkdir,writeFile,open,link,rename,rm,lstat,symlink} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {spawn} from 'node:child_process';
import {createHash} from 'node:crypto';
import {executeLinuxCandidateBuild,LinuxBuildAttemptFailure,retainedLinuxBuildAttempt,requireLinuxCandidateSourceAssociation} from './linux-candidate-executor.mjs';
import {completeFileBackedCommand} from './file-backed-command-completion.mjs';

// Linux filesystem/held-handle semantics are mandatory here. Other hosts are
// explicitly N/A, never evidence of Windows native or filesystem acceptance.
const test=(name,body)=>nodeTest(name,{skip:process.platform!=='linux'},body);

async function refusal(inputs){
 try{await executeLinuxCandidateBuild(inputs);assert.fail('build unexpectedly accepted');}
 catch(original){
  assert.ok(original instanceof LinuxBuildAttemptFailure);
  assert.deepEqual(Object.keys(original),[]);
  return retainedLinuxBuildAttempt(original);
 }
}
async function fixture(body){
 const root=await mkdtemp(path.join(tmpdir(),'tui-linux-executor-'));
 const owner=await refusal({}),failures=[];
 try{await body(root,owner);}catch(original){failures.push(original);}
 finally{
  try{await owner.closeAfterLastBorrower();}catch(original){failures.push(original);}
  try{await rm(root,{recursive:true});}catch(original){failures.push(original);}
 }
 if(failures.length)throw new AggregateError(failures,'fixture and retained cleanup failed');
}
async function inputPaths(root){
 const sourceRoot=path.join(root,'source');await mkdir(sourceRoot);
 const values={sourceRoot,attemptRoot:path.join(root,'attempt'),expectedSourceCommit:'1'.repeat(40),environmentOriginal:{}};
 for(const name of ['go','node','git','compiler','profileOriginal','abiGeometryOriginal','nativeGeometryOriginal'])values[name]=path.join(root,name);
 values.transitiveInputs=[values.go];return values;
}
test('opaque primitive first cause remains private and close is cached once',async()=>{
 const marker=Symbol('private original');
 const owner=await refusal({get go(){throw marker;}});
 assert.equal(owner.primaryOriginal,marker);assert.equal(owner.commands.length,0);
 const first=owner.closeAfterLastBorrower();assert.equal(owner.closeAfterLastBorrower(),first);await first;
});
test('missing profile trio originals refuse without creating output or children',async()=>{
 for(const missing of ['profileOriginal','abiGeometryOriginal','nativeGeometryOriginal']){
  await fixture(async(root,unused)=>{
   const values=await inputPaths(root);let owner;
   try{
    for(const name of ['go','node','git','compiler','profileOriginal','abiGeometryOriginal','nativeGeometryOriginal'])if(name!==missing)await writeFile(values[name],'non-tool refusal original '+name);
    owner=await refusal(values);
    assert.equal(owner.phase,'acquire-original-inputs');assert.equal(owner.primaryOriginal.code,'ENOENT');
    assert.equal(owner.commands.length,0);await assert.rejects(lstat(values.attemptRoot),{code:'ENOENT'});
   }finally{if(owner)await owner.closeAfterLastBorrower();}
  });
 }
});
test('hardlinked distinct tool roles refuse with both originals still retained',async()=>{
 await fixture(async(root)=>{
  const values=await inputPaths(root);await writeFile(values.go,'non-tool original');await link(values.go,values.node);
  const owner=await refusal(values);
  try{
   assert.equal(owner.commands.length,0);assert.equal(owner.held.rows.length,2);
   const [a,b]=owner.held.rows;assert.equal(a.before.ino,b.before.ino);assert.equal(a.closeAttempted,undefined);
  }finally{await owner.closeAfterLastBorrower();}
 });
});
test('Linux case-distinct originals remain distinct and independent revalidation continues after failures',async()=>{
 await fixture(async(root,owner)=>{
  const first=path.join(root,'input'),second=path.join(root,'INPUT');
  await writeFile(first,'first');await writeFile(second,'second');
  const a=await owner.held.acquire(first),b=await owner.held.acquire(second);
  assert.notEqual(a,b);assert.equal(owner.held.rows.length,2);
  const marker=Symbol('first read failure'),later=Object.freeze({privateLater:true});
  a.handle.read=async()=>{throw marker;};b.handle.read=async()=>{throw later;};
  await assert.rejects(owner.held.revalidate(),original=>original===marker);
  const originals=owner.held.revalidations.at(-1).originals;
  assert.equal(originals[0],marker);assert.ok(originals.includes(later));
  assert.equal(a.namedReads.length,1);assert.equal(b.namedReads.length,1);
 });
});
test('same bytes on replaced inode and changed bytes on held inode both refuse',async()=>{
 await fixture(async(root,owner)=>{
  const replaced=path.join(root,'replaced'),changed=path.join(root,'changed');
  await writeFile(replaced,'original');await writeFile(changed,'original');
  const a=await owner.held.acquire(replaced),b=await owner.held.acquire(changed);
  const swap=path.join(root,'swap');await writeFile(swap,'original');await rename(swap,replaced);
  await writeFile(changed,'modified');
  await assert.rejects(owner.held.revalidate());
  assert.ok(owner.held.revalidations.at(-1).originals.length>=2);
  assert.equal(a.bytes.toString(),'original');assert.equal(b.bytes.toString(),'original');
 });
});
test('output under source or source-alias parent refuses before commands',async()=>{
 await fixture(async(root)=>{
  const values=await inputPaths(root);
  const alias=path.join(root,'alias');await symlink(values.sourceRoot,alias);
  for(const attemptRoot of [path.join(values.sourceRoot,'attempt'),path.join(alias,'attempt')]){
   const owner=await refusal({...values,attemptRoot});
   try{assert.equal(owner.commands.length,0);assert.equal(owner.held.rows.length,0);await assert.rejects(lstat(attemptRoot),{code:'ENOENT'});}
   finally{await owner.closeAfterLastBorrower();}
  }
 });
});
const row=text=>{const bytes=Buffer.from(text);return {bytes,sha256:createHash('sha256').update(bytes).digest('hex')};};
const reference=row=>({size:row.bytes.length,sha256:row.sha256});
test('closed paired association binds all three ordered originals and both outputs without upgrading authority',()=>{
 const inputs=['profile','abi','geometry'].map(row),outputs=['go-source','c-source'].map(row);
 const good={kind:'linux-paired-source-association',inputs:inputs.map(reference),outputs:outputs.map(reference),nativeEntrySupplied:false,authenticatedIssuer:false};
 requireLinuxCandidateSourceAssociation(good,inputs,outputs);
 for(const mutate of [value=>value.inputs.reverse(),value=>value.outputs.reverse(),value=>value.inputs[2].size++,value=>value.outputs[0].sha256='0'.repeat(64),value=>value.inputs[0].extra=true,value=>value.extra=true,value=>value.nativeEntrySupplied=true,value=>value.authenticatedIssuer=true,value=>value.outputs.pop()]){
  const bad=structuredClone(good);mutate(bad);assert.throws(()=>requireLinuxCandidateSourceAssociation(bad,inputs,outputs));
 }
});
test('real child file-backed output retains separate original streams and failed partial output',async()=>{
 await fixture(async(root,owner)=>{
  const output=path.join(root,'partial-output');
  const command={phase:'fixture-only-child',state:'starting',argv:[],cwd:root};
  for(const channel of ['stdout','stderr']){
   const file=path.join(root,channel),handle=await open(file,'wx+');
   const stream={path:file,handle,reads:[]};owner.streams.push(stream);command[channel]=stream;
  }
  // This child tests capture/failure custody, not a fake compiler or native pair.
  command.child=spawn(process.execPath,['-e',"require('node:fs').writeFileSync(process.argv[1],'partial');process.stdout.write('out');process.stderr.write('err');process.exitCode=7",output],{cwd:root,stdio:['ignore',command.stdout.handle.fd,command.stderr.handle.fd]});
  owner.commands.push(command);
  await assert.rejects(completeFileBackedCommand(command,owner,[output]));
  assert.equal(command.exitOriginal.code,7);assert.equal(command.closeOriginal.code,7);
  assert.equal(command.stdout.bytes.toString(),'out');assert.equal(command.stderr.bytes.toString(),'err');
  assert.equal(command.revalidationCompleted,true);
  assert.equal(owner.outputs.find(item=>item.path===output).bytes.toString(),'partial');
  assert.equal(owner.state,'incomplete');
 });
});
