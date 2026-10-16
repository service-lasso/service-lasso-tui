import {open, mkdir, lstat} from 'node:fs/promises';
import {spawn} from 'node:child_process';
import path from 'node:path';
import {HeldOverlayInputs, requireEmittedQueryPair} from './held-overlay-inputs.mjs';
import {windowsPayloadFirstBuildPlan} from './native-owner-build-plan.mjs';
import {CommandFailureLedger} from './command-failure-ledger.mjs';
import {completeFileBackedCommand} from './file-backed-command-completion.mjs';

const same=(a,b)=>a.dev===b.dev&&a.ino===b.ino;
const key=value=>path.resolve(value).toLowerCase();
const decode=bytes=>new TextDecoder('utf-8',{fatal:true}).decode(bytes);
const failureOwners=new WeakMap();
function absolute(value){
 if(typeof value!=='string'||!path.isAbsolute(value)||value.includes('\0'))throw new Error('required absolute build input');
 return value;
}
export class WindowsBuildAttemptFailure extends Error {
 constructor(owner){
  super('Windows payload-first build incomplete; original records retained privately');
  failureOwners.set(this,owner);
 }
}
// Actual private ownership is not an enumerable Error property that an
// unhandled-error diagnostic would recursively print with paths/env/streams.
// This association is not a custody root: the actual enclosing caller must
// strongly retain the wrapper until retrieval, then the SAME owner through all
// borrowers/once-close. That genuine caller route is currently unconnected;
// abandoned wrappers/GC imply no closure and production activation stays closed.
export const retainedWindowsBuildAttempt=original=>failureOwners.get(original);

