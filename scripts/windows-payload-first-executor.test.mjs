// Refusal-frontier fixtures only. These files are not executable tools, an
// authentic query profile, generated C entry, issuer or positive build input.
import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,link,rm,lstat} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {executeWindowsPayloadFirstBuild,WindowsBuildAttemptFailure,retainedWindowsBuildAttempt} from './windows-payload-first-executor.mjs';

async function refusal(input){
 try{await executeWindowsPayloadFirstBuild(input);assert.fail('build unexpectedly accepted');}
 catch(original){
  assert.ok(original instanceof WindowsBuildAttemptFailure);
  assert.deepEqual(Object.keys(original),[]);
  return retainedWindowsBuildAttempt(original);
 }
}
test('missing exact inputs and opaque original errors retain the same private failure',async()=>{
 const marker=Object.freeze({privateOriginal:true});
 const input={get go(){throw marker;}};
 const owner=await refusal(input);
 assert.equal(owner.primaryOriginal,marker);
 assert.equal(owner.commands.length,0);
 assert.equal(owner.state,'incomplete');
 const first=owner.closeAfterLastBorrower();
 assert.equal(owner.closeAfterLastBorrower(),first);
 await first;
 const absent=await refusal({});
 assert.equal(absent.commands.length,0);
 await absent.closeAfterLastBorrower();
});
test('missing actual profile or entry refuses before output creation or any child',async()=>{
 for(const missing of ['queryProfile','generatedEntry']){
  const root=await mkdtemp(path.join(tmpdir(),'tui-windows-build-refusal-'));
  let owner;const failures=[];
  try{
   const values={};
   for(const name of ['go','node','git','compiler','queryProfile','generatedEntry']){
    values[name]=path.join(root,name);
    if(name!==missing)await writeFile(values[name],'refusal-only original '+name);
   }
   const attemptRoot=path.join(root,'never-created');
   owner=await refusal({...values,goroot:root,sourceRoot:root,attemptRoot,
    expectedSourceCommit:'1'.repeat(40),environmentOriginal:{},transitiveInputs:[values.go]});
   assert.equal(owner.commands.length,0);
   assert.equal(owner.phase,'acquire-original-inputs');
   assert.equal(owner.primaryOriginal.code,'ENOENT');
   assert.equal(owner.held.rows.length,missing==='queryProfile'?4:5);
   await assert.rejects(lstat(attemptRoot),{code:'ENOENT'});
  }catch(original){failures.push(original);}
  finally{
   try{if(owner)await owner.closeAfterLastBorrower();}catch(original){failures.push(original);}
   try{await rm(root,{recursive:true});}catch(original){failures.push(original);}
  }
  if(failures.length)throw new AggregateError(failures,'refusal fixture and cleanup failed');
 }
});
test('one pathname cannot fill distinct principal tool/profile/entry roles',async()=>{
 const file=path.resolve('refusal-only-absent-input');
 const owner=await refusal({go:file,node:file,git:file,compiler:file,queryProfile:file,generatedEntry:file,
  goroot:path.dirname(file),sourceRoot:path.dirname(file),attemptRoot:path.resolve('refusal-only-absent-output')});
 assert.equal(owner.commands.length,0);assert.equal(owner.held.rows.length,0);
 await owner.closeAfterLastBorrower();
});
test('distinct input names with the same native original refuse with both originals held',async()=>{
 const root=await mkdtemp(path.join(tmpdir(),'tui-windows-build-alias-'));
 let owner;const failures=[];
 try{
  const go=path.join(root,'go'),node=path.join(root,'node');
  await writeFile(go,'refusal-only non-tool original');await link(go,node);
  owner=await refusal({go,node,git:path.join(root,'git'),compiler:path.join(root,'compiler'),
   goroot:root,sourceRoot:root,queryProfile:path.join(root,'profile'),generatedEntry:path.join(root,'entry'),
   attemptRoot:path.join(root,'attempt'),expectedSourceCommit:'1'.repeat(40),
   environmentOriginal:{},transitiveInputs:[go]});
  assert.equal(owner.commands.length,0);
  assert.equal(owner.held.rows.length,2);
  const [a,b]=owner.held.rows;
  assert.equal(a.before.dev,b.before.dev);assert.equal(a.before.ino,b.before.ino);
  assert.equal(a.closeAttempted,undefined);assert.equal(b.closeAttempted,undefined);
  await owner.closeAfterLastBorrower();
  assert.equal(a.closeCompleted,true);assert.equal(b.closeCompleted,true);
 }catch(original){failures.push(original);}
 finally{
  try{if(owner)await owner.closeAfterLastBorrower();}catch(original){failures.push(original);}
  try{await rm(root,{recursive:true});}catch(original){failures.push(original);}
 }
 if(failures.length)throw new AggregateError(failures,'alias fixture and cleanup failed');
});
