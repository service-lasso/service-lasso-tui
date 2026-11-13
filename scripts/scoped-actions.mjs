import { parseStrictJSON } from "./scoped-json.mjs";
import { assertProviderRun, assertProviderJob, NATIVE_PLATFORMS, nativeArtifactName } from "./scoped-native-contract.mjs";
const ROOT="https://api.github.com/repos/service-lasso/service-lasso-tui";
export async function actionsJSON(route,{token,fetchImpl=fetch}) {
  if(!token||!/^\/actions\/runs\/[1-9][0-9]*(?:\/attempts\/[1-9][0-9]*\/jobs\?per_page=100&page=[1-9][0-9]*|\/artifacts\?per_page=100&page=[1-9][0-9]*)?$/u.test(route))throw new Error("Actions route denied");
  const controller=new AbortController();let timer;
  try{return await Promise.race([new Promise((_,reject)=>{timer=setTimeout(()=>{controller.abort();reject(new Error("Actions deadline"));},30000);}), (async()=>{
    const response=await fetchImpl(ROOT+route,{redirect:"error",signal:controller.signal,headers:{Accept:"application/vnd.github+json",Authorization:`Bearer ${token}`}});
    if(response.status!==200||!response.body?.[Symbol.asyncIterator])throw new Error("Actions response denied");
    const declared=response.headers?.get?.("content-length");if(declared!=null&&(!/^[0-9]+$/u.test(declared)||Number(declared)>1048576))throw new Error("Actions metadata quota");
    const chunks=[];let count=0;for await(const chunk of response.body){const bytes=Buffer.from(chunk);count+=bytes.length;if(count>1048576)throw new Error("Actions metadata quota");chunks.push(bytes);}return parseStrictJSON(Buffer.concat(chunks));
  })()]);}finally{clearTimeout(timer);controller.abort();}
}
export async function currentAttempt({run,source,token,fetchImpl=fetch,nativeOnly=false}) {
  const providerRun=await actionsJSON(`/actions/runs/${run.id}`,{token,fetchImpl});assertProviderRun(providerRun,run,source);
  async function list(kind){const all=[];let total;for(let page=1;page<=20;page++){
    const route=kind==="jobs"?`/actions/runs/${run.id}/attempts/${run.attempt}/jobs?per_page=100&page=${page}`:`/actions/runs/${run.id}/artifacts?per_page=100&page=${page}`;
    const response=await actionsJSON(route,{token,fetchImpl});if(!Number.isSafeInteger(response.total_count)||response.total_count<0||response.total_count>2000||!Array.isArray(response[kind])||response[kind].length>100||(total!==undefined&&total!==response.total_count))throw new Error("Actions inventory denied");total=response.total_count;all.push(...response[kind]);if(all.length===total)return all;if(!response[kind].length||all.length>total)throw new Error("Actions inventory denied");
  }throw new Error("Actions pagination denied");}
  const jobs=await list("jobs");if(nativeOnly)return {providerRun,jobs};
  const artifacts=await list("artifacts");const selected=artifacts.filter(item=>item.name?.startsWith("scoped-native-"));if(selected.length!==2)throw new Error("native artifact count denied");
  const nativeJobs=jobs.filter(item=>item.name?.startsWith("native-candidate"));if(nativeJobs.length!==2)throw new Error("native job count denied");
  const identity=jobs.filter(item=>item.name==="validate-source");if(identity.length!==1||identity[0].status!=="completed"||identity[0].conclusion!=="success"||identity[0].head_sha!==source.commit||identity[0].run_attempt!==run.attempt)throw new Error("source job denied");
  for(const platform of NATIVE_PLATFORMS){assertProviderJob(nativeJobs.find(item=>item.name===`native-candidate (${platform})`),platform,run,source);const artifact=selected.find(item=>item.name===nativeArtifactName(platform,run));if(!artifact||artifact.expired!==false||artifact.workflow_run?.head_sha!==source.commit||artifact.workflow_run?.id!==run.id||!Number.isSafeInteger(artifact.id)||artifact.id<=0)throw new Error("current artifact denied");}
  if(new Set(selected.map(item=>item.id)).size!==2)throw new Error("duplicate artifact ID");
  return {providerRun,jobs,artifacts};
}
