// Synthetic image/receipt/provider bytes exercise source contracts only. No
// compiled entry, native issuer/family, actual publication or Core acceptance.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile,writeFile,readdir,rm,lstat} from 'node:fs/promises';
import path from 'node:path';
import {gzipSync,gunzipSync} from 'node:zlib';
import {fixture,artifactZIP,IDENTITY,RUN,SOURCE,VERSION} from './scoped-candidate-fixture.mjs';
import {assembleScopedCandidate} from './assemble-scoped-candidate.mjs';
import {publishCandidate} from './publish-scoped-candidate.mjs';
import {assertManifest,assertActualLocalAssets,verifyDraftAssetBytes} from './verify-scoped-candidate-publication.mjs';
import {assertNativeWrapper,hash} from './scoped-native-contract.mjs';
import {verifyNativeOwnerArchive,createNativeOwnerArchive,assertCoreNativeOwnerArchiveSize,NATIVE_OWNER_CORE_ARCHIVE_LIMIT} from './native-owner-package.mjs';
import {produceNative} from './produce-scoped-native.mjs';

async function usingFixture(body){
 const f=await fixture({nativeVersion:2});const failures=[];
 try{await body(f);}catch(original){failures.push(original);}
 finally{try{await rm(f.root,{recursive:true});}catch(original){failures.push(original);}}
 if(failures.length)throw new AggregateError(failures,'pair source fixture or cleanup failed');
}
async function assemble(f){
 f.assets=path.join(f.root,'package','release-assets');f.evidence=path.join(f.root,'package','evidence');
 await assembleScopedCandidate({nativeDirectory:f.native,assetDirectory:f.assets,evidenceDirectory:f.evidence,
  identity:IDENTITY,run:RUN,token:'synthetic-source-fixture',fetchImpl:f.fetchImpl});
 return JSON.parse(await readFile(path.join(f.assets,'candidate-manifest.json'),'utf8'));
}
async function rebind(row){
 const name=`native-${row.platform}.json`;
 row.files[name]=Buffer.from(JSON.stringify(row.wrapper)+'\n');
 row.zip=artifactZIP(row.files);row.artifact.digest=`sha256:${hash(row.zip)}`;
 for(const [file,bytes] of Object.entries(row.files))await writeFile(path.join(row.directory,file),bytes);
}
test('v2 pair source producer through schema4 assembler and protected byte readbacks retains both bodies',async()=>{
 await usingFixture(async f=>{
  const manifest=await assemble(f);assert.equal(manifest.schemaVersion,4);
  assert.equal((await readdir(f.assets)).length,5);
  for(const asset of manifest.assets){
   assert.deepEqual(asset.members.map(member=>member.role),['native-owner','go-payload']);
   const held=verifyNativeOwnerArchive(asset.platform,await readFile(path.join(f.assets,asset.name)));
   for(const [index,member] of held.entries())assert.equal(hash(member.bytes),asset.members[index].sha256);
  }
  const result=await publishCandidate({assetDirectory:f.assets,nativeDirectory:f.native,evidenceDirectory:f.evidence,
   journalDirectory:path.join(f.root,'journal'),identity:IDENTITY,run:RUN,token:'synthetic-source-fixture',fetchImpl:f.fetchImpl});
  assert.equal(result.receipt.publication.assets.length,4);
  assert.deepEqual(f.mutations,['tag','draft','upload','upload','upload','upload','publish']);
 });
});
for(const [label,mutate] of [
 ['missing member',value=>value.members.pop()],['reordered members',value=>value.members.reverse()],
 ['member path escape',value=>value.members[1].path='../payload'],
 ['member role substitution',value=>value.members[1].role='native-owner'],
 ['member digest substitution',value=>value.members[1].sha256='f'.repeat(64)],
 ['member size substitution',value=>value.members[1].size++],
 ['extra wrapper field',value=>value.unqualifiedGrant=true],
 ['schema downgrade with pair inventory',value=>value.schema='service-lasso.tui-native-candidate.v1'],
])test(`v2 actual aggregate rejects ${label} with rebound raw wrapper/provider bytes`,async()=>{
 await usingFixture(async f=>{
  mutate(f.objects[0].wrapper);await rebind(f.objects[0]);const before=f.calls.length;
  await assert.rejects(assemble(f));assert.equal(f.calls.length,before);assert.deepEqual(f.mutations,[]);
 });
});
test('mixed v1/v2 native wrappers cannot produce a schema4 or schema3 candidate',async()=>{
 await usingFixture(async f=>{
  const legacy=await fixture();
  try{
   const row=f.objects[0],old=legacy.objects[0];row.wrapper=old.wrapper;row.files=old.files;await rebind(row);
   await assert.rejects(assemble(f));assert.deepEqual(f.mutations,[]);
  }finally{await rm(legacy.root,{recursive:true});}
 });
});
for(const platform of ['linux','win32'])test(`${platform} coherent outer hashes cannot hide changed private member bytes`,async()=>{
 await usingFixture(async f=>{
  const row=f.objects.find(row=>row.platform===platform),archiveName=row.wrapper.archive.name;
  const members=verifyNativeOwnerArchive(`${platform}-amd64`,row.files[archiveName]).map(member=>({...member,bytes:Buffer.from(member.bytes)}));
  members[1].bytes[members[1].bytes.length-1]^=1;
  row.files[archiveName]=createNativeOwnerArchive(`${platform}-amd64`,members);
  row.wrapper.archive={name:archiveName,sha256:hash(row.files[archiveName]),size:row.files[archiveName].length};
  await rebind(row);await assert.rejects(assemble(f));assert.deepEqual(f.mutations,[]);
 });
});
test('actual two-member archive with path escape rejects despite coherent outer references',async()=>{
 await usingFixture(async f=>{
  const row=f.objects.find(row=>row.platform==='linux'),name=row.wrapper.archive.name;
  const raw=gunzipSync(row.files[name]);raw.fill(0,1024,1124);raw.write('../payload',1024);
  raw.fill(32,1024+148,1024+156);
  const checksum=raw.subarray(1024,1536).reduce((sum,byte)=>sum+byte,0);
  raw.write(checksum.toString(8).padStart(6,'0')+'\0 ',1024+148,8,'ascii');
  row.files[name]=gzipSync(raw);row.wrapper.archive={name,sha256:hash(row.files[name]),size:row.files[name].length};
  await rebind(row);await assert.rejects(assemble(f));
 });
});
test('schema4 caps and downgrade deny without huge allocation or fake native proof',async()=>{
 assert.equal(assertCoreNativeOwnerArchiveSize(NATIVE_OWNER_CORE_ARCHIVE_LIMIT),NATIVE_OWNER_CORE_ARCHIVE_LIMIT);
 for(const size of [0,-1,1.5,NATIVE_OWNER_CORE_ARCHIVE_LIMIT+1,Number.MAX_SAFE_INTEGER,NaN])assert.throws(()=>assertCoreNativeOwnerArchiveSize(size));
 await usingFixture(async f=>{
  const manifest=await assemble(f);
  for(const mutate of [value=>value.assets[0].members[0].size=256*1024*1024+1,
   value=>{for(const member of value.assets[0].members)member.size=128*1024*1024+1;},
   value=>value.schemaVersion=3,value=>value.assets[0].members.reverse()]){
   const changed=structuredClone(manifest);mutate(changed);assert.throws(()=>assertManifest(changed,IDENTITY));
  }
  const changed=structuredClone(manifest);changed.assets[0].members[1].sha256='f'.repeat(64);
  assertManifest(changed,IDENTITY);
  const inventory=JSON.parse(await readFile(path.join(f.assets,'candidate-local-assets.json'),'utf8'));
  await assert.rejects(assertActualLocalAssets({assetDirectory:f.assets,manifest:changed,localAssets:inventory}));
  const over=structuredClone(inventory);over[manifest.assets[0].name].size=NATIVE_OWNER_CORE_ARCHIVE_LIMIT+1;
  await assert.rejects(assertActualLocalAssets({assetDirectory:f.assets,manifest,localAssets:over}));
 });
});
test('new-production request cannot fallback to Go-only pipeline or accept supplied authority shapes',async()=>{
 await usingFixture(async f=>{
  const output=path.join(f.root,'never-created');let contacted=false;
  await assert.rejects(produceNative({root:f.root,output,version:VERSION,entryReady:true,familyReady:true,
   fetchImpl:async()=>{contacted=true;throw new Error('unexpected provider call');}}));
  assert.equal(contacted,false);await assert.rejects(lstat(output),{code:'ENOENT'});
  for(const row of f.objects)assertNativeWrapper(row.wrapper,{source:SOURCE,run:RUN,version:VERSION,platform:row.platform,bodies:Object.fromEntries(Object.entries(row.files).filter(([name])=>name!==`native-${row.platform}.json`))});
 });
});
test('draft original readback rejects coherent archive hashes with a mismatched payload member inventory',async()=>{
 await usingFixture(async f=>{
  const manifest=await assemble(f),held={};
  for(const name of ['SHA256SUMS.txt','candidate-manifest.json',...manifest.assets.map(asset=>asset.name)])held[name]=await readFile(path.join(f.assets,name));
  const asset=manifest.assets.find(asset=>asset.platform==='win32-amd64');
  const members=verifyNativeOwnerArchive(asset.platform,held[asset.name]).map(member=>({...member,bytes:Buffer.from(member.bytes)}));
  members[1].bytes[members[1].bytes.length-1]^=1;
  held[asset.name]=createNativeOwnerArchive(asset.platform,members);asset.sha256=hash(held[asset.name]);
  held['SHA256SUMS.txt']=Buffer.from(manifest.assets.map(asset=>`${asset.sha256}  ${asset.name}\n`).join(''));
  manifest.checksumManifest.sha256=hash(held['SHA256SUMS.txt']);held['candidate-manifest.json']=Buffer.from(JSON.stringify(manifest)+'\n');
  assertManifest(manifest,IDENTITY); // valid shape, deliberately invalid member-byte join
  const inventory=Object.fromEntries(Object.entries(held).map(([name,bytes])=>[name,{sha256:hash(bytes),size:bytes.length}]));
  const release={tag_name:IDENTITY.tag,target_commitish:SOURCE.commit,draft:true,prerelease:true,assets:Object.keys(held).sort().map((name,index)=>({
   id:index+1,name,size:held[name].length,digest:`sha256:${hash(held[name])}`,
   url:`https://api.github.com/repos/service-lasso/service-lasso-tui/releases/assets/${index+1}`,
   browser_download_url:`https://github.com/service-lasso/service-lasso-tui/releases/download/${IDENTITY.tag}/${name}`}))};
  let fetched=false;
  const fetchImpl=async url=>{
   const row=release.assets.find(row=>row.url===String(url));assert.ok(row);
   if(row.name===asset.name)fetched=true;
   return {status:200,headers:{get:()=>null},body:(async function*(){yield held[row.name];})()};
  };
  await assert.rejects(verifyDraftAssetBytes({release,manifest,localAssets:inventory,heldBytes:held,
   downloadDir:path.join(f.root,'draft-readback'),token:'synthetic-source-fixture',fetchImpl}));
  assert.equal(fetched,true);await assert.rejects(lstat(path.join(f.root,'draft-readback')),{code:'ENOENT'});
 });
});
