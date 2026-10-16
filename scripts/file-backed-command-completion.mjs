import {lstat} from 'node:fs/promises';
import {CommandFailureLedger} from './command-failure-ledger.mjs';

const stable=(a,b)=>a&&b&&a.dev===b.dev&&a.ino===b.ino&&a.size===b.size&&a.mtimeNs===b.mtimeNs&&a.ctimeNs===b.ctimeNs;
function requireSameRead(original,current){
 if(!original?.before||!current?.before||!Buffer.isBuffer(original.bytes)||!Buffer.isBuffer(current.bytes)||
    !stable(original.before,original.after)||!stable(current.before,current.after)||!stable(original.before,current.after)||
    original.returned?.bytesRead!==original.bytes.length||current.returned?.bytesRead!==current.bytes.length||
    !original.bytes.equals(current.bytes))throw new Error('file-backed stream original differs');
}

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
  const retention=await row.failures.attempt(channel+'-retain',async()=>{
   const named=await retain(stream.path);stream.namedOriginal=named;
   if(!stream.readbackCompleted)throw new Error('file-backed stream original unavailable');
   requireSameRead(stream,named);return named;
  });
  stream.retentionCompleted=retention.completed;
 }
 row.outputOriginals=[];
 for(const file of destinations){
  const occurrence={path:file};row.outputOriginals.push(occurrence);
  const result=await row.failures.attempt('destination-retain',async()=>{await lstat(file);return retain(file);});
  occurrence.completed=result.completed;
  if(!result.completed)occurrence.original=result.original;
 }
 // Actual stream handles are owned by attempt.streams, not held.rows. Re-read
 // EACH original independently without replacing its first full snapshot, then
 // join it to the actual retained pathname row. Same bytes on another inode do
 // not join. These handles remain with the enclosing once-close owner.
 for(const channel of ['stdout','stderr']){
  const stream=row[channel];
  const result=await row.failures.attempt(channel+'-original-revalidate',async()=>{
   const current=await held.read(stream.handle,stream);
   if(!stream.readbackCompleted||!stream.retentionCompleted||!stream.namedOriginal)throw new Error('file-backed stream original unavailable');
   requireSameRead(stream,current);requireSameRead(stream.namedOriginal,current);
  });
  stream.originalRevalidationCompleted=result.completed;
 }
 // Named rows/inputs and destinations are revalidated independently even when
 // an actual stream or its correspondence has already failed.
 const revalidation=await row.failures.attempt('post-borrower-revalidate',()=>held.revalidate());
 row.heldRevalidationCompleted=revalidation.completed;
 row.revalidationCompleted=revalidation.completed&&row.stdout.originalRevalidationCompleted&&row.stderr.originalRevalidationCompleted;
 if(row.errors.length){row.primaryOriginal=row.errors[0];row.failures.throwFirst();}
 return row.stdout.bytes;
}
