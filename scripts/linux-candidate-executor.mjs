import {open, mkdir, lstat, readdir, realpath} from 'node:fs/promises';
import {spawn} from 'node:child_process';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {HeldOverlayInputs} from './held-overlay-inputs.mjs';
import {linuxCandidateBuildPlan} from './native-owner-build-plan.mjs';
import {CommandFailureLedger} from './command-failure-ledger.mjs';
import {completeFileBackedCommand} from './file-backed-command-completion.mjs';

const same=(a,b)=>a.dev===b.dev&&a.ino===b.ino;
const key=value=>path.resolve(value);
const decode=bytes=>new TextDecoder('utf-8',{fatal:true}).decode(bytes);
const failureOwners=new WeakMap();
const stable=(a,b)=>same(a,b)&&a.size===b.size&&a.mtimeNs===b.mtimeNs&&a.ctimeNs===b.ctimeNs;
function inside(parent,file){
 const relative=path.relative(parent,file);
 return relative===''||(!path.isAbsolute(relative)&&relative!=='..'&&!relative.startsWith('..'+path.sep));
}
async function sourceFiles(directory){
 const files=[];
 for(const entry of await readdir(directory,{withFileTypes:true})){
  const file=path.join(directory,entry.name);
  if(entry.isSymbolicLink())throw new Error('source namespace contains unqualified symbolic link');
  if(entry.isDirectory())files.push(...await sourceFiles(file));
  else if(entry.isFile()&&/\.(?:go|h|s|S|c)$/u.test(entry.name))files.push(file);
 }
 return files.sort();
}
class LinuxHeldInputs extends HeldOverlayInputs {
 async acquire(file){
  const resolved=key(file),previous=this.rows.find(row=>row.key===resolved);
  if(previous)return previous;
  const handle=await open(file,'r'),row={path:file,key:resolved,handle,reads:[]};
  this.rows.push(row);
  Object.assign(row,await this.read(handle,row));
  row.sha256=createHash('sha256').update(row.bytes).digest('hex');
  return row;
 }
 async revalidate(){
  const failures=new CommandFailureLedger();
  this.revalidations??=[];this.revalidations.push(failures);
  for(const row of this.rows){
   await failures.attempt('held-original-revalidate',async()=>{
    const current=await this.read(row.handle,row);
    if(!stable(row.before,current.after)||!row.bytes.equals(current.bytes))throw new Error('held input original changed');
   });
   // A failed original read must not suppress this pathname or any later row.
   await failures.attempt('held-name-revalidate',async()=>{
    const named=await open(row.path,'r'),occurrence={handle:named,reads:[]};
    row.namedReads??=[];row.namedReads.push(occurrence);
    let primaryPresent=false,primary;
    try{
     Object.assign(occurrence,await this.read(named,occurrence));
     if(!stable(row.before,occurrence.after)||!row.bytes.equals(occurrence.bytes))throw new Error('held input pathname rebound');
    }catch(original){primaryPresent=true;primary=original;occurrence.primaryOriginal=original;}
    occurrence.closeAttempted=true;
    try{await named.close();occurrence.closeCompleted=true;}
    catch(original){
     occurrence.closeOriginal=original;this.closeOriginals.push({row:occurrence,original});
     if(primaryPresent)failures.record('held-name-read',primary);
     throw original;
    }
    if(primaryPresent)throw primary;
   });
  }
  failures.throwFirst();
 }
}
// Closed association only: neither these flags nor matching bytes grant native
// issuer, entry, finance, provider or compiler-family authority.
export function requireLinuxCandidateSourceAssociation(emitted,inputs,outputs){
 const closed=(value,keys)=>value&&typeof value==='object'&&!Array.isArray(value)&&Object.keys(value).sort().join(',')===[...keys].sort().join(',');
 if(!closed(emitted,['kind','inputs','outputs','nativeEntrySupplied','authenticatedIssuer'])||
  emitted.kind!=='linux-paired-source-association'||emitted.nativeEntrySupplied!==false||emitted.authenticatedIssuer!==false||
  !Array.isArray(emitted.inputs)||emitted.inputs.length!==3||!Array.isArray(emitted.outputs)||emitted.outputs.length!==2||
  !Array.isArray(inputs)||inputs.length!==3||!Array.isArray(outputs)||outputs.length!==2)throw new Error('paired source closed association required');
 for(const [reported,held]of [[emitted.inputs,inputs],[emitted.outputs,outputs]]){
  for(let index=0;index<held.length;index++){
   const item=reported[index],row=held[index];
   if(!closed(item,['size','sha256'])||!Number.isSafeInteger(item.size)||item.size<=0||
    !/^[a-f0-9]{64}$/u.test(item.sha256)||!Buffer.isBuffer(row?.bytes)||
    item.size!==row.bytes.length||item.sha256!==row.sha256)throw new Error('paired source original association differs');
  }
 }
}
function absolute(value){
 if(typeof value!=='string'||!path.isAbsolute(value)||value.includes('\0'))throw new Error('required absolute build input');
 return value;
}
export class LinuxBuildAttemptFailure extends Error {
 constructor(owner){
  super('Linux candidate build incomplete; original records retained privately');
  failureOwners.set(this,owner);
 }
}
// Actual private ownership is not an enumerable Error property that an
// unhandled-error diagnostic would recursively print with paths/env/streams.
// This association is not a custody root: the actual enclosing caller must
// strongly retain the wrapper until retrieval, then the SAME owner through all
// borrowers/once-close. That genuine caller route is currently unconnected;
// abandoned wrappers/GC imply no closure and production activation stays closed.
export const retainedLinuxBuildAttempt=original=>failureOwners.get(original);

