import {open, mkdir, writeFile} from 'node:fs/promises';
import {spawn} from 'node:child_process';
import {createHash} from 'node:crypto';
import path from 'node:path';
import {windowsOwnedOverlayMappings} from './native-owner-build-plan.mjs';

// Exact-command admission and genuine provider/profile ownership belong to the
// external attempt owner. This executable gate does not issue those authorities.
const [go, sourceRoot, queryProfile, outputRoot] = process.argv.slice(2);
for (const value of [go, sourceRoot, queryProfile, outputRoot]) {
  if (typeof value !== 'string' || !path.isAbsolute(value) || value.includes('\0')) throw new Error('missing absolute overlay test input');
}
if (process.argv.length !== 6 || process.platform !== 'win32' || process.arch !== 'x64') throw new Error('overlay tests require native Windows amd64');
await mkdir(outputRoot); // fresh attempt only; never overwrite an earlier result
const environment = {...Object.fromEntries(Object.entries(process.env).filter(([key])=>
  ['path','systemroot','windir','temp','tmp','userprofile','localappdata','appdata','comspec','programfiles','programfiles(x86)','programdata'].includes(key.toLowerCase()))), GOOS:'windows', GOARCH:'amd64', GOAMD64:'v1',
  CGO_ENABLED:'0', GOENV:'off', GOTOOLCHAIN:'local', GOWORK:'off', GOFLAGS:'',
  GOEXPERIMENT:'', GOFIPS140:'off', GOCACHEPROG:''};