// Internal execution body, not an admission or issuer constructor. The enclosing
// owner supplies the genuine reservation, complete input/tool/provider closure,
// exclusive output parent and compiler-family custody before this call. Native
// identities and byte joins below establish correspondence only.
export async function executeWindowsPayloadFirstBuild(inputs){
 const held=new HeldOverlayInputs();
 const owner={held,commands:[],streams:[],outputs:[],secondaryOriginals:[],
  primaryPresent:false,state:'incomplete',phase:'validate-inputs',closePromise:null};
 owner.closeAfterLastBorrower=()=>{
  if(owner.closePromise)return owner.closePromise;
  owner.closePromise=(async()=>{
   for(const row of [...owner.streams].reverse()){
    if(row.closeAttempted)continue;
    row.closeAttempted=true;
    try{await row.handle.close();row.closeCompleted=true;}catch(original){row.closeOriginal=original;owner.secondaryOriginals.push(original);}
   }
   try{await held.close();}catch(original){owner.secondaryOriginals.push(original);}
   if(owner.secondaryOriginals.length)throw new WindowsBuildAttemptFailure(owner);
  })();
  return owner.closePromise;
 };
 try{
  const {go,node,git,compiler,goroot,sourceRoot,queryProfile,generatedEntry,attemptRoot,expectedSourceCommit,
   environmentOriginal,transitiveInputs}=inputs;
  for(const value of [go,node,git,compiler,goroot,sourceRoot,queryProfile,generatedEntry,attemptRoot])absolute(value);
  if(new Set([go,node,git,compiler,queryProfile,generatedEntry].map(key)).size!==6)throw new Error('distinct principal build input roles required');
  if(!/^[a-f0-9]{40}$/u.test(expectedSourceCommit)||!Array.isArray(transitiveInputs)||!transitiveInputs.length||
   !environmentOriginal||typeof environmentOriginal!=='object'||Array.isArray(environmentOriginal))throw new Error('missing original build association inputs');
  const environment=Object.create(null);
  for(const [name,value] of Object.entries(environmentOriginal)){
   if(!name.length||/[=\0]/u.test(name)||typeof value!=='string'||value.includes('\0'))throw new Error('invalid original environment representation');
   if(Object.keys(environment).some(previous=>previous.toLowerCase()===name.toLowerCase()))throw new Error('ambiguous Windows environment');
   environment[name]=value;
  }
  const output=name=>path.join(attemptRoot,name);
  const plan=windowsPayloadFirstBuildPlan({go,node,compiler,goroot,sourceRoot,queryProfile,generatedEntry,
   overlay:output('overlay.json'),queryRegionsGo:output('query-regions.go'),queryHeader:output('query-regions.h'),
   payload:output('service-lasso-tui-go.exe'),rootHeader:output('private-root.h'),objectDirectory:output('objects'),ownerOutput:output('service-lasso-tui.exe')});
  owner.plan=plan;owner.sourceRoot=sourceRoot;
  // No inherited case variant may override the selected fixed toolchain values.
  for(const [name,value] of Object.entries(plan.environment)){
   for(const previous of Object.keys(environment))if(previous.toLowerCase()===name.toLowerCase())delete environment[previous];
   environment[name]=value;
  }
  owner.environment=Object.freeze(environment);
  const paths=[go,node,git,compiler,queryProfile,generatedEntry,
   path.join(sourceRoot,'go.mod'),path.join(sourceRoot,'go.sum'),
   path.join(sourceRoot,'scripts/windows-payload-first-executor.mjs'),
   path.join(sourceRoot,'scripts/held-overlay-inputs.mjs'),
   path.join(sourceRoot,'scripts/command-failure-ledger.mjs'),
   path.join(sourceRoot,'scripts/file-backed-command-completion.mjs'),
   path.join(sourceRoot,'scripts/native-owner-build-plan.mjs'),
   path.join(sourceRoot,'scripts/emit-windows-query-regions.mjs'),
   path.join(sourceRoot,'scripts/generate-windows-query-regions.mjs'),
   path.join(sourceRoot,'scripts/emit-windows-native-root-header.mjs'),
   path.join(sourceRoot,'scripts/windows-native-root-locator.mjs'),
   path.join(sourceRoot,'scripts/windows-native-root-layout.mjs'),
   ...plan.overlayMappings.filter(row=>key(row.backing)!==key(output('query-regions.go'))).map(row=>row.backing),
   ...plan.owner.commands.map(row=>path.resolve(sourceRoot,row.source)),...transitiveInputs];
  owner.phase='acquire-original-inputs';
  for(const file of paths){
   absolute(file);
   const relative=path.relative(attemptRoot,file);
   if(relative===''||(!relative.startsWith('..'+path.sep)&&relative!=='..'&&!path.isAbsolute(relative)))throw new Error('input inside output namespace');
   const row=await held.acquire(file);
   if(held.rows.some(previous=>previous!==row&&same(previous.before,row.before)))throw new Error('distinct input names alias original');
  }
  const profile=await held.acquire(queryProfile);
  // Full schema arithmetic is performed by the existing emitter. This early
  // association check supplies no provenance, provider or finance authority.
  const parsed=JSON.parse(decode(profile.bytes));
  if(parsed.schemaVersion!==3||parsed.sourceCommit!==expectedSourceCommit)throw new Error('exact schema3 source association required');
  if(process.platform!=='win32'||process.arch!=='x64')throw new Error('native Windows amd64 build required');
  await held.revalidate();
  await mkdir(attemptRoot); // must fail if any prior attempt occupies this name
  await mkdir(output('objects'));
  async function absent(file){
   try{await lstat(file);}catch(original){if(original?.code==='ENOENT')return;throw original;}
   throw new Error('build output already occupied');
  }
  async function retainOutput(file){
   const row=await held.acquire(file);
   if(held.rows.some(previous=>previous!==row&&same(previous.before,row.before)))throw new Error('build output aliases held original');
   owner.outputs.push(row);return row;
  }
  async function command(phase,program,argv,destination){
   owner.phase=phase;
   await held.revalidate();
   const destinations=destination?(Array.isArray(destination)?destination:[destination]):[];
   for(const file of destinations)await absent(file);
   const row={phase,command:program,argv:[...argv],cwd:sourceRoot,environment:owner.environment,state:'opening-streams',failures:new CommandFailureLedger()};
   owner.commands.push(row);
   for(const channel of ['stdout','stderr']){
    const file=output('command-'+(owner.commands.length-1)+'.'+channel);
    const handle=await open(file,'wx+'),stream={path:file,handle,reads:[]};
    owner.streams.push(stream);row[channel]=stream;
   }
   row.state='starting';
   // No shell, timeout, retry, kill or substitute tool selection.
   const child=spawn(program,row.argv,{cwd:sourceRoot,env:owner.environment,
    stdio:['ignore',row.stdout.handle.fd,row.stderr.handle.fd],windowsHide:true});
   row.child=child;
   return completeFileBackedCommand(row,owner,destinations);
  }
  if(decode(await command('source-head',git,['rev-parse','HEAD'])).trim()!==expectedSourceCommit)throw new Error('source head differs');
  if(decode(await command('source-clean',git,['status','--porcelain=v1','--untracked-files=all'])).trim())throw new Error('source tree is not clean');
  const tool=JSON.parse(decode(await command('matching-host-toolchain',go,['env','-json','GOROOT','GOVERSION','GOHOSTOS','GOHOSTARCH'])));
  if(tool.GOVERSION!=='go1.26.0'||tool.GOHOSTOS!=='windows'||tool.GOHOSTARCH!=='amd64'||key(tool.GOROOT)!==key(goroot))throw new Error('matching original Go toolchain required');
  const emitted=JSON.parse(decode(await command('emit-query-pair',plan.queryRegions.command,plan.queryRegions.argv,[output('query-regions.go'),output('query-regions.h')])));
  const goRow=await held.acquire(output('query-regions.go')),headerRow=await held.acquire(output('query-regions.h'));
  requireEmittedQueryPair(emitted,profile,goRow,headerRow,expectedSourceCommit);
  const overlayBytes=Buffer.from(JSON.stringify({Replace:Object.fromEntries(plan.overlayMappings.map(row=>[row.target,row.backing]))}));
  const overlayHandle=await open(output('overlay.json'),'wx+');
  const overlayOriginal={path:output('overlay.json'),handle:overlayHandle,reads:[]};owner.streams.push(overlayOriginal);
  await overlayHandle.writeFile(overlayBytes);await overlayHandle.sync();
  Object.assign(overlayOriginal,await held.read(overlayHandle,overlayOriginal));
  if(!overlayOriginal.bytes.equals(overlayBytes))throw new Error('overlay original readback differs');
  await retainOutput(overlayOriginal.path);
  await command('build-private-payload',plan.payload.command,plan.payload.argv,output('service-lasso-tui-go.exe'));
  await command('derive-private-root',plan.locator.command,plan.locator.argv,output('private-root.h'));
  for(const row of plan.owner.commands)await command('compile-native-owner',row.command,row.argv,row.output);
  await command('link-native-owner',plan.link.command,plan.link.argv,plan.link.output);
  await held.revalidate();
  owner.phase='retained-completed-build';owner.state='completed-build-not-activated';
  return owner;
 }catch(original){
  owner.primaryPresent=true;owner.primaryOriginal=original;
  // Live originals are deliberately returned with the failure, not silently
  // closed while a compiler family or later private borrower may still exist.
  throw new WindowsBuildAttemptFailure(owner);
 }
}