// Internal execution body, not an admission or issuer constructor. The enclosing
// owner supplies the genuine reservation, complete input/tool/provider closure,
// exclusive output parent and compiler-family custody before this call. Native
// identities and byte joins below establish correspondence only.
export async function executeLinuxCandidateBuild(inputs){
 const held=new LinuxHeldInputs();
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
   if(owner.secondaryOriginals.length)throw new LinuxBuildAttemptFailure(owner);
  })();
  return owner.closePromise;
 };
 try{
  const {go,node,git,compiler,sourceRoot,profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,attemptRoot,expectedSourceCommit,
   environmentOriginal,transitiveInputs}=inputs;
  for(const value of [go,node,git,compiler,sourceRoot,profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,attemptRoot])absolute(value);
  if(new Set([go,node,git,compiler,profileOriginal,abiGeometryOriginal,nativeGeometryOriginal].map(key)).size!==7)throw new Error('distinct principal build input roles required');
  if(!/^[a-f0-9]{40}$/u.test(expectedSourceCommit)||!Array.isArray(transitiveInputs)||!transitiveInputs.length||
   !environmentOriginal||typeof environmentOriginal!=='object'||Array.isArray(environmentOriginal))throw new Error('missing original build association inputs');
  const environment=Object.create(null);
  for(const [name,value] of Object.entries(environmentOriginal)){
   if(!name.length||/[=\0]/u.test(name)||typeof value!=='string'||value.includes('\0'))throw new Error('invalid original environment representation');
   environment[name]=value;
  }
  const output=name=>path.join(attemptRoot,name);
  if(inside(sourceRoot,attemptRoot)||[go,node,git,compiler,profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,...transitiveInputs].some(file=>inside(attemptRoot,absolute(file))))throw new Error('output namespace overlaps original inputs');
  const actualOutput=path.join(await realpath(path.dirname(attemptRoot)),path.basename(attemptRoot));
  if(inside(await realpath(sourceRoot),actualOutput))throw new Error('output parent aliases source namespace');
  const plan=linuxCandidateBuildPlan({go,node,compiler,sourceRoot,profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,
   generatedGo:output('candidate-profile.go'),generatedEntry:output('candidate-entry.c'),
   overlay:output('overlay.json'),payload:output('service-lasso-tui-go'),
   objectDirectory:output('objects'),ownerOutput:output('service-lasso-tui')});
  owner.plan=plan;owner.sourceRoot=sourceRoot;
  Object.assign(environment,plan.environment);
  owner.environment=Object.freeze(environment);
  const paths=[go,node,git,compiler,profileOriginal,abiGeometryOriginal,nativeGeometryOriginal,
   path.join(sourceRoot,'go.mod'),path.join(sourceRoot,'go.sum'),
   ...['linux-candidate-executor.mjs','held-overlay-inputs.mjs','command-failure-ledger.mjs',
    'file-backed-command-completion.mjs','native-owner-build-plan.mjs','windows-native-root-layout.mjs',
    'emit-linux-candidate-profile.mjs','candidate-profile-source.mjs'].map(file=>path.join(sourceRoot,'scripts',file)),
   ...plan.owner.commands.filter(row=>row.source!==output('candidate-entry.c')).map(row=>path.resolve(sourceRoot,row.source)),
   ...transitiveInputs];
  async function acquireDistinct(file){
   const row=await held.acquire(file);
   if(held.rows.some(previous=>previous!==row&&same(previous.before,row.before)))throw new Error('distinct input names alias original');
   return row;
  }
  owner.phase='acquire-original-inputs';
  for(const file of paths){
   absolute(file);
   const relative=path.relative(attemptRoot,file);
   if(relative===''||(!relative.startsWith('..'+path.sep)&&relative!=='..'&&!path.isAbsolute(relative)))throw new Error('input inside output namespace');
   await acquireDistinct(file);
  }
  // Repository include and Go source closure is held in addition to the caller's
  // complete toolchain/SDK/module/provider transitive original roster.
  for(const directory of ['native/owner','third_party/libsepol/src','third_party/libsepol/include','cmd','internal','third_party']){
   for(const file of await sourceFiles(path.join(sourceRoot,directory)))await acquireDistinct(file);
  }
  const originals=[profileOriginal,abiGeometryOriginal,nativeGeometryOriginal].map(file=>held.rows.find(row=>row.key===key(file)));
  const abi=JSON.parse(decode(originals[1].bytes));
  if(!abi||Object.keys(abi).sort().join(',')!=='cSizeTBits,goIntBits'||abi.goIntBits!==64||abi.cSizeTBits!==64)throw new Error('Linux amd64 ABI original required');
  if(process.platform!=='linux'||process.arch!=='x64')throw new Error('native Linux amd64 build required');
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
  const tool=JSON.parse(decode(await command('matching-host-toolchain',go,['env','-json','GOVERSION','GOHOSTOS','GOHOSTARCH'])));
  if(tool.GOVERSION!=='go1.26.0'||tool.GOHOSTOS!=='linux'||tool.GOHOSTARCH!=='amd64')throw new Error('matching original Go toolchain required');
  if(decode(await command('matching-node-toolchain',node,['--version'])).trim()!=='v22.23.2')throw new Error('matching original Node toolchain required');
  const emitted=JSON.parse(decode(await command('emit-paired-candidate-source',plan.emit.command,plan.emit.argv,[output('candidate-profile.go'),output('candidate-entry.c')])));
  const generated=[await held.acquire(output('candidate-profile.go')),await held.acquire(output('candidate-entry.c'))];
  requireLinuxCandidateSourceAssociation(emitted,originals,generated);
  const overlayBytes=Buffer.from(JSON.stringify({Replace:Object.fromEntries(plan.overlayMappings.map(row=>[row.target,row.backing]))}));
  const overlayHandle=await open(output('overlay.json'),'wx+');
  const overlayOriginal={path:output('overlay.json'),handle:overlayHandle,reads:[]};owner.streams.push(overlayOriginal);
  await overlayHandle.writeFile(overlayBytes);await overlayHandle.sync();
  Object.assign(overlayOriginal,await held.read(overlayHandle,overlayOriginal));
  if(!overlayOriginal.bytes.equals(overlayBytes))throw new Error('overlay original readback differs');
  await retainOutput(overlayOriginal.path);
  await command('test-candidate-source',plan.test.command,plan.test.argv);
  await command('build-private-payload',plan.payload.command,plan.payload.argv,output('service-lasso-tui-go'));
  for(const row of plan.owner.commands)await command('compile-native-owner',row.command,row.argv,row.output);
  await command('link-native-owner',plan.link.command,plan.link.argv,plan.link.output);
  await held.revalidate();
  // Current generated C has no genuine main/_start. A natural link failure is
  // retained above; completing these commands still grants no native authority.
  owner.phase='retained-completed-build';owner.state='completed-build-not-activated';
  return owner;
 }catch(original){
  owner.primaryPresent=true;owner.primaryOriginal=original;
  // Live originals are deliberately returned with the failure, not silently
  // closed while a compiler family or later private borrower may still exist.
  throw new LinuxBuildAttemptFailure(owner);
 }
}
