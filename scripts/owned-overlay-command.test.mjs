import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,mkdir,readFile,rm} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {once} from 'node:events';
import {spawn} from 'node:child_process';
import {HeldOverlayInputs} from './held-overlay-inputs.mjs';
import {runOwnedOverlayCommand,captureOwnedOverlayChild} from './owned-overlay-command.mjs';

async function fixture(body){
 const root=await mkdtemp(path.join(tmpdir(),'tui-overlay-command-')),held=new HeldOverlayInputs(),failures=[];
 try{
  const input=path.join(root,'input');await writeFile(input,'actual held original');
  const original=await held.acquire(input);
  const owner={command:process.execPath,argv:[],cwd:root,environment:{...process.env},
   stdoutPath:path.join(root,'stdout'),stderrPath:path.join(root,'stderr')};
  await body({root,held,original,owner});
 }catch(original){failures.push(original);}
 finally{
  try{await held.close();}catch(original){failures.push(original);}
  try{await rm(root,{recursive:true});}catch(original){failures.push(original);}
 }
 if(failures.length)throw new AggregateError(failures,'actual command fixture or cleanup failed');
}
test('actual transport error cannot settle command or release input before real child close',async()=>{
 await fixture(async({held,original,owner})=>{
  // Actual input keeps the process live independently of damaged stdout.
  owner.argv=['-e','process.stdout.write("ready");process.stdin.resume();process.stdin.on("end",()=>process.exit(0));'];
  let settled=false;
  const child=spawn(owner.command,owner.argv,{cwd:owner.cwd,env:owner.environment,stdio:['pipe','pipe','pipe'],windowsHide:true});
  owner.child=child;
  const pending=captureOwnedOverlayChild(owner,held);pending.then(()=>{settled=true;},()=>{settled=true;});
  const marker=new Error('controlled real transport failure');
  try{
  await once(child.stdout,'data');
  const observed=once(child.stdout,'error');child.stdout.destroy(marker);
  assert.equal((await observed)[0],marker);
  // A subsequent genuine held-file read completes while child remains alive;
  // there is no timer race or fabricated close/exit event.
  await held.revalidate();
  assert.equal(settled,false);assert.equal(owner.closeObserved,undefined);
  assert.equal(original.closeAttempted,undefined);
  child.stdin.end(); // release actual input; no fabricated event, timer or kill
  await assert.rejects(pending,original=>original===marker);
  assert.equal(owner.closeObserved,true);
  assert.equal(owner.originals[0],marker);
  assert.equal(original.closeAttempted,undefined);
  }finally{
   child.stdin.end();
   try{await pending;}catch(original){if(original!==marker)throw original;}
  }
 });
});
test('real stdout persistence failure still captures complete actual stderr',async()=>{
 await fixture(async({held,root,owner})=>{
  await mkdir(owner.stdoutPath); // native writeFile failure, no injected writer
  owner.argv=['-e','process.stderr.write("actual stderr original");process.stdout.write("actual stdout original");'];
  await assert.rejects(runOwnedOverlayCommand(owner,held),original=>{
   assert.equal(original,owner.captures[0].original);return true;
  });
  assert.equal(owner.closeObserved,true);assert.equal(owner.captures.length,2);
  assert.equal(owner.captures[1].completed,true);
  assert.equal(await readFile(path.join(root,'stderr'),'utf8'),'actual stderr original');
 });
});
test('real failed spawn keeps primary identity while both persistence attempts fail independently',async()=>{
 await fixture(async({held,owner})=>{
  owner.command=path.join(owner.cwd,'genuinely-absent-program');
  await mkdir(owner.stdoutPath);await mkdir(owner.stderrPath);
  await assert.rejects(runOwnedOverlayCommand(owner,held),original=>{
   assert.equal(original,owner.originals[0]);assert.equal(original.code,'ENOENT');return true;
  });
  assert.equal(owner.closeObserved,true);assert.equal(owner.captures.length,2);
  assert.equal(owner.originals.length,3);
  assert.equal(owner.originals[1],owner.captures[0].original);
  assert.equal(owner.originals[2],owner.captures[1].original);
 });
});
