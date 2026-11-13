import {nativeOwnerMemberInventory} from './native-owner-package.mjs';

const limit=256*1024*1024;
const fail=()=>{throw new Error('invalid build output schema');};
const exact=(value,keys)=>value&&typeof value==='object'&&!Array.isArray(value)&&Object.keys(value).sort().join('\0')===[...keys].sort().join('\0');
const commit=value=>typeof value==='string'&&/^[a-f0-9]{40}$/u.test(value);
const hash=value=>typeof value==='string'&&/^[a-f0-9]{64}$/u.test(value);
const positive=value=>Number.isSafeInteger(value)&&value>0;
export function assertNativeBuildOutput(value){
 const pair=value?.schemaVersion===2;
 if(!exact(value,['schemaVersion','kind','tuiCommit','nativeBinary',...(pair?['run','members']:[])])||
  ![1,2].includes(value.schemaVersion)||value.kind!=='tui20-native-build-output'||!commit(value.tuiCommit)||
  !exact(value.nativeBinary,['sha256','size'])||!hash(value.nativeBinary.sha256)||!positive(value.nativeBinary.size))fail();
 if(pair){
  if(!exact(value.run,['id','attempt','workflowSha'])||!positive(value.run.id)||!positive(value.run.attempt)||
   value.run.workflowSha!==value.tuiCommit||!Array.isArray(value.members)||value.members.length!==2)fail();
  let total=0;
  for(const [index,role] of ['native-owner','go-payload'].entries()){
   const member=value.members[index];
   if(!exact(member,['role','sha256','size'])||member.role!==role||!hash(member.sha256)||!positive(member.size)||member.size>limit)fail();
   total+=member.size;
  }
  if(total>limit||value.nativeBinary.sha256!==value.members[0].sha256||value.nativeBinary.size!==value.members[0].size)fail();
 }
 return value;
}

// Read the actual enclosing process's original provider run representation.
// This is metadata acquisition, never native issuer/family/finance authority.
// The genuine producer must additionally join actual currentAttempt/job
// observations; child SHA/ref alone cannot supply the enclosing run identity.
export function captureEnclosingPairRun(){
 const environmentOriginal=Object.freeze(Object.fromEntries(['GITHUB_REPOSITORY','GITHUB_SHA','GITHUB_REF','GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT'].map(name=>[name,process.env[name]])));
 const source={repository:environmentOriginal.GITHUB_REPOSITORY,commit:environmentOriginal.GITHUB_SHA,ref:environmentOriginal.GITHUB_REF};
 if(source.repository!=='service-lasso/service-lasso-tui'||source.ref!=='refs/heads/develop'||!commit(source.commit))fail();
 for(const name of ['GITHUB_RUN_ID','GITHUB_RUN_ATTEMPT'])if(typeof environmentOriginal[name]!=='string'||!/^[1-9][0-9]*$/u.test(environmentOriginal[name]))fail();
 const run={id:Number(environmentOriginal.GITHUB_RUN_ID),attempt:Number(environmentOriginal.GITHUB_RUN_ATTEMPT),workflowSha:source.commit};
 if(!positive(run.id)||!positive(run.attempt))fail();
 return {source,run,environmentOriginal};
}

// Internal receipt construction consumes actual pair buffers supplied by the
// real retained writer, and independently reads its enclosing run. Neither the
// supplied buffers nor the returned JSON grant acquisition or qualification.
// No production caller is connected while the genuine constructor is absent.
export function createEnclosingPairBuildOutput({expectedSource,expectedRun,platform,members}){
 const enclosing=captureEnclosingPairRun();
 if(!exact(expectedSource,['repository','commit','ref'])||!exact(expectedRun,['id','attempt','workflowSha'])||
  Object.keys(enclosing.source).some(key=>enclosing.source[key]!==expectedSource[key])||
  Object.keys(enclosing.run).some(key=>enclosing.run[key]!==expectedRun[key]))fail();
 const inventory=nativeOwnerMemberInventory(platform,members);
 const record={schemaVersion:2,kind:'tui20-native-build-output',tuiCommit:enclosing.source.commit,run:{...enclosing.run},
  nativeBinary:{sha256:inventory[0].sha256,size:inventory[0].size},
  members:inventory.map(member=>({role:member.role,sha256:member.sha256,size:member.size}))};
 assertNativeBuildOutput(record);
 return {record,enclosing};
}
