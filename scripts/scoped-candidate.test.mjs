import test from "node:test";
import assert from "node:assert/strict";
import { readFile, writeFile, readdir } from "node:fs/promises";
import path from "node:path";
import { fixture, artifactZIP, IDENTITY, RUN, SOURCE, VERSION } from "./scoped-candidate-fixture.mjs";
import { assembleScopedCandidate } from "./assemble-scoped-candidate.mjs";
import { publishCandidate } from "./publish-scoped-candidate.mjs";
import { assertManifest, assertActualLocalAssets, assertPreflight } from "./verify-scoped-candidate-publication.mjs";
import { assertManifest as legacyManifest } from "./verify-candidate-publication.mjs";
import { SCOPE, PUBLIC_RECEIPTS, hash } from "./scoped-native-contract.mjs";
import { parseStrictJSON } from "./scoped-json.mjs";
const assemble=async(f,options={})=>{const assets=path.join(f.root,"package","release-assets"),evidence=path.join(f.root,"package","evidence");await assembleScopedCandidate({nativeDirectory:f.native,assetDirectory:assets,evidenceDirectory:evidence,identity:IDENTITY,run:RUN,token:"fixture-authority",fetchImpl:f.fetchImpl,...options});return {...f,assets,evidence};};
const publish=(f,options={})=>publishCandidate({assetDirectory:f.assets,nativeDirectory:f.native,evidenceDirectory:f.evidence,journalDirectory:path.join(f.root,"private-journal"),identity:IDENTITY,run:RUN,token:"fixture-authority",fetchImpl:f.fetchImpl,...options});

