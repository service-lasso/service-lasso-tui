import test from 'node:test';
import assert from 'node:assert/strict';
import {open,mkdtemp,writeFile,rename,rm} from 'node:fs/promises';
import {spawn} from 'node:child_process';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {HeldOverlayInputs} from './held-overlay-inputs.mjs';
import {completeFileBackedCommand} from './file-backed-command-completion.mjs';

async function fixture(body){
 const root=await mkdtemp(path.join(tmpdir(),'tui-file-command-'));
 const attempt={held:new HeldOverlayInputs(),outputs:[]},row={},failures=[];
 try{
  const input=path.join(root,'input');await writeFile(input,'held original');
  const original=await attempt.held.acquire(input);
  for(const channel of ['stdout','stderr']){
   const file=path.join(root,channel),handle=await open(file,'wx+');
   row[channel]={path:file,handle,reads:[]};
  }
  await body({root,input,original,row,attempt});
 }catch(original){failures.push(original);}
 finally{
  for(const channel of ['stdout','stderr']){
   const stream=row[channel];if(!stream||stream.closeAttempted)continue;
   stream.closeAttempted=true;
   try{await stream.handle.close();}catch(original){failures.push(original);}
  }
  try{await attempt.held.close();}catch(original){failures.push(original);}
  try{await rm(root,{recursive:true});}catch(original){failures.push(original);}
 }
 if(failures.length)throw new AggregateError(failures,'actual file-backed command fixture or cleanup failed');
}
test('closed stdout original cannot skip real stderr readback or failed input revalidation',async()=>{
 await fixture(async({input,original,row,attempt})=>{
  row.child=spawn(process.execPath,['-e',
   'process.stdin.resume();process.stdin.on("end",()=>{require("node:fs").writeFileSync(process.argv[1],"changed input");process.stdout.write("actual stdout");process.stderr.write("actual stderr");});',input],
   {stdio:['pipe',row.stdout.handle.fd,row.stderr.handle.fd],windowsHide:true});
  const pending=completeFileBackedCommand(row,attempt,[]);
  // The actual child owns its inherited duplicate. Closing this parent's own
  // original supplies a real readback/sync fault, not a synthetic native error.
  const lifecycleFailures=[];
  try{row.stdout.closeAttempted=true;await row.stdout.handle.close();}
  catch(original){lifecycleFailures.push(original);}
  finally{row.child.stdin.end();}
  try{await assert.rejects(pending,original=>original===row.errors[0]);}
  catch(original){lifecycleFailures.push(original);}
  if(lifecycleFailures.length)throw new AggregateError(lifecycleFailures,'actual closed-original lifecycle failed');
  assert.equal(row.state,'closed');
  assert.equal(row.failures.occurrences[0].phase,'stdout-sync');
  assert.equal(row.stdout.syncCompleted,false);assert.equal(row.stdout.readbackCompleted,false);
  assert.equal(row.stderr.syncCompleted,true);assert.equal(row.stderr.readbackCompleted,true);
  assert.equal(row.stderr.retentionCompleted,true);assert.equal(row.stderr.bytes.toString(),'actual stderr');
  assert.equal(row.revalidationCompleted,false);
  assert.equal(row.failures.occurrences.at(-1).phase,'post-borrower-revalidate');
  assert.equal(original.reads.length,2);assert.equal(original.closeAttempted,undefined);
 });
});
test('actual spawn error remains first through missing named output and changed-input failures',async()=>{
 await fixture(async({root,input,original,row,attempt})=>{
  await rename(row.stdout.path,path.join(root,'parked-stdout'));
  await writeFile(input,'changed input');
  row.child=spawn(path.join(root,'genuinely-absent-program'),[],
   {stdio:['ignore',row.stdout.handle.fd,row.stderr.handle.fd],windowsHide:true});
  await assert.rejects(completeFileBackedCommand(row,attempt,[]),original=>{
   assert.equal(original,row.errors[0]);assert.equal(original.code,'ENOENT');return true;
  });
  assert.equal(row.state,'closed');assert.equal(row.failures.occurrences[0].phase,'child-error');
  assert.equal(row.stdout.readbackCompleted,true);assert.equal(row.stdout.retentionCompleted,false);
  assert.equal(row.stderr.readbackCompleted,true);assert.equal(row.stderr.retentionCompleted,true);
  assert.equal(row.revalidationCompleted,false);assert.equal(original.reads.length,2);
  assert.equal(row.failures.occurrences.at(-1).phase,'post-borrower-revalidate');
  assert.equal(original.closeAttempted,undefined);
 });
});
