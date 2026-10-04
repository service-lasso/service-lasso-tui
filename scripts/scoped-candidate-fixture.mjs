// Production-shaped fixture bytes prove source boundary consistency only. They
// never constitute native execution, actual publication or catalog admission.
import { mkdtemp, mkdir, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { SCOPE, CORE_COMMIT, PUBLIC_RECEIPTS, hash } from "./scoped-native-contract.mjs";
import { createNativeArchive, crc32 } from "./scoped-native-archive.mjs";
export const SOURCE={repository:"service-lasso/service-lasso-tui",commit:"a".repeat(40),ref:"refs/heads/develop"};
export const RUN={id:55,attempt:2,workflowSha:SOURCE.commit};
export const VERSION="2026.10.4-aaaaaaa";
export const IDENTITY={sourceRef:SOURCE.ref,sourceCommit:SOURCE.commit,version:VERSION,tag:`candidate-${VERSION}`};
const json=value=>Buffer.from(JSON.stringify(value)+"\n");
const EMPTY=hash(Buffer.alloc(0));
export function artifactZIP(files){
  const locals=[],centrals=[];let offset=0;
  for(const name of Object.keys(files).sort()){
    const data=files[name],label=Buffer.from(name),crc=crc32(data),local=Buffer.alloc(30),central=Buffer.alloc(46);
    local.writeUInt32LE(0x04034b50);local.writeUInt16LE(20,4);local.writeUInt32LE(crc,14);local.writeUInt32LE(data.length,18);local.writeUInt32LE(data.length,22);local.writeUInt16LE(label.length,26);
    central.writeUInt32LE(0x02014b50);central.writeUInt16LE(20,4);central.writeUInt16LE(20,6);central.writeUInt32LE(crc,16);central.writeUInt32LE(data.length,20);central.writeUInt32LE(data.length,24);central.writeUInt16LE(label.length,28);central.writeUInt32LE(offset,42);
    locals.push(local,label,data);centrals.push(central,label);offset+=30+label.length+data.length;
  }
  const central=Buffer.concat(centrals),end=Buffer.alloc(22);end.writeUInt32LE(0x06054b50);end.writeUInt16LE(Object.keys(files).length,8);end.writeUInt16LE(Object.keys(files).length,10);end.writeUInt32LE(central.length,12);end.writeUInt32LE(offset,16);return Buffer.concat([...locals,central,end]);
}
export async function fixture(){
  const root=await mkdtemp(path.join(os.tmpdir(),"tui-scoped-fixture-")),native=path.join(root,"native");await mkdir(native);const objects=[];
  const jobs=[{id:100,name:"validate-source",head_sha:SOURCE.commit,run_id:RUN.id,run_attempt:RUN.attempt,status:"completed",conclusion:"success"},{id:103,name:"aggregate",head_sha:SOURCE.commit,run_id:RUN.id,run_attempt:RUN.attempt,status:"completed",conclusion:"success"}];
  for(const [index,platform] of ["win32","linux"].entries()){
    const binary=Buffer.alloc(256);if(platform==="linux"){binary.write("7f454c46",0,"hex");binary[4]=2;binary[5]=1;binary.writeUInt16LE(62,18);}else{binary.write("MZ");binary.writeUInt32LE(64,60);binary.writeUInt32LE(0x00004550,64);binary.writeUInt16LE(0x8664,68);binary.writeUInt16LE(0x20b,88);}
    const digest=hash(binary),archiveName=`service-lasso-tui-${VERSION}-${platform}-amd64.${platform==="win32"?"zip":"tar.gz"}`,archive=createNativeArchive(`${platform}-amd64`,binary);
    const records={
      "binary-digest.json":{schemaVersion:1,kind:"tui20-native-binary-digest",sha256:digest,size:binary.length},
      "build-output.json":{schemaVersion:1,kind:"tui20-native-build-output",tuiCommit:SOURCE.commit,nativeBinary:{sha256:digest,size:binary.length}},
      "core-source-binding.json":{schemaVersion:1,kind:"tui20-native-core-source-binding",coreCommit:CORE_COMMIT,coreTree:"b".repeat(40),coreDirtyHash:EMPTY},
      "input-custody.json":{schemaVersion:1,kind:"tui20-native-input-custody",source:{tuiCommit:SOURCE.commit,tuiTree:"c".repeat(40),tuiDirtyHash:EMPTY,tuiInventoryHash:hash(Buffer.from("fixture source inventory"))},core:{requestedCommit:CORE_COMMIT},ownership:{threeDistinctPaths:true,allParentsNonLink:true,registriesInitiallyAbsent:true},verification:{freshEnvironment:true,requiredToolsVerified:["git","go","node","npm","python"]}},
      "native-public-projection.json":{schemaVersion:1,kind:"tui20-native-public-result",sourceCommit:SOURCE.commit,binarySHA256:digest,result:"succeeded",actionsPassed:true,ownedRuntimeClosed:true}
    };
    const files=Object.fromEntries(Object.entries(records).map(([name,value])=>[name,json(value)]));files[archiveName]=archive;
    const wrapper={schema:"service-lasso.tui-native-candidate.v1",scope:SCOPE,source:SOURCE,run:RUN,platform,jobId:101+index,archive:{name:archiveName,sha256:hash(archive),size:archive.length},binary:{name:platform==="win32"?"service-lasso-tui.exe":"service-lasso-tui",sha256:digest,size:binary.length},receipts:PUBLIC_RECEIPTS.map(name=>({name,sha256:hash(files[name]),size:files[name].length})),outcome:"success"};files[`native-${platform}.json`]=json(wrapper);
    const directory=path.join(native,platform);await mkdir(directory);for(const [name,bytes] of Object.entries(files))await writeFile(path.join(directory,name),bytes);
    const zip=artifactZIP(files),artifact={id:201+index,name:`scoped-native-${platform}-${RUN.id}-${RUN.attempt}`,digest:`sha256:${hash(zip)}`,expired:false,workflow_run:{id:RUN.id,head_sha:SOURCE.commit}};objects.push({platform,files,wrapper,zip,artifact,directory});jobs.push({id:wrapper.jobId,name:`native-candidate (${platform})`,run_id:RUN.id,run_attempt:RUN.attempt,head_sha:SOURCE.commit,status:"completed",conclusion:"success",labels:[platform==="win32"?"windows-latest":"ubuntu-latest"],runner_name:"fixture-native-host"});
  }
  const mutations=[],calls=[];let release=null,tag=null,nextId=300;
  const response=(value,status=200)=>({status,headers:{get:()=>null},body:(async function*(){yield Buffer.isBuffer(value)?value:json(value);})()});
  const fetchImpl=async(url,options={})=>{
    url=String(url);calls.push({url,method:options.method??"GET",authorization:options.headers?.Authorization});const method=options.method??"GET";
    if(url.includes("/actions/runs/")&&!url.includes("/artifacts/")){if(url.includes("/jobs?"))return response({total_count:jobs.length,jobs});if(url.includes("/artifacts?"))return response({total_count:objects.length,artifacts:objects.map(item=>item.artifact)});return response({id:RUN.id,run_attempt:RUN.attempt,head_sha:SOURCE.commit,head_branch:"develop",path:".github/workflows/release-scoped.yml"});}
    const artifact=/\/actions\/artifacts\/(\d+)\/zip$/u.exec(url);if(artifact)return {status:302,headers:{get:()=>`https://productionresultssa1.blob.core.windows.net/actions-results/${artifact[1]}?sig=fixture&se=fixture&sp=r&sv=fixture`}};
    if(url.startsWith("https://productionresultssa1.blob.core.windows.net/")){if(options.headers?.Authorization)throw new Error("artifact credential forwarded");const id=Number(new URL(url).pathname.split("/").at(-1));return response(objects.find(item=>item.artifact.id===id).zip);}
    if(url.endsWith("/immutable-releases"))return response({enabled:true});
    if(url.endsWith("/environments/development-candidate"))return response({name:"development-candidate",protection_rules:[{type:"wait_timer",wait_timer:1}],deployment_branch_policy:{protected_branches:true,custom_branch_policies:false}});
    if(url.endsWith("/branches/develop/protection"))return response({required_status_checks:{strict:true,contexts:["Linux test and build","Windows test and build","Scoped release asset cross-compilation"]},required_pull_request_reviews:{required_approving_review_count:0},allow_force_pushes:{enabled:false}});
    if(url.includes("/git/ref/tags/"))return tag?response({ref:`refs/tags/${IDENTITY.tag}`,object:{type:"commit",sha:tag}}):response({},404);
    if(url.endsWith("/git/refs")&&method==="POST"){mutations.push("tag");tag=JSON.parse(options.body).sha;return response({},201);}
    if(url.endsWith("/releases")&&method==="POST"){mutations.push("draft");release={id:299,tag_name:IDENTITY.tag,target_commitish:SOURCE.commit,draft:true,prerelease:true,immutable:false,assets:[]};return response(release,201);}
    if(url.startsWith("https://uploads.github.com/")){mutations.push("upload");const name=new URL(url).searchParams.get("name"),bytes=Buffer.from(options.body),id=nextId++;release.assets.push({id,name,url:`https://api.github.com/repos/service-lasso/service-lasso-tui/releases/assets/${id}`,size:bytes.length,digest:`sha256:${hash(bytes)}`,browser_download_url:`https://github.com/service-lasso/service-lasso-tui/releases/download/${IDENTITY.tag}/${name}`,_bytes:bytes});return response({},201);}
    if(url.includes("/releases/assets/")){const id=Number(url.split("/").at(-1));return response(release.assets.find(item=>item.id===id)._bytes);}
    if(url.startsWith("https://github.com/service-lasso/service-lasso-tui/releases/download/")){if(options.headers?.Authorization)throw new Error("public credential forwarded");return response(release.assets.find(item=>item.name===url.split("/").at(-1))._bytes);}
    if(url.endsWith("/releases/299")&&method==="PATCH"){mutations.push("publish");release.draft=false;release.immutable=true;return response(release);}
    if(url.includes("/releases/tags/")||url.endsWith("/releases/299"))return release?response(release):response({},404);
    throw new Error("unexpected fixture provider route");
  };
  return {root,native,objects,jobs,fetchImpl,mutations,calls};
}
