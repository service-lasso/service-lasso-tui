import test from "node:test";
import assert from "node:assert/strict";
import { readFile, writeFile, readdir } from "node:fs/promises";
import path from "node:path";
import { fixture, IDENTITY, RUN, SOURCE, VERSION } from "./scoped-candidate-fixture.mjs";
import { assembleScopedCandidate } from "./assemble-scoped-candidate.mjs";
import { publishCandidate } from "./publish-scoped-candidate.mjs";
import { assertManifest, assertActualLocalAssets } from "./verify-scoped-candidate-publication.mjs";
import { assertManifest as legacyManifest } from "./verify-candidate-publication.mjs";
import { SCOPE, hash } from "./scoped-native-contract.mjs";
import { parseStrictJSON } from "./scoped-json.mjs";
const assemble=async f=>{const assets=path.join(f.root,"package","release-assets"),evidence=path.join(f.root,"package","evidence");await assembleScopedCandidate({nativeDirectory:f.native,assetDirectory:assets,evidenceDirectory:evidence,identity:IDENTITY,run:RUN,token:"fixture-authority",fetchImpl:f.fetchImpl});return {...f,assets,evidence};};
const publish=f=>publishCandidate({assetDirectory:f.assets,nativeDirectory:f.native,evidenceDirectory:f.evidence,journalDirectory:path.join(f.root,"private-journal"),identity:IDENTITY,run:RUN,token:"fixture-authority",fetchImpl:f.fetchImpl});
test("actual scoped aggregate and protected publisher preserve four original public byte receipts",async()=>{
  const f=await assemble(await fixture());assert.equal((await readdir(f.assets)).length,5);const sums=await readFile(path.join(f.assets,"SHA256SUMS.txt"),"utf8");assert.equal(sums.trimEnd().split("\n").length,2);assert.match(sums.split("\n")[0],/-linux-amd64.tar.gz$/u);
  const result=await publish(f);assert.equal(result.receipt.schema,"service-lasso.tui-publication-evidence.v1");assert.equal(result.receipt.publication.assets.length,4);assert.deepEqual(f.mutations,["tag","draft","upload","upload","upload","upload","publish"]);assert.equal(result.receipt.candidate.sha256,hash(await readFile(path.join(f.assets,"candidate-manifest.json"))));assert.equal(result.receipt.publication.immutable,true);
});
for(const [name,mutate] of [
  ["missing native job",f=>f.jobs.splice(f.jobs.findIndex(job=>job.name==="native-candidate (linux)"),1)],
  ["cancelled required job",f=>f.jobs.find(job=>job.name==="native-candidate (win32)").conclusion="cancelled"],
  ["skipped required job",f=>f.jobs.find(job=>job.name==="native-candidate (win32)").conclusion="skipped"],
  ["prior attempt",f=>f.jobs.find(job=>job.name==="native-candidate (win32)").run_attempt=1],
  ["wrong workflow source",f=>f.jobs.find(job=>job.name==="native-candidate (linux)").head_sha="b".repeat(40)],
  ["extra Darwin artifact",f=>f.objects.push({...f.objects[0],artifact:{...f.objects[0].artifact,id:777,name:`scoped-native-darwin-${RUN.id}-${RUN.attempt}`}})],
  ["expired artifact",f=>f.objects[1].artifact.expired=true],
  ["artifact source replacement",f=>f.objects[1].artifact.workflow_run.head_sha="b".repeat(40)],
  ["artifact raw byte replacement",f=>f.objects[1].zip=Buffer.from("altered")],
  ["unsupported architecture job",f=>f.jobs.find(job=>job.name==="native-candidate (win32)").labels=["windows-arm64"]]
])test(`actual aggregate denies ${name} before candidate construction`,async()=>{const f=await fixture();mutate(f);await assert.rejects(assemble(f));assert.deepEqual(f.mutations,[]);});
for(const [name,mutate] of [
  ["wrong policy",manifest=>manifest.scope={...SCOPE,policySha256:"f".repeat(64)}],
  ["missing Windows",manifest=>manifest.assets.pop()],
  ["reordered archives",manifest=>manifest.assets.reverse()],
  ["schema downgrade",manifest=>manifest.schemaVersion=2],
  ["architecture substitution",manifest=>manifest.assets[0].platform="linux-arm64"],
  ["unknown outer key",manifest=>manifest.fixtureCatalogAdmission=true]
])test(`actual publisher denies ${name} before provider contact`,async()=>{const f=await assemble(await fixture());const manifest=parseStrictJSON(await readFile(path.join(f.assets,"candidate-manifest.json")));mutate(manifest);await writeFile(path.join(f.assets,"candidate-manifest.json"),JSON.stringify(manifest));const before=f.calls.length;await assert.rejects(publish(f));assert.equal(f.calls.length,before);assert.deepEqual(f.mutations,[]);});
test("publisher denies coherent manifest and inventory replacement at actual held acquisition",async()=>{
  const f=await assemble(await fixture());const before=f.calls.length;
  await assert.rejects(publishCandidate({assetDirectory:f.assets,nativeDirectory:f.native,evidenceDirectory:f.evidence,journalDirectory:path.join(f.root,"journal"),identity:IDENTITY,run:RUN,token:"fixture-authority",fetchImpl:f.fetchImpl,beforeHeldAcquisition:async()=>{
    const file=path.join(f.assets,"candidate-manifest.json");const value=parseStrictJSON(await readFile(file));const bytes=Buffer.from(JSON.stringify({...value,source:{ref:value.source.ref,repository:value.source.repository,commit:value.source.commit}})+"\n");await writeFile(file,bytes);const inventory=parseStrictJSON(await readFile(path.join(f.assets,"candidate-local-assets.json")));inventory["candidate-manifest.json"]={sha256:hash(bytes),size:bytes.length};await writeFile(path.join(f.assets,"candidate-local-assets.json"),JSON.stringify(inventory));
  }}));assert.equal(f.calls.length,before);
});
for(const variant of ["manifest-row","duplicate","missing","reordered","wrong-hash"])
 test(`actual local verifier denies ${variant} checksum list`,async()=>{
  const f=await assemble(await fixture()),file=path.join(f.assets,"SHA256SUMS.txt");let rows=(await readFile(file,"utf8")).trimEnd().split("\n");if(variant==="manifest-row")rows.push(`${"f".repeat(64)}  candidate-manifest.json`);if(variant==="duplicate")rows[1]=rows[0];if(variant==="missing")rows.pop();if(variant==="reordered")rows.reverse();if(variant==="wrong-hash")rows[0]=`${"f".repeat(64)}${rows[0].slice(64)}`;
  const bytes=Buffer.from(rows.join("\n")+"\n");await writeFile(file,bytes);const manifest=parseStrictJSON(await readFile(path.join(f.assets,"candidate-manifest.json")));manifest.checksumManifest.sha256=hash(bytes);const manifestBytes=Buffer.from(JSON.stringify(manifest));await writeFile(path.join(f.assets,"candidate-manifest.json"),manifestBytes);const inventory=parseStrictJSON(await readFile(path.join(f.assets,"candidate-local-assets.json")));inventory["SHA256SUMS.txt"]={sha256:hash(bytes),size:bytes.length};inventory["candidate-manifest.json"]={sha256:hash(manifestBytes),size:manifestBytes.length};await assert.rejects(assertActualLocalAssets({assetDirectory:f.assets,manifest,localAssets:inventory}));
 });
