// Controlled actual file/child failures; no profile/native authority supplied.
import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtemp,writeFile,rename,rm,readFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import {HeldOverlayInputs,requireEmittedQueryPair} from './held-overlay-inputs.mjs';

test('real held borrower rejects substituted pathname and changed original bytes',async()=>{
 for(const mode of ['replace','mutate']){
  const root=await mkdtemp(path.join(tmpdir(),'tui-held-input-')),held=new HeldOverlayInputs();
  try{
   const file=path.join(root,'input'),replacement=path.join(root,'replacement');
   await writeFile(file,'original-source');const row=await held.acquire(file);
   await held.revalidate();
   if(mode==='replace'){await writeFile(replacement,'original-source');await rename(replacement,file);}
   else await writeFile(file,'changed-source!');
   await assert.rejects(held.revalidate());
   assert.equal(row.bytes.toString(),'original-source');
  }finally{await held.close();await rm(root,{recursive:true});}
 }
});
test('actual consumed generated Go/header hashes both rejoin emitter result',()=>{
 const profile={sha256:'profile'},go={sha256:'go'},header={sha256:'header'};
 const emitted={profileSHA256:'profile',goSHA256:'go',headerSHA256:'header',sourceCommit:'commit'};
 requireEmittedQueryPair(emitted,profile,go,header,'commit');
 for(const key of ['profileSHA256','goSHA256','headerSHA256','sourceCommit'])assert.throws(()=>requireEmittedQueryPair({...emitted,[key]:'substitute'},profile,go,header,'commit'));
});
test('actual executor keeps malformed/private and opaque failures out of public output',async()=>{
 const sourceRoot=fileURLToPath(new URL('..',import.meta.url));
 for(const mode of ['malformed','opaque']){
  const root=await mkdtemp(path.join(tmpdir(),'tui-private-error-'));
  try{
   const profile=path.join(root,'PRIVATE-profile'),output=path.join(root,'attempt'),wrapper=path.join(root,'wrapper.mjs');
   await writeFile(profile,'{"PRIVATE-CONTENT":'+ 'PRIVATE-INVALID');
   // Host substitution reaches only the actual profile parse failure. No Go,
   // native provider, emitter or product operation is permitted by this test.
   await writeFile(wrapper,`Object.defineProperty(process,'platform',{value:'win32'});Object.defineProperty(process,'arch',{value:'x64'});process.argv=${JSON.stringify([process.execPath,'fixture',process.execPath,sourceRoot,profile,output])};
${mode==='opaque'?`JSON.parse=()=>{throw Object.defineProperty({},'message',{get(){process.stderr.write('PRIVATE-GETTER-TOUCHED');return 'PRIVATE';}})};`:''}
await import(${JSON.stringify(new URL('./test-windows-owned-overlay.mjs',import.meta.url).href)});`);
   const result=spawnSync(process.execPath,[wrapper],{encoding:'utf8'});
   assert.equal(result.status,1);assert.equal(result.signal,null);
   assert.doesNotMatch(result.stdout+result.stderr,/PRIVATE|SyntaxError|ENOENT/u);
   assert.ok(!result.stdout.includes(root)&&!result.stderr.includes(root));
   const summary=JSON.parse(await readFile(path.join(output,'public-summary.json'),'utf8'));
   assert.equal(summary.state,'incomplete');assert.equal(summary.qualifiedNativeOperatorAcceptance,false);
  }finally{await rm(root,{recursive:true});}
 }
});
