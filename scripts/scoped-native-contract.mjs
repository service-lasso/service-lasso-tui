import { createHash } from "node:crypto";
import { SCOPE, assertScope } from "./scoped-policy.mjs";
import { verifyNativeArchive } from "./scoped-native-archive.mjs";
import { verifyHeldNativePublicReceipt } from "./scoped-native-public.mjs";
export const CORE_COMMIT="2633c07be25512d0a84f9bfa28de6be5edff35e8";
export const PUBLIC_RECEIPTS=Object.freeze(["binary-digest.json","build-output.json","core-source-binding.json","input-custody.json","native-public-projection.json"]);
export const NATIVE_PLATFORMS=Object.freeze(["win32","linux"]);
const fail=()=>{throw new Error("scoped native evidence denied");};
export const hash=bytes=>createHash("sha256").update(bytes).digest("hex");
export function exact(value,keys){if(!value||typeof value!=="object"||Array.isArray(value)||Object.keys(value).sort().join("\0")!==[...keys].sort().join("\0"))fail();}
export function byteRef(value){exact(value,["name","sha256","size"]);if(typeof value.name!=="string"||!/^[A-Za-z0-9._-]{1,200}$/u.test(value.name)||!/^[a-f0-9]{64}$/u.test(value.sha256)||!Number.isSafeInteger(value.size)||value.size<=0||value.size>1024*1024*1024)fail();return value;}
export function assertRun(run){exact(run,["id","attempt","workflowSha"]);if(!Number.isSafeInteger(run.id)||run.id<=0||!Number.isSafeInteger(run.attempt)||run.attempt<=0||!/^[a-f0-9]{40}$/u.test(run.workflowSha))fail();return run;}
export function assertSource(source){exact(source,["repository","commit","ref"]);if(source.repository!=="service-lasso/service-lasso-tui"||source.ref!=="refs/heads/develop"||!/^[a-f0-9]{40}$/u.test(source.commit))fail();return source;}
export function nativeArtifactName(platform,run){if(!NATIVE_PLATFORMS.includes(platform))fail();assertRun(run);return `scoped-native-${platform}-${run.id}-${run.attempt}`;}
export function assertNativeWrapper(value,{source,run,version,platform,bodies}) {
  exact(value,["schema","scope","source","run","platform","jobId","archive","binary","receipts","outcome"]);
  if(value.schema!=="service-lasso.tui-native-candidate.v1"||value.outcome!=="success"||value.platform!==platform||!NATIVE_PLATFORMS.includes(platform)||!Number.isSafeInteger(value.jobId)||value.jobId<=0)fail();
  assertScope(value.scope);assertSource(value.source);assertRun(value.run);
  if(Object.keys(source).some(key=>value.source[key]!==source[key])||Object.keys(run).some(key=>value.run[key]!==run[key])||run.workflowSha!==source.commit)fail();
  byteRef(value.archive);byteRef(value.binary);
  const target=`${platform}-amd64`,executable=platform==="win32"?"service-lasso-tui.exe":"service-lasso-tui";
  if(value.archive.name!==`service-lasso-tui-${version}-${target}.${platform==="win32"?"zip":"tar.gz"}`||value.binary.name!==executable||!Array.isArray(value.receipts)||value.receipts.length!==PUBLIC_RECEIPTS.length)fail();
  if(!bodies||Object.keys(bodies).sort().join("\0")!==[...PUBLIC_RECEIPTS,value.archive.name].sort().join("\0"))fail();
  for(const [index,name] of PUBLIC_RECEIPTS.entries()){const ref=byteRef(value.receipts[index]);if(ref.name!==name||hash(bodies[name])!==ref.sha256||bodies[name].length!==ref.size)fail();}
  const archive=bodies[value.archive.name];if(hash(archive)!==value.archive.sha256||archive.length!==value.archive.size)fail();
  const binary=verifyNativeArchive(target,archive);if(hash(binary)!==value.binary.sha256||binary.length!==value.binary.size)fail();
  // Parse the same original buffers bound above and later consumed by provider ZIP checks.
  const inner=verifyHeldNativePublicReceipt(Object.fromEntries(PUBLIC_RECEIPTS.map(name=>[name,bodies[name]])));
  if(inner.input.source.tuiCommit!==source.commit||inner.binary.sha256!==hash(binary)||inner.binary.size!==binary.length||inner.core.coreCommit!==CORE_COMMIT||inner.projection.result!=="succeeded"||inner.projection.actionsPassed!==true||inner.projection.ownedRuntimeClosed!==true)fail();
  return value;
}
export function assertProviderRun(provider,run,source){assertRun(run);assertSource(source);if(provider?.id!==run.id||provider.run_attempt!==run.attempt||provider.head_sha!==source.commit||provider.head_branch!=="develop"||provider.path!==".github/workflows/release-scoped.yml"||run.workflowSha!==source.commit)fail();}
export function assertProviderJob(job,platform,run,source,{terminal=true}={}) {
  if(!NATIVE_PLATFORMS.includes(platform)||job?.name!==`native-candidate (${platform})`||!Number.isSafeInteger(job.id)||job.id<=0||job.run_id!==run.id||job.run_attempt!==run.attempt||job.head_sha!==source.commit||!Array.isArray(job.labels)||!job.labels.includes(platform==="win32"?"windows-latest":"ubuntu-latest")||typeof job.runner_name!=="string"||!job.runner_name)fail();
  if(terminal?(job.status!=="completed"||job.conclusion!=="success"):job.status!=="in_progress")fail();return job;
}
export function assertAggregate(value,{source,run,manifest,wrappers,jobs,artifacts,bodies,providerRun}) {
  exact(value,["schema","scope","source","run","platforms","receipts","outcome"]);
  if(value.schema!=="service-lasso.tui-candidate-qualification.v1"||value.outcome!=="success"||JSON.stringify(value.platforms)!==JSON.stringify(NATIVE_PLATFORMS))fail();
  assertScope(value.scope);assertScope(manifest.scope);assertSource(value.source);assertRun(value.run);assertProviderRun(providerRun,run,source);
  if(Object.keys(source).some(key=>value.source[key]!==source[key])||Object.keys(run).some(key=>value.run[key]!==run[key])||manifest.source.commit!==source.commit||wrappers.length!==2||value.receipts.length!==2)fail();
  const scoped=artifacts.filter(artifact=>artifact.name?.startsWith("scoped-native-"));if(scoped.length!==2)fail();
  const selectors=jobs.filter(job=>job.name?.startsWith("native-candidate"));if(selectors.length!==2)fail();
  const ids=new Set();
  for(const [index,platform] of NATIVE_PLATFORMS.entries()){
    const wrapper=wrappers[index],ref=value.receipts[index];exact(ref,["platform","jobId","runId","runAttempt","workflowSha","name","sha256","size"]);
    const job=assertProviderJob(selectors.find(item=>item.name===`native-candidate (${platform})`),platform,run,source);
    if(ids.has(job.id)||ref.platform!==platform||ref.jobId!==job.id||wrapper.jobId!==job.id||ref.runId!==run.id||ref.runAttempt!==run.attempt||ref.workflowSha!==run.workflowSha||ref.name!==`native-${platform}.json`||hash(bodies[index].wrapper)!==ref.sha256||bodies[index].wrapper.length!==ref.size)fail();ids.add(job.id);
    const artifact=scoped.find(item=>item.name===nativeArtifactName(platform,run));if(!artifact||artifact.expired!==false||!Number.isSafeInteger(artifact.id)||artifact.id<=0||artifact.workflow_run?.id!==run.id||artifact.workflow_run?.head_sha!==source.commit)fail();
    assertNativeWrapper(wrapper,{source,run,version:manifest.version,platform,bodies:bodies[index].files});
    const archive=manifest.assets.find(asset=>asset.platform===`${platform}-amd64`);if(!archive||archive.name!==wrapper.archive.name||archive.sha256!==wrapper.archive.sha256)fail();
  }
  return value;
}
export { SCOPE };
