import { spawn } from "node:child_process";
import { readNativeProducerHandoff } from "./native-producer-handoff.mjs";
import { observeNativeProducerChild, nativeProducerClosedSuccessfully } from "./native-producer-child.mjs";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { execFileSync } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { assertSourcePolicy } from "./scoped-policy.mjs";
import { parseStrictJSON } from "./scoped-json.mjs";
import { candidateIdentity } from "./verify-scoped-candidate-publication.mjs";
import { createNativeArchive, verifyNativeArchive } from "./scoped-native-archive.mjs";
import {createNativeOwnerArchive,nativeOwnerMemberInventory,nativeOwnerLayout,assertCoreNativeOwnerAsset} from './native-owner-package.mjs';
import { verifyNativePublicReceipt } from "./scoped-native-public.mjs";
import { SCOPE, CORE_COMMIT, PUBLIC_RECEIPTS, hash, exact, assertNativeWrapper, assertProviderJob } from "./scoped-native-contract.mjs";
import { currentAttempt } from "./scoped-actions.mjs";
export function assertNativeOutcome(value,{source,binary}){
  if(value.outcome!=="succeeded"||value.coreCommit!==CORE_COMMIT||value.candidateIdentity?.sourceCommit!==source.commit||value.candidateIdentity?.binarySHA256!==binary.sha256||JSON.stringify(value.actions)!==JSON.stringify(["install","config","start","stop","restart"])||value.fixtureOperationCount!==5||value.reloadDenied!==true)throw new Error("complete native action proof denied");
  exact(value.keyboard,["previewEscapeNoSubmission","helpRendered","nativeResizeRendered","unsupportedRunningCancellationDenied"]);if(Object.values(value.keyboard).some(item=>item!==true))throw new Error("native keyboard proof denied");
  exact(value.cancellation,["advertised","supportedActionTested"]);if(value.cancellation.advertised!==false||value.cancellation.supportedActionTested!==false)throw new Error("unsupported cancellation proof denied");
  exact(value.reconnect,["completedOperationNoReplay","pendingReconciliation"]);if(value.reconnect.completedOperationNoReplay!==true||value.reconnect.pendingReconciliation!=="blocked_core_1553_no_adapter")throw new Error("qualified runtime reconnect proof denied");
  if(value.unrelatedService?.unchangedAfterFiveActions!==true||!Array.isArray(value.terminals)||value.terminals.length!==5||value.terminals.some(item=>item.exit!==(item.terminal==="missing-credential"?"terminal_exit_code_2":"terminal_exited_zero"))||new Set(value.terminals.map(item=>item.terminal)).size!==5||!Array.isArray(value.adverseAudit)||value.adverseAudit.length!==3)throw new Error("native terminal/error proof denied");
  const expected={"missing-credential":0,"invalid-credential":0,"scope-denied":5};if(new Set(value.adverseAudit.map(item=>item.case)).size!==3||value.terminals.map(item=>item.terminal).sort().join(",")!==["missing-credential","invalid-credential","scope-denied","allowed","reconnect"].sort().join(","))throw new Error("native error/terminal inventory denied");for(const item of value.adverseAudit)if(!(item.case in expected)||item.noOperation!==true||item.beforeOperationCount!==item.afterOperationCount||item.coreDeniedAuditDelta!==expected[item.case]||item.coreDeniedAuditAfter-item.coreDeniedAuditBefore!==expected[item.case])throw new Error("native adverse audit denied");
}
// Exact pair-byte packaging, not an issuer/qualification constructor. Genuine
// retained owner/payload and receipt provenance must already be supplied by the
// real enclosing native producer; the default acquisition below remains closed.
export function packageNativeOwnerCandidate({source,run,version,platform,jobId,members,receiptOriginals}){
  candidateIdentity({sourceRef:source?.ref,sourceCommit:source?.commit,version,tag:`candidate-${version}`});
  const target=`${platform}-amd64`,layout=nativeOwnerLayout(target);
  const archiveName=`service-lasso-tui-${version}-${target}.${platform==='win32'?'zip':'tar.gz'}`;
  const archive=createNativeOwnerArchive(target,members),inventory=nativeOwnerMemberInventory(target,members);
  assertCoreNativeOwnerAsset({platform:target,name:archiveName,sha256:hash(archive),executable:layout[0].path,members:inventory},archive);
  exact(receiptOriginals,PUBLIC_RECEIPTS);
  const files={[archiveName]:archive};
  for(const name of PUBLIC_RECEIPTS){
    if(!Buffer.isBuffer(receiptOriginals[name]))throw new Error('actual public receipt originals required');
    files[name]=receiptOriginals[name];
  }
  const wrapper={schema:'service-lasso.tui-native-candidate.v2',scope:SCOPE,source,run,platform,jobId,
    archive:{name:archiveName,sha256:hash(archive),size:archive.length},
    binary:{name:layout[0].path,sha256:inventory[0].sha256,size:inventory[0].size},members:inventory,
    receipts:PUBLIC_RECEIPTS.map(name=>({name,sha256:hash(files[name]),size:files[name].length})),outcome:'success'};
  assertNativeWrapper(wrapper,{source,run,version,platform,bodies:files});
  return {wrapper,files};
}
export async function produceNative(){
  await assertSourcePolicy();
  // There is NO genuine installer/profile/entry/family constructor connected
  // yet, nor accepted Core schema4 receiver. No caller option, shape, digest,
  // executor return or generic callback can open this production boundary.
  // In particular, do not execute the old Go-only pipeline as a v1 fallback.
  throw new Error('genuine native-owner production acquisition unavailable');
}
// Historical source path retained for its existing Go-only pipeline semantics.
// It is private/uninvoked by the current CLI and cannot produce a v2 wrapper.
async function produceLegacyNativeSingleBody({root,output,version,windowsQueryProfile,python=process.platform==="win32"?"python":"python3",token,fetchImpl=fetch}) {
  await assertSourcePolicy();const platform=process.platform;if(!["win32","linux"].includes(platform)||process.arch!=="x64")throw new Error("native host denied");
  if(platform==='win32'&&(typeof windowsQueryProfile!=='string'||!path.isAbsolute(windowsQueryProfile)||windowsQueryProfile.includes('\0')))throw new Error('reviewed query profile required');
  if(platform==='linux'&&windowsQueryProfile!==undefined)throw new Error('foreign query profile');
  const source={repository:"service-lasso/service-lasso-tui",commit:process.env.GITHUB_SHA,ref:process.env.GITHUB_REF};candidateIdentity({sourceRef:source.ref,sourceCommit:source.commit,version,tag:`candidate-${version}`});
  const run={id:Number(process.env.GITHUB_RUN_ID),attempt:Number(process.env.GITHUB_RUN_ATTEMPT),workflowSha:source.commit};
  const provider=await currentAttempt({run,source,token,fetchImpl,nativeOnly:true});const job=assertProviderJob(provider.jobs.find(item=>item.name===`native-candidate (${platform})`),platform,run,source,{terminal:false});
  const directory=path.dirname(fileURLToPath(import.meta.url));const child=spawn(python,["-s",path.join(directory,"tui28-native-producer.py"),"--root",root,"--source",process.cwd(),"--version",version,"--controller-pid",String(process.pid),"--node",process.execPath,...(platform==='win32'?['--windows-query-profile',windowsQueryProfile]:[])],{stdio:["pipe","pipe","ignore"],env:Object.fromEntries(Object.entries(process.env).filter(([key])=>["APPDATA","COMSPEC","LOCALAPPDATA","PATHEXT","PATH","SYSTEMROOT","TEMP","TMP","USERPROFILE","WINDIR","LANG","LC_ALL","HOME","SHELL","GITHUB_SHA","GITHUB_REF"].includes(key)))});
  const childOwner=observeNativeProducerChild(child);const completion=childOwner.completion;
  try {
    const ready=await readNativeProducerHandoff(child);
    const observation=parseStrictJSON(execFileSync(python,["-s",path.join(directory,"tui28-process-observe.py"),String(child.pid)],{encoding:"utf8",stdio:["ignore","pipe","ignore"],env:Object.fromEntries(Object.entries(process.env).filter(([key])=>["APPDATA","COMSPEC","LOCALAPPDATA","PATHEXT","PATH","SYSTEMROOT","TEMP","TMP","USERPROFILE","WINDIR","LANG","LC_ALL","HOME","SHELL"].includes(key)))}));
    if(ready.event!=="input-held"||ready.writerPID!==child.pid||ready.producerPID!==process.pid||ready.sourceCommit!==source.commit||observation.parentPID!==process.pid||JSON.stringify(observation)!==JSON.stringify(ready.writerBirth))throw new Error("independent actual writer observation denied");
    await writeFile(path.join(root,"owner-private-parent-observation.json"),JSON.stringify({classification:"owner-private",producerPID:process.pid,writerPID:child.pid,writerBirth:observation})+"\n",{flag:"wx",mode:0o600});
    child.stdin.end("admit\n");
  }catch(error){child.stdin.end();const closed=await completion;throw new Error("native input handoff failed",{cause:{primary:error,childOwner:closed}});}
  const closed=await completion;if(!nativeProducerClosedSuccessfully(closed))throw new Error("native owned pipeline failed",{cause:closed});
  const custody=parseStrictJSON(await readFile(path.join(root,"owner-private-input-custody.json")));if(custody.writerPID!==child.pid||custody.producerPID!==process.pid||custody.writerBirth?.parentPID!==process.pid||custody.sourceCommit!==source.commit)throw new Error("actual child custody denied");
  verifyNativePublicReceipt(root);
  const binary=parseStrictJSON(await readFile(path.join(root,"binary-digest.json")));const primary=parseStrictJSON(await readFile(path.join(root,"native-exit-receipt.json")));assertNativeOutcome(primary,{source,binary});
  const projection=parseStrictJSON(await readFile(path.join(root,"native-public-projection.json")));if(projection.result!=="succeeded"||!projection.actionsPassed||!projection.ownedRuntimeClosed)throw new Error("owned native proof denied");
  const original=await readFile(path.join(root,platform==="win32"?"service-lasso-tui.exe":"service-lasso-tui"));if(hash(original)!==binary.sha256||original.length!==binary.size)throw new Error("native binary substitution");
  const archiveName=`service-lasso-tui-${version}-${platform}-amd64.${platform==="win32"?"zip":"tar.gz"}`;const archive=createNativeArchive(`${platform}-amd64`,original);if(!verifyNativeArchive(`${platform}-amd64`,archive).equals(original))throw new Error("native archive substitution");
  const files={[archiveName]:archive};const receipts=[];for(const name of PUBLIC_RECEIPTS){const bytes=await readFile(path.join(root,name));files[name]=bytes;receipts.push({name,sha256:hash(bytes),size:bytes.length});}
  const wrapper={schema:"service-lasso.tui-native-candidate.v1",scope:SCOPE,source,run,platform,jobId:job.id,archive:{name:archiveName,sha256:hash(archive),size:archive.length},binary:{name:platform==="win32"?"service-lasso-tui.exe":"service-lasso-tui",sha256:binary.sha256,size:binary.size},receipts,outcome:"success"};assertNativeWrapper(wrapper,{source,run,version,platform,bodies:files});
  await mkdir(output);for(const [name,bytes] of Object.entries(files))await writeFile(path.join(output,name),bytes,{flag:"wx"});await writeFile(path.join(output,`native-${platform}.json`),JSON.stringify(wrapper)+"\n",{flag:"wx"});return wrapper;
}
if(process.argv[1]===fileURLToPath(import.meta.url))produceNative({root:process.argv[2],output:process.argv[3],windowsQueryProfile:process.argv[4],version:process.env.CANDIDATE_VERSION,token:process.env.GITHUB_TOKEN}).catch(()=>{process.stderr.write("native candidate denied\n");process.exitCode=1;});
