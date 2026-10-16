import {lstat} from 'node:fs/promises';
import {CommandFailureLedger} from './command-failure-ledger.mjs';

// Internal SAME actual file-backed child completion, not a spawn/admission API.
// The enclosing attempt roots row before spawn and retains all handles/failures
// through later borrowers. Actual close does not settle native family custody.
export async function completeFileBackedCommand(row,attempt,destinations){
 const held=attempt.held;
 row.failures??=new CommandFailureLedger();row.errors=row.failures.originals;
 await new Promise(resolve=>{
  row.child.on('error',original=>row.failures.record('child-error',original));
  row.child.once('exit',(code,signal)=>{
   row.exitOriginal={code,signal};
   if(!row.errors.length&&(code!==0||signal!==null))row.failures.record('child-exit',new Error('payload-first command failed'));
  });
  row.child.once('close',(code,signal)=>{
   row.closeOriginal={code,signal};row.state='closed';
   if(!row.errors.length&&(code!==0||signal!==null))row.failures.record('child-close',new Error('payload-first command failed'));
   resolve();
  });
 });
 async function retain(file){
  const output=await held.acquire(file);
  if(!output.before||!output.bytes||held.rows.some(previous=>!previous.before))throw new Error('held output original incomplete');
  if(held.rows.some(previous=>previous!==output&&previous.before.dev===output.before.dev&&previous.before.ino===output.before.ino))throw new Error('build output aliases held original');
  attempt.outputs.push(output);return output;
 }
 for(const channel of ['stdout','stderr']){
  const stream=row[channel];
  const sync=await row.failures.attempt(channel+'-sync',()=>stream.handle.sync());
  stream.syncCompleted=sync.completed;
  const read=await row.failures.attempt(channel+'-readback',async()=>Object.assign(stream,await held.read(stream.handle,stream)));
  stream.readbackCompleted=read.completed;
  const retention=await row.failures.attempt(channel+'-retain',()=>retain(stream.path));
  stream.retentionCompleted=retention.completed;
 }
 row.outputOriginals=[];
 for(const file of destinations){
  const occurrence={path:file};row.outputOriginals.push(occurrence);
  const result=await row.failures.attempt('destination-retain',async()=>{await lstat(file);return retain(file);});
  occurrence.completed=result.completed;
  if(!result.completed)occurrence.original=result.original;
 }
 // This attempt is independent of every preceding capture/output failure.
 const revalidation=await row.failures.attempt('post-borrower-revalidate',()=>held.revalidate());
 row.revalidationCompleted=revalidation.completed;
 if(row.errors.length){row.primaryOriginal=row.errors[0];row.failures.throwFirst();}
 return row.stdout.bytes;
}
