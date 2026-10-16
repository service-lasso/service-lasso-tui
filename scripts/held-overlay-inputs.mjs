import {open} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import path from 'node:path';

const identity=(a,b)=>a.dev===b.dev&&a.ino===b.ino;
const stable=(a,b)=>identity(a,b)&&a.size===b.size&&a.mtimeNs===b.mtimeNs&&a.ctimeNs===b.ctimeNs;
export class HeldOverlayInputs {
 constructor(){this.rows=[];this.closeOriginals=[];}
 async read(handle,owner){
  const occurrence={handle};owner.reads.push(occurrence);
  try{
  const before=await handle.stat({bigint:true});occurrence.before=before;
  if(!before.isFile()||before.size>BigInt(Number.MAX_SAFE_INTEGER))throw new Error('held input extent');
  const bytes=Buffer.alloc(Number(before.size));occurrence.bytes=bytes;
  const returned=await handle.read(bytes,0,bytes.length,0);occurrence.returned=returned;
  const after=await handle.stat({bigint:true});occurrence.after=after;
  if(returned.bytesRead!==bytes.length||!stable(before,after))throw new Error('held input changed');
  return occurrence;
  }catch(original){occurrence.primaryOriginal=original;throw original;}
 }
 async acquire(file){
  const key=path.resolve(file).toLowerCase();
  const previous=this.rows.find(row=>row.key===key);
  if(previous)return previous;
  const handle=await open(file,'r'),row={path:file,key,handle,reads:[]};this.rows.push(row);
  Object.assign(row,await this.read(handle,row));
  row.sha256=createHash('sha256').update(row.bytes).digest('hex');
  return row;
 }
 async revalidate(){
  for(const row of this.rows){
   const current=await this.read(row.handle,row);
   if(!stable(row.before,current.after)||!row.bytes.equals(current.bytes))throw new Error('held input original changed');
   const named=await open(row.path,'r'),occurrence={handle:named,reads:[]};
   if(!row.namedReads)row.namedReads=[];row.namedReads.push(occurrence);
   let failed=false;
   try{
    const currentName=await this.read(named,occurrence);
    Object.assign(occurrence,currentName);
    if(!stable(row.before,currentName.after)||!row.bytes.equals(currentName.bytes))throw new Error('held input pathname rebound');
   }catch(original){failed=true;occurrence.primaryOriginal=original;throw original;}
   finally{occurrence.closeAttempted=true;try{await named.close();occurrence.closeCompleted=true;}catch(original){occurrence.closeOriginal=original;this.closeOriginals.push({row:occurrence,original});if(!failed)throw original;}}
  }
 }
 async close(){
  for(const row of [...this.rows].reverse()){
   if(row.closeAttempted)continue;
   row.closeAttempted=true;
   try{await row.handle.close();row.closeCompleted=true;}catch(original){row.closeOriginal=original;this.closeOriginals.push({row,original});}
  }
  if(this.closeOriginals.length)throw new AggregateError(this.closeOriginals.map(row=>row.original),'held input close failed');
 }
}
export function requireEmittedQueryPair(emitted,profileRow,goRow,headerRow,commit){
 if(emitted.profileSHA256!==profileRow.sha256||emitted.sourceCommit!==commit||emitted.goSHA256!==goRow.sha256||emitted.headerSHA256!==headerRow.sha256)throw new Error('emitted input association differs');
}
