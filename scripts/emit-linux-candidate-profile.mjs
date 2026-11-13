import {open} from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {emitCandidateProfileSources} from './candidate-profile-source.mjs';

const owner={primaryPresent:false,primaryOriginal:null,closeOriginals:[]};
async function produce(){

// The genuine external attempt owner admits these originals, complete resource
// costs and exact command before invocation. This caller only produces paired
// representations; file/hash/JSON/shape checks issue no source or finance grant.
const paths=process.argv.slice(2);
if(paths.length!==5||paths.some(value=>!path.isAbsolute(value)||value.includes('\0'))||new Set(paths.map(value=>path.resolve(value))).size!==5)throw new Error('distinct absolute paired source paths required');
const inputs=[],outputs=[],closeOriginals=owner.closeOriginals;
owner.inputs=inputs;owner.outputs=outputs;
let association;
const digest=bytes=>createHash('sha256').update(bytes).digest('hex');
const same=(a,b)=>a.dev===b.dev&&a.ino===b.ino;
const stable=(a,b)=>same(a,b)&&a.size===b.size&&a.mtimeNs===b.mtimeNs&&a.ctimeNs===b.ctimeNs;
async function readOriginal(handle,expected,row){
 const occurrence={handle};row.reads.push(occurrence);
 try{
 const before=await handle.stat({bigint:true});occurrence.before=before;
 if(!before.isFile()||before.size<=0n||before.size>BigInt(Number.MAX_SAFE_INTEGER))throw new Error('paired source regular extent required');
 // Caller finances the exact input/readback envelope beforehand. These native
 // lengths select reads only; they do not supply allocation authority.
 const bytes=Buffer.alloc(Number(before.size));occurrence.bytes=bytes;
 const returned=await handle.read(bytes,0,bytes.length,0);occurrence.returned=returned;
 const after=await handle.stat({bigint:true});occurrence.after=after;
 if(returned.bytesRead!==bytes.length||!stable(before,after)||(expected!==null&&!bytes.equals(expected)))throw new Error('paired source original readback changed');
 return occurrence;
 }catch(original){occurrence.primaryOriginal=original;throw original;}
}
try {
 for(const file of paths.slice(0,3)){
  const handle=await open(file,'r');
  const row={handle,path:file,reads:[]};inputs.push(row);
  const read=await readOriginal(handle,null,row);Object.assign(row,read);
  if(inputs.slice(0,-1).some(prior=>same(prior.after,row.after)))throw new Error('paired source input inode alias');
 }
 const decode=bytes=>JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(bytes));
 const generated=emitCandidateProfileSources(inputs[0].bytes,decode(inputs[1].bytes),decode(inputs[2].bytes));
 const sources=[Buffer.from(generated.goSource,'utf8'),Buffer.from(generated.cSource,'utf8')];
 owner.generated=generated;owner.sources=sources;
 for(let index=0;index<2;index++){
  const handle=await open(paths[index+3],'wx+');
  const row={handle,path:paths[index+3],reads:[]};outputs.push(row);
  const initial=await handle.stat({bigint:true});
  if(!initial.isFile()||initial.size!==0n||inputs.some(input=>same(input.after,initial))||outputs.slice(0,-1).some(prior=>same(prior.after,initial)))throw new Error('paired output inode alias');
  row.write={source:sources[index],attempted:true};
  try{await handle.writeFile(sources[index]);row.write.completed=true;}catch(original){row.write.original=original;throw original;}
  row.sync={attempted:true};
  try{await handle.sync();row.sync.completed=true;}catch(original){row.sync.original=original;throw original;}
  const read=await readOriginal(handle,sources[index],row);Object.assign(row,read);
 }
 for(const row of inputs){const final=await readOriginal(row.handle,row.bytes,row);if(!stable(row.before,final.after))throw new Error('paired source original changed during emission');}
 for(let index=0;index<outputs.length;index++){const final=await readOriginal(outputs[index].handle,sources[index],outputs[index]);if(!stable(outputs[index].before,final.after))throw new Error('paired output changed after readback');}
 // Private build association only. The caller retains this original result and
 // all failed prefixes; stdout must never be treated as release/native proof.
 association={kind:'linux-paired-source-association',
  inputs:inputs.map(row=>({size:row.bytes.length,sha256:digest(row.bytes)})),
  outputs:outputs.map(row=>({size:row.bytes.length,sha256:digest(row.bytes)})),
  nativeEntrySupplied:false,authenticatedIssuer:false};
} catch(original){
 owner.primaryPresent=true;owner.primaryOriginal=original;
 process.exitCode=1;
} finally {
 // Attempt each own handle close once even after a failed peer close. Preserve
 // original primary failure; close failures become separately retained causes.
 for(const row of [...outputs,...inputs].reverse()){
  row.closeAttempted=true;
  try{await row.handle.close();row.closeCompleted=true;}catch(original){row.closeOriginal=original;closeOriginals.push({row,original});}
 }
 // These invocation-local originals are no independent late-custody proof.
 // An existing primary is retained unchanged, including primitive throw values.
 if(closeOriginals.length)process.exitCode=1;
}
if(!process.exitCode)process.stdout.write(JSON.stringify(association)+'\n');
}
try{await produce();}catch(original){owner.primaryPresent=true;owner.primaryOriginal=original;process.exitCode=1;}
if(process.exitCode)process.stderr.write('Paired candidate source emission failed; original failure stays private.\n');