test("nested duplicate keys deny before JSON parsing can overwrite scope",()=>assert.throws(()=>parseStrictJSON('{"scope":{"policyId":"one","policyId":"two"}}')));
test("historical v2 exact four-target manifest stays readable but never scoped",()=>{
 const assets=["darwin-amd64","darwin-arm64","linux-amd64","win32-amd64"].map(platform=>({platform,name:`service-lasso-tui-${VERSION}-${platform}.${platform==="win32-amd64"?"zip":"tar.gz"}`,executable:platform==="win32-amd64"?"service-lasso-tui.exe":"service-lasso-tui",sha256:"f".repeat(64)}));
 const manifest={schemaVersion:2,kind:"develop-prerelease-candidate",version:VERSION,source:SOURCE,release:{tag:IDENTITY.tag,draft:false,prerelease:true,immutable:true},corePackagingIssue:"service-lasso/service-lasso#1461",assets,checksumManifest:{name:"SHA256SUMS.txt",sha256:"f".repeat(64)}};assert.equal(legacyManifest(manifest,IDENTITY).schemaVersion,2);assert.throws(()=>assertManifest(manifest,IDENTITY));
});
test("actual publisher denies substituted final public manifest body",async()=>{
 const f=await assemble(await fixture()),original=f.fetchImpl;f.fetchImpl=async(url,options)=>{if(String(url).startsWith("https://github.com/")&&String(url).endsWith("candidate-manifest.json"))return {status:200,headers:{get:()=>null},body:(async function*(){yield Buffer.alloc((await readFile(path.join(f.assets,"candidate-manifest.json"))).length,32);})()};return original(url,options);};await assert.rejects(publish(f));assert.equal(f.mutations.at(-1),"publish");
});
test("publisher denies failed actual aggregate even with coherent qualified fixture bytes",async()=>{const f=await assemble(await fixture());f.jobs.find(job=>job.name==="aggregate").conclusion="failure";await assert.rejects(publish(f));assert.deepEqual(f.mutations,[]);});

