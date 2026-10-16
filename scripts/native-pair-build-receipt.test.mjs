// Synthetic source metadata/images only: no native issuer, compiled entry,
// actual provider publication, reservation or family/closure qualification.
import test from 'node:test';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {writeFile,rm} from 'node:fs/promises';
import path from 'node:path';
import {fixture,artifactZIP,SOURCE,RUN,IDENTITY} from './scoped-candidate-fixture.mjs';
import {assertNativeBuildOutput} from './native-pair-build-receipt.mjs';
import {verifyHeldNativePublicReceipt} from './scoped-native-public.mjs';
import {PUBLIC_RECEIPTS,hash} from './scoped-native-contract.mjs';
import {assembleScopedCandidate} from './assemble-scoped-candidate.mjs';

const cli=fileURLToPath(new URL('./verify-tui20-native-public-receipt.mjs',import.meta.url));
async function usingFixture(body){
 const f=await fixture({nativeVersion:2});const failures=[];
 try{await body(f);}catch(original){failures.push(original);}
 finally{try{await rm(f.root,{recursive:true});}catch(original){failures.push(original);}}
 if(failures.length)throw new AggregateError(failures,'pair receipt fixture or cleanup failed');
}
async function rebind(row,build){
 row.files['build-output.json']=Buffer.from(JSON.stringify(build)+'\n');
 row.wrapper.receipts=PUBLIC_RECEIPTS.map(name=>({name,sha256:hash(row.files[name]),size:row.files[name].length}));
 row.files[`native-${row.platform}.json`]=Buffer.from(JSON.stringify(row.wrapper)+'\n');
 row.zip=artifactZIP(row.files);row.artifact.digest=`sha256:${hash(row.zip)}`;
 for(const [name,bytes] of Object.entries(row.files))await writeFile(path.join(row.directory,name),bytes);
}
test('both public validator surfaces accept the same closed schema2 record, retaining legacy schema1',async()=>{
 await usingFixture(async f=>{
  for(const row of f.objects){
   const publicFiles=Object.fromEntries(PUBLIC_RECEIPTS.map(name=>[name,row.files[name]]));
   const result=verifyHeldNativePublicReceipt(publicFiles);assert.equal(result.build.schemaVersion,2);
   assert.deepEqual(result.build.run,RUN);assert.equal(assertNativeBuildOutput(result.build),result.build);
   const actual=spawnSync(process.execPath,[cli,row.directory],{encoding:'utf8'});
   assert.equal(actual.status,0,actual.stderr);assert.equal(actual.signal,null);
  }
  const legacy=await fixture();
  try{for(const row of legacy.objects)assert.equal(verifyHeldNativePublicReceipt(Object.fromEntries(PUBLIC_RECEIPTS.map(name=>[name,row.files[name]]))).build.schemaVersion,1);}
  finally{await rm(legacy.root,{recursive:true});}
 });
});
for(const [label,mutate] of [
 ['missing run',value=>delete value.run],['unknown run field',value=>value.run.grant=true],
 ['invalid run identity',value=>value.run.id=0],['invalid attempt',value=>value.run.attempt=1.5],
 ['foreign workflow source',value=>value.run.workflowSha='b'.repeat(40)],
 ['missing member',value=>value.members.pop()],['reordered member',value=>value.members.reverse()],
 ['duplicate role',value=>value.members[1].role='native-owner'],
 ['foreign role',value=>value.members[1].role='other'],['unknown member field',value=>value.members[1].permission=true],
 ['invalid digest',value=>value.members[1].sha256='x'.repeat(64)],
 ['oversize member',value=>value.members[1].size=256*1024*1024+1],
 ['oversize aggregate',value=>{value.members[0].size=128*1024*1024+1;value.members[1].size=128*1024*1024+1;value.nativeBinary.size=value.members[0].size;}],
 ['public binary mismatch',value=>value.nativeBinary.sha256='f'.repeat(64)],
 ['version downgrade',value=>value.schemaVersion=1],['extra field',value=>value.permission=true]
])test(`both public receipt validator surfaces reject ${label}`,async()=>{
 await usingFixture(async f=>{
  const row=f.objects[0],build=JSON.parse(row.files['build-output.json']);mutate(build);
  row.files['build-output.json']=Buffer.from(JSON.stringify(build));await writeFile(path.join(row.directory,'build-output.json'),row.files['build-output.json']);
  assert.throws(()=>verifyHeldNativePublicReceipt(Object.fromEntries(PUBLIC_RECEIPTS.map(name=>[name,row.files[name]]))));
  const actual=spawnSync(process.execPath,[cli,row.directory],{encoding:'utf8'});
  assert.notEqual(actual.status,0);assert.equal(actual.signal,null);
 });
});
for(const [label,mutate] of [
 ['prior run',value=>value.run.id--],['prior attempt',value=>value.run.attempt--],
 ['private payload substitution',value=>value.members[1].sha256='f'.repeat(64)],
 ['private payload size substitution',value=>value.members[1].size++],
 ['old Go-only receipt',value=>{value.schemaVersion=1;delete value.run;delete value.members;}]
])test(`actual aggregate rejects ${label} despite coherent raw receipt/wrapper/provider refs`,async()=>{
 await usingFixture(async f=>{
  const row=f.objects[0],build=JSON.parse(row.files['build-output.json']);mutate(build);assertNativeBuildOutput(build);
  await rebind(row,build);const before=f.calls.length;
  await assert.rejects(assembleScopedCandidate({nativeDirectory:f.native,assetDirectory:path.join(f.root,'assets'),
   evidenceDirectory:path.join(f.root,'evidence'),identity:IDENTITY,run:RUN,token:'synthetic-fixture',fetchImpl:f.fetchImpl}));
  assert.equal(f.calls.length,before);assert.deepEqual(f.mutations,[]);
 });
});
test('actual enclosing run is acquired from all original env fields, never SHA/ref defaults',async()=>{
 await usingFixture(async f=>{
  const row=f.objects.find(row=>row.platform==='linux');
  const owner=path.join(f.root,'owner-image'),payload=path.join(f.root,'payload-image');
  const {verifyNativeOwnerArchive}=await import('./native-owner-package.mjs');
  const members=verifyNativeOwnerArchive('linux-amd64',row.files[row.wrapper.archive.name]);
  await writeFile(owner,members[0].bytes);await writeFile(payload,members[1].bytes);
  const moduleURL=new URL('./native-pair-build-receipt.mjs',import.meta.url).href;
  const script=path.join(f.root,'writer.mjs');
  await writeFile(script,`import {open} from 'node:fs/promises';import {createEnclosingPairBuildOutput} from ${JSON.stringify(moduleURL)};
const handles=[];try{const members=[];for(const [index,file] of process.argv.slice(2).entries()){const handle=await open(file,'r');handles.push(handle);members.push({path:index?'.service-lasso-tui/service-lasso-tui-go':'service-lasso-tui',role:index?'go-payload':'native-owner',bytes:await handle.readFile()});}
const result=createEnclosingPairBuildOutput({expectedSource:${JSON.stringify(SOURCE)},expectedRun:${JSON.stringify(RUN)},platform:'linux-amd64',members});process.stdout.write(JSON.stringify(result.record));}finally{for(const handle of handles.reverse())await handle.close();}`);
  const env={...process.env,GITHUB_REPOSITORY:SOURCE.repository,GITHUB_SHA:SOURCE.commit,GITHUB_REF:SOURCE.ref,GITHUB_RUN_ID:String(RUN.id),GITHUB_RUN_ATTEMPT:String(RUN.attempt)};
  const actual=spawnSync(process.execPath,[script,owner,payload],{env,encoding:'utf8'});
  assert.equal(actual.status,0,actual.stderr);assert.deepEqual(JSON.parse(actual.stdout).run,RUN);
  for(const missing of ['GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT']){
   const incomplete={...env};delete incomplete[missing];
   const refused=spawnSync(process.execPath,[script,owner,payload],{env:incomplete,encoding:'utf8'});
   assert.notEqual(refused.status,0);assert.equal(refused.stdout,'');
  }
  const prior=spawnSync(process.execPath,[script,owner,payload],{env:{...env,GITHUB_RUN_ATTEMPT:String(RUN.attempt-1)},encoding:'utf8'});
  assert.notEqual(prior.status,0);assert.equal(prior.stdout,'');
 });
});
test('legacy v1 rejects a schema2 build original even with coherent raw public receipt references',async()=>{
 await usingFixture(async pair=>{
  const legacy=await fixture();
  try{
   await rebind(legacy.objects[0],JSON.parse(pair.objects[0].files['build-output.json']));
   const before=legacy.calls.length;
   await assert.rejects(assembleScopedCandidate({nativeDirectory:legacy.native,assetDirectory:path.join(legacy.root,'assets'),
    evidenceDirectory:path.join(legacy.root,'evidence'),identity:IDENTITY,run:RUN,token:'synthetic-fixture',fetchImpl:legacy.fetchImpl}));
   assert.equal(legacy.calls.length,before);assert.deepEqual(legacy.mutations,[]);
  }finally{await rm(legacy.root,{recursive:true});}
 });
});
test('coherent foreign inner source/run/receipt originals cannot replace the actual candidate source',async()=>{
 await usingFixture(async f=>{
  const row=f.objects[0],foreign='b'.repeat(40),build=JSON.parse(row.files['build-output.json']);
  build.tuiCommit=foreign;build.run.workflowSha=foreign;
  for(const name of ['input-custody.json','native-public-projection.json']){
   const value=JSON.parse(row.files[name]);
   if(name==='input-custody.json')value.source.tuiCommit=foreign;else value.sourceCommit=foreign;
   row.files[name]=Buffer.from(JSON.stringify(value)+'\n');
  }
  await rebind(row,build);
  assert.equal(verifyHeldNativePublicReceipt(Object.fromEntries(PUBLIC_RECEIPTS.map(name=>[name,row.files[name]]))).build.tuiCommit,foreign);
  const before=f.calls.length;
  await assert.rejects(assembleScopedCandidate({nativeDirectory:f.native,assetDirectory:path.join(f.root,'assets'),
   evidenceDirectory:path.join(f.root,'evidence'),identity:IDENTITY,run:RUN,token:'synthetic-fixture',fetchImpl:f.fetchImpl}));
  assert.equal(f.calls.length,before);assert.deepEqual(f.mutations,[]);
 });
});
