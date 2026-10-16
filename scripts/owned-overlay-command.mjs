import {spawn} from 'node:child_process';
import {writeFile} from 'node:fs/promises';

// The actual gate roots this private completion owner before invoking the body.
// Direct-child/transport closure is not compiler-family or native authority.
// No timer, retry, kill, opaque-error inspection or early error settlement.
export async function runOwnedOverlayCommand(owner,heldInputs){
 owner.originals=[];
 await heldInputs.revalidate();
 try{
  owner.child=spawn(owner.command,owner.argv,{cwd:owner.cwd,env:owner.environment,
   stdio:['ignore','pipe','pipe'],windowsHide:true});
 }catch(original){owner.originals.push(original);throw original;}
 return captureOwnedOverlayChild(owner,heldInputs);
}
// Same actual child completion algorithm; the gate supplies only its own spawn
// above. Fixtures may supply their own real spawned child with a live stdin.
// This is private correspondence/transport custody, never family authority.
export async function captureOwnedOverlayChild(owner,heldInputs){
 owner.originals=[];owner.stdoutChunks=[];owner.stderrChunks=[];owner.captures=[];
 const retain=original=>{owner.originals.push(original);};
 const child=owner.child;
 child.stdout.on('data',bytes=>{owner.stdoutChunks.push(bytes);});
 child.stderr.on('data',bytes=>{owner.stderrChunks.push(bytes);});
 child.stdout.on('error',retain);child.stderr.on('error',retain);
 await new Promise(resolve=>{
  child.on('error',retain);
  child.once('exit',(code,signal)=>{owner.exitOriginal={code,signal};});
  child.once('close',(code,signal)=>{
   owner.closeOriginal={code,signal};owner.closeObserved=true;
   if(!owner.originals.length&&(code!==0||signal!==null))retain(new Error('overlay command failed'));
   resolve();
  });
 });
 owner.stdout=Buffer.concat(owner.stdoutChunks);owner.stderr=Buffer.concat(owner.stderrChunks);
 // Attempt both private captures even when child/transport/first write failed.
 // The actual original list keeps the first failure and all later failures in
 // observed order; no opaque original is stringified or serialized here.
 for(const [channel,bytes,file] of [['stdout',owner.stdout,owner.stdoutPath],['stderr',owner.stderr,owner.stderrPath]]){
  const occurrence={channel,bytes,file};owner.captures.push(occurrence);
  try{await writeFile(file,bytes,{flag:'wx'});occurrence.completed=true;}
  catch(original){occurrence.original=original;retain(original);}
 }
 try{await heldInputs.revalidate();}catch(original){retain(original);}
 if(owner.originals.length)throw owner.originals[0];
 return owner.stdout;
}
