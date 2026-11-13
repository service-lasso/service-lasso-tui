import {spawn} from 'node:child_process';
import {writeFile} from 'node:fs/promises';
import {CommandFailureLedger} from './command-failure-ledger.mjs';

// The actual gate roots this private completion owner before invoking the body.
// Direct-child/transport closure is not compiler-family or native authority.
// No timer, retry, kill, opaque-error inspection or early error settlement.
export async function runOwnedOverlayCommand(owner,heldInputs){
 owner.failures=new CommandFailureLedger();owner.originals=owner.failures.originals;
 await heldInputs.revalidate();
 try{
  owner.child=spawn(owner.command,owner.argv,{cwd:owner.cwd,env:owner.environment,
   stdio:['ignore','pipe','pipe'],windowsHide:true});
 }catch(original){owner.failures.record('spawn',original);throw original;}
 return captureOwnedOverlayChild(owner,heldInputs);
}
// Same actual child completion algorithm; the gate supplies only its own spawn
// above. Fixtures may supply their own real spawned child with a live stdin.
// This is private correspondence/transport custody, never family authority.
export async function captureOwnedOverlayChild(owner,heldInputs){
 owner.failures??=new CommandFailureLedger();owner.originals=owner.failures.originals;
 owner.stdoutChunks=[];owner.stderrChunks=[];owner.captures=[];
 const child=owner.child;
 child.stdout.on('data',bytes=>{owner.stdoutChunks.push(bytes);});
 child.stderr.on('data',bytes=>{owner.stderrChunks.push(bytes);});
 child.stdout.on('error',original=>owner.failures.record('stdout-transport',original));
 child.stderr.on('error',original=>owner.failures.record('stderr-transport',original));
 await new Promise(resolve=>{
  child.on('error',original=>owner.failures.record('child-error',original));
  child.once('exit',(code,signal)=>{
   owner.exitOriginal={code,signal};
   if(!owner.originals.length&&(code!==0||signal!==null))owner.failures.record('child-exit',new Error('overlay command failed'));
  });
  child.once('close',(code,signal)=>{
   owner.closeOriginal={code,signal};owner.closeObserved=true;
   if(!owner.originals.length&&(code!==0||signal!==null))owner.failures.record('child-close',new Error('overlay command failed'));
   resolve();
  });
 });
 // Attempt both private captures even when child/transport/first write failed.
 // The actual original list keeps the first failure and all later failures in
 // observed order; no opaque original is stringified or serialized here.
 for(const channel of ['stdout','stderr']){
  const occurrence={channel,file:owner[channel+'Path']};owner.captures.push(occurrence);
  const preparation=await owner.failures.attempt(channel+'-prepare',()=>{
   const bytes=Buffer.concat(owner[channel+'Chunks']);occurrence.bytes=bytes;owner[channel]=bytes;return bytes;
  });
  occurrence.preparationCompleted=preparation.completed;
  if(!preparation.completed)occurrence.original=preparation.original;
  if(preparation.completed){
   const persistence=await owner.failures.attempt(channel+'-persist',()=>writeFile(occurrence.file,preparation.value,{flag:'wx'}));
   occurrence.completed=persistence.completed;
   if(!persistence.completed)occurrence.original=persistence.original;
  }
 }
 const revalidation=await owner.failures.attempt('post-borrower-revalidate',()=>heldInputs.revalidate());
 owner.revalidationCompleted=revalidation.completed;
 owner.failures.throwFirst();
 return owner.stdout;
}