// Rebind every outer raw reference and the actual provider ZIP to the altered
// inner originals. Only the semantic bridge, rather than stale metadata, can deny.
async function rebindInner(f,row,change){
  const originals=Object.fromEntries(PUBLIC_RECEIPTS.map(name=>[name,Buffer.from(row.files[name])]));
  const records=Object.fromEntries(PUBLIC_RECEIPTS.map(name=>[name,parseStrictJSON(row.files[name])]));
  change(records);
  for(const name of PUBLIC_RECEIPTS)row.files[name]=Buffer.from(JSON.stringify(records[name])+"\n");
  row.wrapper.receipts=PUBLIC_RECEIPTS.map(name=>({name,sha256:hash(row.files[name]),size:row.files[name].length}));
  const wrapperName=`native-${row.platform}.json`;row.files[wrapperName]=Buffer.from(JSON.stringify(row.wrapper)+"\n");
  row.zip=artifactZIP(row.files);row.artifact.digest=`sha256:${hash(row.zip)}`;
  for(const [name,bytes] of Object.entries(row.files))await writeFile(path.join(row.directory,name),bytes);
  if(f.evidence){
    const file=path.join(f.evidence,"qualification.json"),value=parseStrictJSON(await readFile(file));
    const ref=value.receipts.find(item=>item.platform===row.platform);
    ref.sha256=hash(row.files[wrapperName]);ref.size=row.files[wrapperName].length;
    await writeFile(file,JSON.stringify(value)+"\n");
  }
  return originals;
}
const innerChanges=[
  ["coherent foreign inner source",records=>{records["input-custody.json"].source.tuiCommit="d".repeat(40);records["build-output.json"].tuiCommit="d".repeat(40);records["native-public-projection.json"].sourceCommit="d".repeat(40);}],
  ["coherent foreign inner binary digest",records=>{records["binary-digest.json"].sha256="d".repeat(64);records["build-output.json"].nativeBinary.sha256="d".repeat(64);records["native-public-projection.json"].binarySHA256="d".repeat(64);}],
  ["coherent foreign inner binary size",records=>{records["binary-digest.json"].size+=1;records["build-output.json"].nativeBinary.size+=1;}],
  ["coherent foreign inner binary digest and size",records=>{records["binary-digest.json"].sha256="d".repeat(64);records["build-output.json"].nativeBinary={sha256:"d".repeat(64),size:257};records["binary-digest.json"].size=257;records["native-public-projection.json"].binarySHA256="d".repeat(64);}]
];
for(const route of ["aggregate","publisher"])for(const platform of ["win32","linux"])for(const [label,change] of innerChanges)
test(`TUI28-DENIALS actual ${route} denies ${platform} ${label} with matching raw refs and provider ZIP`,async()=>{
  const f=route==="publisher"?await assemble(await fixture()):await fixture();
  const row=f.objects.find(item=>item.platform===platform),outerBinary={...row.wrapper.binary},archive=Buffer.from(row.files[row.wrapper.archive.name]);
  await rebindInner(f,row,change);
  assert.deepEqual(row.wrapper.binary,outerBinary);assert.ok(row.files[row.wrapper.archive.name].equals(archive));
  for(const ref of row.wrapper.receipts){assert.equal(hash(row.files[ref.name]),ref.sha256);assert.equal(row.files[ref.name].length,ref.size);}
  assert.equal(row.artifact.digest,`sha256:${hash(row.zip)}`);
  const before=f.calls.length;await assert.rejects(route==="publisher"?publish(f):assemble(f));
  assert.equal(f.calls.length,before);assert.deepEqual(f.mutations,[]);
});
for(const route of ["aggregate","publisher"])for(const name of PUBLIC_RECEIPTS)
test(`TUI28-DENIALS actual ${route} validates original held ${name} despite valid pathname replacement`,async()=>{
  const f=route==="publisher"?await assemble(await fixture()):await fixture(),row=f.objects[0];
  const originals=await rebindInner(f,row,records=>{records[name].unexpectedHeldKey=true;});
  const held=Buffer.from(row.files[name]);let reached=false;
  const beforeNativeValidation=async({root,platform})=>{
    if(platform!==row.platform)return;reached=true;
    // All five pathname siblings become the valid original set. The held malformed
    // provider body remains unchanged and still has coherent wrapper/raw ZIP refs.
    for(const [file,bytes] of Object.entries(originals))await writeFile(path.join(root,file),bytes);
  };
  const before=f.calls.length;await assert.rejects(route==="publisher"?publish(f,{beforeNativeValidation}):assemble(f,{beforeNativeValidation}));
  assert.equal(reached,true);assert.ok((await readFile(path.join(row.directory,name))).equals(originals[name]));
  assert.ok(row.files[name].equals(held));assert.equal(row.artifact.digest,`sha256:${hash(row.zip)}`);
  assert.equal(f.calls.length,before);assert.deepEqual(f.mutations,[]);
});
test("SPEC002 AC-3 actual scoped and original publisher jobs retain the 30-minute authority bound",async()=>{
  for(const name of ["release-scoped.yml","release.yml"]){
    const workflow=(await readFile(new URL(`../.github/workflows/${name}`,import.meta.url),"utf8")).replace(/\r\n/gu,"\n");
    const starts=[...workflow.matchAll(/^  publish-candidate:$/gmu)];
    assert.equal(starts.length,1,`${name} has one actual publisher job`);
    const rest=workflow.slice(starts[0].index+starts[0][0].length);
    const next=rest.search(/^  [A-Za-z0-9_-]+:/mu);
    const job=next<0?rest:rest.slice(0,next);
    const bounds=[...job.matchAll(/^    timeout-minutes:([^\n]*)$/gmu)];
    assert.equal(bounds.length,1,`${name} requires one job-level deadline`);
    assert.equal(bounds[0][1].trim(),"30",`${name} retains the original 30-minute deadline`);
  }
});
test("actual scoped aggregate and protected publisher preserve four original public byte receipts",async()=>{
  const f=await assemble(await fixture());assert.equal((await readdir(f.assets)).length,5);const sums=await readFile(path.join(f.assets,"SHA256SUMS.txt"),"utf8");assert.equal(sums.trimEnd().split("\n").length,2);assert.match(sums.split("\n")[0],/-linux-amd64.tar.gz$/u);
  const result=await publish(f);assert.equal(result.receipt.schema,"service-lasso.tui-publication-evidence.v1");assert.equal(result.receipt.publication.assets.length,4);assert.deepEqual(f.mutations,["tag","draft","upload","upload","upload","upload","publish"]);assert.equal(result.receipt.candidate.sha256,hash(await readFile(path.join(f.assets,"candidate-manifest.json"))));assert.equal(result.receipt.publication.immutable,true);
});
test("TUI28-NATIVE aggregate and publisher use valid held originals despite later malformed pathnames",async()=>{
  const original=await fixture();let acquisitions=0;
  const beforeNativeValidation=async({root})=>{acquisitions+=1;await writeFile(path.join(root,"binary-digest.json"),'{"malformedLaterPath":true}\n');};
  const f=await assemble(original,{beforeNativeValidation});assert.equal(acquisitions,2);
  // Restore only the fixture input paths for the next independent publisher
  // acquisition; provider ZIP bodies have always remained the valid originals.
  for(const row of f.objects)for(const [name,bytes] of Object.entries(row.files))await writeFile(path.join(row.directory,name),bytes);
  const result=await publish(f,{beforeNativeValidation});assert.equal(acquisitions,4);
  assert.equal(result.receipt.outcome,"success");assert.equal(result.receipt.publication.assets.length,4);
  assert.deepEqual(f.mutations,["tag","draft","upload","upload","upload","upload","publish"]);
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

const scopedChecks=["Linux test and build","Windows test and build","Scoped release asset cross-compilation"];
const protectedScopedPolicy=contexts=>({immutableReleases:{enabled:true},environment:{name:"development-candidate",protection_rules:[{type:"wait_timer",wait_timer:1}],deployment_branch_policy:{protected_branches:true,custom_branch_policies:false}},branchProtection:{required_status_checks:{strict:true,contexts},required_pull_request_reviews:{required_approving_review_count:0},allow_force_pushes:{enabled:false}}});
test("TUI28-PUBLISH scoped policy binds actual two-target CI without a Darwin prerequisite",async()=>{
  const workflow=(await readFile(new URL("../.github/workflows/ci.yml",import.meta.url),"utf8")).replace(/\r\n/gu,"\n");
  const starts=[...workflow.matchAll(/^  scoped-release-asset-compile:$/gmu)];assert.equal(starts.length,1);
  const rest=workflow.slice(starts[0].index+starts[0][0].length),next=rest.search(/^  [A-Za-z0-9_-]+:/mu),job=next<0?rest:rest.slice(0,next);
  assert.match(job,/^    name: Scoped release asset cross-compilation$/mu);
  assert.doesNotMatch(job,/^    needs:|darwin|macos/imu);
  const targets=[...job.matchAll(/^          for target in ([^;]+); do$/gmu)];assert.equal(targets.length,1);assert.deepEqual(targets[0][1].trim().split(/\s+/u),["windows/amd64","linux/amd64"]);
  assert.match(job,/node scripts\/assert-go-source-provenance\.mjs/u);
  assert.match(job,/test "\$source_commit" = "\$CI_SOURCE_SHA"/u);
  assert.match(job,/go build -mod=readonly -buildvcs=true/u);
  assert.match(job,/vcs\.revision=\$source_commit/u);assert.match(job,/vcs\.modified=false/u);
  assert.deepEqual(assertPreflight(protectedScopedPolicy(scopedChecks)).requiredChecks,scopedChecks);
  // Legacy source policy is independently retained, rather than relabeled scoped.
  const legacy=await readFile(new URL("./verify-candidate-publication.mjs",import.meta.url),"utf8");assert.match(legacy,/"macOS test and build", "Release asset cross-compilation"/u);
});
for(const [label,contexts] of [...scopedChecks.map(check=>[`missing ${check}`,scopedChecks.filter(name=>name!==check)]),["legacy-only substitution",["Linux test and build","Windows test and build","macOS test and build","Release asset cross-compilation"]]])
test(`TUI28-DENIALS actual protected publisher denies ${label} before every mutation`,async()=>{
  const f=await assemble(await fixture()),original=f.fetchImpl;f.fetchImpl=async(url,options)=>{
    if(String(url).endsWith("/branches/develop/protection")){const policy=protectedScopedPolicy(contexts).branchProtection;return {status:200,headers:{get:()=>null},body:(async function*(){yield Buffer.from(JSON.stringify(policy));})()};}
    return original(url,options);
  };
  await assert.rejects(publish(f),/missing a current TUI CI check/u);assert.deepEqual(f.mutations,[]);
});