test("historical actual local verifier retains six public files and four archive-only Map rows",async()=>{
 const {assertActualLocalAssets:legacyLocal}=await import("./verify-candidate-publication.mjs");const f=await fixture();const directory=path.join(f.root,"historical");await (await import("node:fs/promises")).mkdir(directory);
 const assets=[];const buffers={};for(const platform of ["darwin-amd64","darwin-arm64","linux-amd64","win32-amd64"]){const name=`service-lasso-tui-${VERSION}-${platform}.${platform==="win32-amd64"?"zip":"tar.gz"}`;buffers[name]=Buffer.from(`historical archive bytes ${platform}`);assets.push({platform,name,executable:platform==="win32-amd64"?"service-lasso-tui.exe":"service-lasso-tui",sha256:hash(buffers[name])});}
 buffers["SHA256SUMS.txt"]=Buffer.from([...assets].reverse().map(asset=>`${asset.sha256}  ${asset.name}\n`).join(""));const manifest={schemaVersion:2,kind:"develop-prerelease-candidate",version:VERSION,source:SOURCE,release:{tag:IDENTITY.tag,draft:false,prerelease:true,immutable:true},corePackagingIssue:"service-lasso/service-lasso#1461",assets,checksumManifest:{name:"SHA256SUMS.txt",sha256:hash(buffers["SHA256SUMS.txt"])}};buffers["candidate-manifest.json"]=Buffer.from(JSON.stringify(manifest));const inventory={};for(const [name,bytes] of Object.entries(buffers)){await writeFile(path.join(directory,name),bytes);inventory[name]={sha256:hash(bytes),size:bytes.length};}assert.equal(Object.keys(await legacyLocal({assetDirectory:directory,manifest:legacyManifest(manifest,IDENTITY),localAssets:inventory})).length,6);
});
test("actual publisher denies corrupt native ZIP even with coherent manifest and inventory hashes",async()=>{
 const f=await assemble(await fixture()),manifestFile=path.join(f.assets,"candidate-manifest.json"),manifest=parseStrictJSON(await readFile(manifestFile));const asset=manifest.assets.find(row=>row.platform==="win32-amd64");const bytes=await readFile(path.join(f.assets,asset.name));bytes.writeUInt16LE(1,6);asset.sha256=hash(bytes);await writeFile(path.join(f.assets,asset.name),bytes);const manifestBytes=Buffer.from(JSON.stringify(manifest));await writeFile(manifestFile,manifestBytes);const inventory=parseStrictJSON(await readFile(path.join(f.assets,"candidate-local-assets.json")));inventory[asset.name]={sha256:hash(bytes),size:bytes.length};inventory["candidate-manifest.json"]={sha256:hash(manifestBytes),size:manifestBytes.length};await writeFile(path.join(f.assets,"candidate-local-assets.json"),JSON.stringify(inventory));const before=f.calls.length;await assert.rejects(publish(f));assert.equal(f.calls.length,before);
});

for(const [label,change] of [
 ["public manifest policy substitution",value=>value.scope.policySource={...value.scope.policySource,blob:"f".repeat(40)}],
 ["native receipt raw body substitution",value=>value.receipts[0].sha256="f".repeat(64)],
 ["native completed proof failure",value=>value.outcome="failure"],
 ["wrong native source",value=>value.source={...value.source,commit:"b".repeat(40)}],
 ["mixed attempt",value=>value.run={...value.run,attempt:1}],
 ["third platform",value=>value.platform="darwin"]
])test(`actual aggregate denies coherent ${label}`,async()=>{const f=await fixture();const row=f.objects[0];const value=structuredClone(row.wrapper);change(value);await writeFile(path.join(row.directory,"native-win32.json"),JSON.stringify(value));await assert.rejects(assemble(f));assert.deepEqual(f.mutations,[]);});

test("actual aggregate denies structurally valid TAR containing wrong ELF architecture",async()=>{
 const f=await fixture(),row=f.objects[1],name=row.wrapper.archive.name;const {createNativeArchive}=await import("./scoped-native-archive.mjs");const binary=Buffer.alloc(256);binary.write("7f454c46",0,"hex");binary[4]=2;binary[5]=1;binary.writeUInt16LE(183,18);row.files[name]=createNativeArchive("linux-amd64",binary);row.wrapper.archive.sha256=hash(row.files[name]);row.wrapper.archive.size=row.files[name].length;row.wrapper.binary.sha256=hash(binary);row.files["native-linux.json"]=Buffer.from(JSON.stringify(row.wrapper));await writeFile(path.join(row.directory,name),row.files[name]);await writeFile(path.join(row.directory,"native-linux.json"),row.files["native-linux.json"]);await assert.rejects(assemble(f));
});

test("raw JSON denies malformed UTF-8 and BOM before boundary decoding",()=>{assert.throws(()=>parseStrictJSON(Buffer.from([0x7b,0x22,0x61,0x22,0x3a,0x22,0xff,0x22,0x7d])));assert.throws(()=>parseStrictJSON(Buffer.concat([Buffer.from([0xef,0xbb,0xbf]),Buffer.from("{}")])));});