const record = {kind:'windows-owned-goroot-test-attempt', commands:[], inputs:[], state:'incomplete'};
async function original(file) {
  const handle = await open(file, 'r');
  try {
    const before = await handle.stat({bigint:true});
    if (!before.isFile()) throw new Error('overlay input is not a regular file');
    const bytes = await handle.readFile(), after = await handle.stat({bigint:true});
    if (before.dev !== after.dev || before.ino !== after.ino || before.size !== after.size ||
        before.mtimeNs !== after.mtimeNs || before.ctimeNs !== after.ctimeNs || BigInt(bytes.length) !== before.size) throw new Error('overlay input changed');
    record.inputs.push({path:file, size:bytes.length, sha256:createHash('sha256').update(bytes).digest('hex')});
    return bytes;
  } finally { await handle.close(); }
}
async function command(program, argv) {
  const index=record.commands.length;
  const row={command:program, argv, cwd:sourceRoot, environment, state:'starting'};
  record.commands.push(row);
  const stdoutChunks=[], stderrChunks=[];
  const child=spawn(program, argv, {cwd:sourceRoot, env:environment, stdio:['ignore','pipe','pipe'], windowsHide:true});
  child.stdout.on('data', bytes=>{stdoutChunks.push(bytes);});
  child.stderr.on('data', bytes=>{stderrChunks.push(bytes);});
  try {
    await new Promise((resolve,reject)=>{
      child.once('error',error=>{row.state='spawn-failed';row.failure=String(error);reject(error);});
      child.once('close',(code,signal)=>{row.exitCode=code;row.signal=signal;if(row.state!=='spawn-failed')row.state='closed';resolve();});
    });
  } finally {
    await writeFile(path.join(outputRoot, index+'.stdout'),Buffer.concat(stdoutChunks),{flag:'wx'});
    await writeFile(path.join(outputRoot, index+'.stderr'),Buffer.concat(stderrChunks),{flag:'wx'});
  }
  if(row.exitCode!==0||row.signal!==null)throw new Error('overlay command failed: '+index);
  return new TextDecoder('utf-8',{fatal:true}).decode(Buffer.concat(stdoutChunks));
}
try {
  await original(go);
  await original(process.execPath);
  await original(path.join(sourceRoot,'go.mod'));
  await original(path.join(sourceRoot,'toolchain-overlay/go1.26.0/go.mod'));
  await original(path.join(sourceRoot,'scripts/test-windows-owned-overlay.mjs'));
  const profileBytes=await original(queryProfile);
  const profile=JSON.parse(new TextDecoder('utf-8',{fatal:true}).decode(profileBytes));
  const commit=(await command('git',['rev-parse','HEAD'])).trim();
  if(!/^[a-f0-9]{40}$/u.test(commit))throw new Error('invalid checked source commit');
  record.sourceCommit=commit;
  if(profile.schemaVersion!==3||profile.sourceCommit!==commit)throw new Error('genuine schema3 query profile for exact source required');
  if((await command('git',['status','--porcelain=v1','--untracked-files=all'])).trim())throw new Error('overlay source must be clean');
  const tool=JSON.parse(await command(go,['env','-json','GOROOT','GOVERSION','GOHOSTOS','GOHOSTARCH']));
  if(tool.GOVERSION!=='go1.26.0'||tool.GOHOSTOS!=='windows'||tool.GOHOSTARCH!=='amd64'||!path.isAbsolute(tool.GOROOT))throw new Error('matching native Go1.26.0 required');
  const generatedGo=path.join(outputRoot,'query-regions.go'), header=path.join(outputRoot,'query-regions.h');
  const emitter=path.join(sourceRoot,'scripts/emit-windows-query-regions.mjs');
  await original(emitter);
  await original(path.join(sourceRoot,'scripts/generate-windows-query-regions.mjs'));
  await original(path.join(sourceRoot,'scripts/native-owner-build-plan.mjs'));
  const emitted=JSON.parse(await command(process.execPath,[emitter,queryProfile,generatedGo,header]));
  if(emitted.profileSHA256!==createHash('sha256').update(profileBytes).digest('hex')||emitted.sourceCommit!==commit)throw new Error('emitter consumed different query original');
  const mappings=windowsOwnedOverlayMappings(tool.GOROOT,sourceRoot,generatedGo);
  for(const row of mappings)await original(row.backing);
  await original(header);
  const overlay=path.join(outputRoot,'overlay.json');
  const overlayBytes=JSON.stringify({Replace:Object.fromEntries(mappings.map(row=>[row.target,row.backing]))});
  await writeFile(overlay,overlayBytes,{flag:'wx'});
  await original(overlay);
  // Actual complete package execution, including all six mapped test sources.
  // count=1 forbids cached successes; JSON makes missing tests a failed gate.
  const output=await command(go,['test','-json','-count=1','-overlay='+overlay,'internal/runtime/syscall/windows','os']);
  const events=output.split(/\r?\n/u).filter(Boolean).map(line=>JSON.parse(line));
  for(const name of ['TestServiceLassoFixedPublicArguments','TestServiceLassoCreatorPERepresentation',
    'TestCompletedDecodeBindingRetainsZeroPublicArgumentsAndRefusesMismatches',
    'TestServiceLassoPrivateConstructorKeepsActualBacking','TestServiceLassoPrivateUnsignedBoundaries','TestServiceLassoNativeBOOLWidth']) {
    if(!events.some(event=>event.Package==='internal/runtime/syscall/windows'&&event.Test===name&&event.Action==='pass'))throw new Error('owned overlay test missing: '+name);
  }
  for(const name of ['internal/runtime/syscall/windows','os'])if(!events.some(event=>event.Package===name&&!event.Test&&event.Action==='pass'))throw new Error('GOROOT package did not pass: '+name);
  record.state='passed-matching-host-goroot-tests';
} catch(error) {
  record.failure=String(error);
  throw error;
} finally {
  // Exact environment, profile/path/source inputs and raw streams stay private
  // with this attempt. A public closed summary does not claim durable custody.
  await writeFile(path.join(outputRoot,'attempt.json'),JSON.stringify(record,null,2)+'\n',{flag:'wx'});
  await writeFile(path.join(outputRoot,'public-summary.json'),JSON.stringify({
    kind:'windows-owned-goroot-test-summary', state:record.state,
    sourceCommit:record.sourceCommit??null,
    privateOriginals:'retained-in-runner-attempt-only',
    qualifiedNativeOperatorAcceptance:false,
  })+'\n',{flag:'wx'});
}
