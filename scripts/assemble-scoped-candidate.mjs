import { verifyProviderArtifactBytes } from "./scoped-artifact-custody.mjs";
import { mkdir, readdir, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { parseStrictJSON } from "./scoped-json.mjs";
import { assertSourcePolicy } from "./scoped-policy.mjs";
import { candidateIdentity, assertManifest, assertActualLocalAssets, readBoundedRegularLocalAsset } from "./verify-scoped-candidate-publication.mjs";
import { SCOPE, NATIVE_PLATFORMS, PUBLIC_RECEIPTS, hash, assertAggregate, assertNativeWrapper } from "./scoped-native-contract.mjs";
import { currentAttempt } from "./scoped-actions.mjs";
export async function readNativeInputs(directory,{version,source,run,beforeNativeValidation}) {
  const entries=await readdir(directory);if(entries.length!==2||entries.some(name=>!NATIVE_PLATFORMS.includes(name)))throw new Error("aggregate directory denied");
  const wrappers=[],bodies=[];
  for(const platform of NATIVE_PLATFORMS){
    const root=path.join(directory,platform);const name=`native-${platform}.json`;const wrapper=await readBoundedRegularLocalAsset(root,name,1048576);const value=parseStrictJSON(wrapper);
    const archiveName=`service-lasso-tui-${version}-${platform}-amd64.${platform==="win32"?"zip":"tar.gz"}`;
    const expected=[name,archiveName,...PUBLIC_RECEIPTS].sort();const actual=(await readdir(root)).sort();if(actual.join("\0")!==expected.join("\0"))throw new Error("native input inventory denied");
    const files={};for(const name of [archiveName,...PUBLIC_RECEIPTS])files[name]=await readBoundedRegularLocalAsset(root,name,name.endsWith(".json")?1048576:1024*1024*1024);
    // Observe the acquisition boundary without granting access to held buffers or replacing validation.
    if(beforeNativeValidation)await beforeNativeValidation({root,platform});
    assertNativeWrapper(value,{source,run,version,platform,bodies:files});
    wrappers.push(value);bodies.push({wrapper,files});
  }return {wrappers,bodies};
}
export async function assembleScopedCandidate({nativeDirectory,assetDirectory,evidenceDirectory,identity,run,token,fetchImpl=fetch,beforeNativeValidation}) {
  await assertSourcePolicy();candidateIdentity(identity);const source={repository:"service-lasso/service-lasso-tui",commit:identity.sourceCommit,ref:identity.sourceRef};
  const inputs=await readNativeInputs(nativeDirectory,{version:identity.version,source,run,beforeNativeValidation});const provider=await currentAttempt({run,source,token,fetchImpl});
  const archiveAssets=inputs.wrappers.map(value=>({executable:value.binary.name,name:value.archive.name,platform:`${value.platform}-amd64`,sha256:value.archive.sha256})).sort((a,b)=>a.name<b.name?-1:a.name>b.name?1:0);
  const sums=Buffer.from(archiveAssets.map(asset=>`${asset.sha256}  ${asset.name}\n`).join(""));
  const manifest=assertManifest({schemaVersion:3,kind:"develop-prerelease-candidate",version:identity.version,source,release:{tag:identity.tag,draft:false,prerelease:true,immutable:true},corePackagingIssue:"service-lasso/service-lasso#1461",assets:archiveAssets,checksumManifest:{name:"SHA256SUMS.txt",sha256:hash(sums)},scope:SCOPE},identity);
  const qualification={schema:"service-lasso.tui-candidate-qualification.v1",scope:SCOPE,source,run,platforms:NATIVE_PLATFORMS,receipts:inputs.wrappers.map((wrapper,index)=>({platform:wrapper.platform,jobId:wrapper.jobId,runId:run.id,runAttempt:run.attempt,workflowSha:run.workflowSha,name:`native-${wrapper.platform}.json`,sha256:hash(inputs.bodies[index].wrapper),size:inputs.bodies[index].wrapper.length})),outcome:"success"};
  assertAggregate(qualification,{source,run,manifest,...inputs,...provider});
  for (const [index,platform] of NATIVE_PLATFORMS.entries()) await verifyProviderArtifactBytes({artifact:provider.artifacts.find(item=>item.name===`scoped-native-${platform}-${run.id}-${run.attempt}`),expected:{...inputs.bodies[index].files,[`native-${platform}.json`]:inputs.bodies[index].wrapper},token,fetchImpl});
  await mkdir(path.dirname(assetDirectory));
  await mkdir(assetDirectory);const buffers={"SHA256SUMS.txt":sums,"candidate-manifest.json":Buffer.from(JSON.stringify(manifest)+"\n")};for(const row of inputs.bodies)for(const [name,bytes] of Object.entries(row.files))if(name.startsWith("service-lasso-tui-"))buffers[name]=bytes;
  const inventory={};for(const name of Object.keys(buffers).sort()){await writeFile(path.join(assetDirectory,name),buffers[name],{flag:"wx"});inventory[name]={sha256:hash(buffers[name]),size:buffers[name].length};}
  await writeFile(path.join(assetDirectory,"candidate-local-assets.json"),JSON.stringify(inventory)+"\n",{flag:"wx"});await assertActualLocalAssets({assetDirectory,manifest,localAssets:inventory});
  await mkdir(evidenceDirectory);await writeFile(path.join(evidenceDirectory,"qualification.json"),JSON.stringify(qualification)+"\n",{flag:"wx"});return qualification;
}
export async function verifyScopedQualification({nativeDirectory,evidenceDirectory,manifest,run,token,fetchImpl=fetch,beforeNativeValidation}) {
  await assertSourcePolicy();const source={repository:manifest.source.repository,commit:manifest.source.commit,ref:manifest.source.ref};
  const bytes=await readBoundedRegularLocalAsset(evidenceDirectory,"qualification.json",1048576);const qualification=parseStrictJSON(bytes);const inputs=await readNativeInputs(nativeDirectory,{version:manifest.version,source,run,beforeNativeValidation});const provider=await currentAttempt({run,source,token,fetchImpl});
  assertAggregate(qualification,{source,run,manifest,...inputs,...provider});
  const aggregates=provider.jobs.filter(job=>job.name==="aggregate"); if(aggregates.length!==1||aggregates[0].status!=="completed"||aggregates[0].conclusion!=="success"||aggregates[0].head_sha!==source.commit||aggregates[0].run_attempt!==run.attempt)throw new Error("actual aggregate job denied");
  for (const [index,platform] of NATIVE_PLATFORMS.entries()) await verifyProviderArtifactBytes({artifact:provider.artifacts.find(item=>item.name===`scoped-native-${platform}-${run.id}-${run.attempt}`),expected:{...inputs.bodies[index].files,[`native-${platform}.json`]:inputs.bodies[index].wrapper},token,fetchImpl});return {qualification,ref:{name:"qualification.json",sha256:hash(bytes),size:bytes.length}};
}
if(process.argv[1]===fileURLToPath(import.meta.url)) {
  const identity=candidateIdentity({sourceRef:process.env.GITHUB_REF,sourceCommit:process.env.GITHUB_SHA,version:process.env.CANDIDATE_VERSION,tag:`candidate-${process.env.CANDIDATE_VERSION}`});
  assembleScopedCandidate({nativeDirectory:process.argv[2],assetDirectory:process.argv[3],evidenceDirectory:process.argv[4],identity,run:{id:Number(process.env.GITHUB_RUN_ID),attempt:Number(process.env.GITHUB_RUN_ATTEMPT),workflowSha:process.env.GITHUB_SHA},token:process.env.GITHUB_TOKEN}).catch(()=>{process.stderr.write("scoped aggregate denied\n");process.exitCode=1;});
}
