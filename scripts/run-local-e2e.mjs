#!/usr/bin/env node
// Ordinary source/backend E2E. Never a native profile or publication receipt.
import {spawn} from 'node:child_process';
import {createHash, randomBytes} from 'node:crypto';
import {mkdtemp, mkdir, readFile, writeFile, stat, lstat, realpath} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import path from 'node:path';
import net from 'node:net';
import {fileURLToPath} from 'node:url';
import {createInterface} from 'node:readline';

export function parseOptions(argv) {
  const options={};
  const names=new Set(['core-dir','expected-core-revision','core-fixture-helper','output-dir','go','suite']);
  for(let i=0;i<argv.length;i+=2){const name=argv[i]?.replace(/^--/,'');if(!names.has(name)||!argv[i+1]||Object.hasOwn(options,name))throw new Error('invalid E2E arguments');options[name]=argv[i+1];}
  options.suite??='all';
  if(!options['core-dir']||!/^[a-f0-9]{40}$/.test(options['expected-core-revision']??'')||!['compat','v2','all'].includes(options.suite))throw new Error('explicit Core checkout/revision and suite required');
  return options;
}
export async function validateConnection(file) {
  const info=await stat(file);if(!info.isFile()||(process.platform!=='win32'&&(info.mode&0o077)))throw new Error('connection must be private');
  const value=JSON.parse(await readFile(file,'utf8'));const url=new URL(value.url);
  if(url.protocol!=='http:'||url.hostname!=='127.0.0.1'||url.username||url.password||url.search||url.hash||url.pathname!=='/'||typeof value.token!=='string'||value.token.length<24)throw new Error('invalid isolated backend connection');
  return value;
}
export function suitePassed(suite,results){return (suite==='compat'?['compat']:suite==='v2'?['v2']:['compat','v2']).every(mode=>results.some(row=>row.mode===mode&&row.status==='passed'));}
function digest(bytes){return createHash('sha256').update(bytes).digest('hex');}
export function assertSourceReceipt(receipt,revision){
 const hex=x=>/^[a-f0-9]{64}$/.test(x??'');
 if(receipt.schemaVersion!==1||receipt.coreRevision!==revision||receipt.classification!=='source-built-ordinary-smoke'||receipt.cleanSource!==true||!/^[a-f0-9]{40}$/.test(receipt.coreTree??'')||!hex(receipt.coreArtifactSHA256)||!hex(receipt.sourceInputSHA256)||!hex(receipt.artifactInventorySHA256)||receipt.initializationAPI!==true||receipt.build?.exitCode!==0||receipt.build?.signal!==null||JSON.stringify(receipt.build?.command)!==JSON.stringify(['npm','run','build'])||!receipt.tools?.node||!receipt.tools?.npm||!receipt.tools?.typescript||!hex(receipt.tools?.compilerSHA256)||!Object.keys(receipt.installedDependencies??{}).length)throw new Error('source-qualified backend receipt required');
}
export async function assertReadyPaths(ready,backendRoot){
 const root=await realpath(backendRoot);if((await lstat(backendRoot)).isSymbolicLink())throw new Error('backend root link');
 for(const [key,name] of [['connectionFile','connection.json'],['receiptFile','receipt.json']]){const expected=path.join(root,name);if(path.resolve(ready[key]??'')!==expected||(await lstat(expected)).isSymbolicLink()||await realpath(expected)!==expected)throw new Error('ready path escaped owned backend');}
}
function waitBound(promise,ms){return new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(new Error('deadline')),ms);promise.then(x=>{clearTimeout(timer);resolve(x);},e=>{clearTimeout(timer);reject(e);});});}
async function stopBound(child,closed){
 try{return await waitBound(closed,20000);}catch{}
 for(const signal of ['SIGINT','SIGTERM','SIGKILL']){child.kill(signal);try{return {...await waitBound(closed,10000),forced:true};}catch{}}
 throw new Error('owned process cleanup unresolved');
}
export async function command(argv,{cwd,env,log,timeout=120000}){
 const child=spawn(argv[0],argv.slice(1),{cwd,env,stdio:['ignore','pipe','pipe']});const chunks=[];let size=0,error,overflow=false;
 const capture=x=>{size+=x.length;if(size<=8*1024*1024)chunks.push(x);else overflow=true;};child.stdout.on('data',capture);child.stderr.on('data',capture);child.on('error',x=>{error=x;});
 const closed=new Promise(resolve=>child.on('close',(code,signal)=>resolve({code,signal})));let outcome,timedOut=false;
 try{outcome=await waitBound(closed,timeout);}catch{timedOut=true;child.kill('SIGINT');outcome=await stopBound(child,closed);}
 await writeFile(log,Buffer.concat(chunks),{mode:0o600});if(error||overflow||timedOut||outcome.forced||outcome.signal||outcome.code!==0)throw new Error('E2E command failed');return Buffer.concat(chunks).toString();
}
async function portClosed(url){return await new Promise(resolve=>{const socket=net.connect({host:'127.0.0.1',port:Number(new URL(url).port)});socket.once('connect',()=>{socket.destroy();resolve(false);});socket.once('error',error=>resolve(error.code==='ECONNREFUSED'));socket.setTimeout(1000,()=>{socket.destroy();resolve(false);});});}
export async function run(options){
  const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
  const output=options['output-dir']?path.resolve(options['output-dir']):await mkdtemp(path.join(tmpdir(),'tui-local-e2e-'));
  if(options['output-dir'])await mkdir(output,{mode:0o700});
  const env={...process.env,GOWORK:'off',GOFLAGS:'',GOTOOLCHAIN:'local'};
  const go=options.go??'go';const stages=[];let backend,connectionFile,connection,stopPromise,coreReceipt;let stderr=[];
  const summary={schemaVersion:1,suite:options.suite,scope:'ordinary-source-real-backend',status:'failed',stages,platform:process.platform,nativeQualified:false};
  try{
    if(process.platform==='win32'){summary.status='blocked';summary.reason='matching-host ConPTY adapter required';return summary;}
    const revision=(await command(['git','rev-parse','HEAD'],{cwd:repo,env,log:path.join(output,'revision.private.log')})).trim();
    if((await command(['git','status','--porcelain'],{cwd:repo,env,log:path.join(output,'status.private.log')})).trim())throw new Error('clean TUI checkout required');
    const version=(await command([go,'version'],{cwd:repo,env,log:path.join(output,'go-version.private.log')})).trim();if(!version.startsWith('go version go1.26.0 '))throw new Error('Go1.26.0 required');
    const binary=path.join(output,'service-lasso-tui');await command([go,'build','-mod=readonly','-buildvcs=true','-o',binary,'./cmd/service-lasso-tui'],{cwd:repo,env,log:path.join(output,'build.private.log')});
    const metadata=await command([go,'version','-m',binary],{cwd:repo,env,log:path.join(output,'build-metadata.private.log')});if(!metadata.includes(`vcs.revision=${revision}`)||!metadata.includes('vcs.modified=false'))throw new Error('source provenance mismatch');
    summary.tuiRevision=revision;summary.tuiArtifactSHA256=digest(await readFile(binary));stages.push({stage:'source-build',status:'passed'});
    const helper=path.resolve(options['core-fixture-helper']??path.join(options['core-dir'],'scripts/operator-e2e-fixture.mjs'));
    const backendOutput=path.join(output,'backend');
    backend=spawn(process.execPath,[helper,'--core-dir',path.resolve(options['core-dir']),'--expected-core-revision',options['expected-core-revision'],'--output-dir',backendOutput,'--build-core'],{stdio:['pipe','pipe','pipe']});
    stderr=[];let stderrSize=0,stderrOverflow=false;backend.stderr.on('data',x=>{stderrSize+=x.length;if(stderrSize<=8*1024*1024)stderr.push(x);else stderrOverflow=true;});let backendError;
    backend.on('error',e=>{backendError=e;});backend.stdin.on('error',e=>{backendError=e;});
    stopPromise=new Promise(resolve=>backend.on('close',(code,signal)=>resolve({code,signal,error:Boolean(backendError)||stderrOverflow})));
    const lines=createInterface({input:backend.stdout});
    const ready=await new Promise((resolve,reject)=>{
      const timer=setTimeout(()=>reject(new Error('backend readiness deadline')),180000);
      lines.on('line',line=>{try{const row=JSON.parse(line);if(row.ready===true){clearTimeout(timer);resolve(row);}}catch{}});
      backend.on('close',()=>{clearTimeout(timer);reject(new Error('backend closed before ready'));});
      backend.on('error',()=>{clearTimeout(timer);reject(new Error('backend spawn failed'));});
    });
    await assertReadyPaths(ready,backendOutput);coreReceipt=ready.receiptFile;
    const receipt=JSON.parse(await readFile(coreReceipt,'utf8'));assertSourceReceipt(receipt,options['expected-core-revision']);
    for(const [file,field] of [['source-inputs.json','sourceInputSHA256'],['artifact-inventory.json','artifactInventorySHA256']]){const bytes=await readFile(path.join(backendOutput,file),'utf8');if(digest(JSON.stringify(JSON.parse(bytes)))!==receipt[field])throw new Error('backend inventory correspondence failed');}
    connectionFile=ready.connectionFile;connection=await validateConnection(connectionFile);if(receipt.url!==connection.url)throw new Error('backend connection mismatch');
    summary.coreRevision=receipt.coreRevision;summary.coreArtifactSHA256=receipt.coreArtifactSHA256;
    const identities=[];
    for(const token of [connection.token,randomBytes(32).toString('hex')]){const response=await fetch(new URL('/api/runtime/security',connection.url),{headers:{'x-service-lasso-admin-token':token},signal:AbortSignal.timeout(5000)});const value=await response.json();identities.push({status:response.status,kind:value.auth?.actor?.kind,authenticated:value.auth?.actor?.authenticated,configured:value.auth?.policy?.localTokenConfigured});}
    if(identities[0].status!==200||identities[0].kind!=='local-token'||identities[0].authenticated!==true||identities[0].configured!==true||identities[1].kind==='local-token')throw new Error('token identity expectation failed');
    stages.push({stage:'token-identity',status:'passed',validKind:identities[0].kind,invalidKind:identities[1].kind??'denied'});
    stages.push({stage:'permission-denial',status:'blocked',reason:'guarded OAuth lifecycle fixture required; loopback local-root fallback is not access denial'});
    const modes=options.suite==='all'?['compat','v2']:[options.suite];
    for(const mode of modes){
      try{await command(['python3',path.join(repo,'scripts/local-e2e-pty.py'),'--binary',binary,'--connection-file',connectionFile,'--output',path.join(output,mode),'--mode',mode,'--observations',path.join(backendOutput,'request-observations.json')],{cwd:repo,env,timeout:60000,log:path.join(output,`${mode}.private.log`)});}catch{/* Read the closed stage result; no raw output enters public summary. */}
      const result=JSON.parse(await readFile(path.join(output,mode,'result.json'),'utf8'));stages.push(result);
    }
    summary.status=suitePassed(options.suite,stages)?'passed':'blocked';
  }catch{summary.status='failed';summary.reason='stage-failed-see-private-evidence';}
  finally{
    if(backend){backend.stdin.end('stop\n');try{const outcome=await stopBound(backend,stopPromise);await writeFile(path.join(output,'backend-stderr.private.log'),Buffer.concat(stderr),{mode:0o600});const closed=JSON.parse(await readFile(coreReceipt??path.join(output,'backend','receipt.json'),'utf8'));const cleanup=closed.cleanup;const verified=outcome.code===0&&!outcome.signal&&!outcome.error&&!outcome.forced&&closed.status==='passed'&&closed.stage==='closed'&&cleanup?.status==='complete'&&cleanup.serverClosed===true&&cleanup.portClosed===true&&cleanup.parentPortClosed===true&&cleanup.credentialRemoved===true&&cleanup.forcedTermination===false&&(!connection||await portClosed(connection.url));stages.push({stage:'backend-stop',status:verified?'passed':'failed'});if(!verified)summary.status='failed';}catch{stages.push({stage:'backend-stop',status:'failed'});summary.status='failed';}}
    if(connectionFile){try{await stat(connectionFile);summary.credentialRemoved=false;summary.status='failed';}catch(e){summary.credentialRemoved=e.code==='ENOENT';if(!summary.credentialRemoved)summary.status='failed';}}
    // Defend against accidental token inclusion in any public projection.
    if(connection&&JSON.stringify(summary).includes(connection.token))throw new Error('private token entered summary');
    await writeFile(path.join(output,'summary.json'),JSON.stringify(summary,null,2)+'\n',{mode:0o600});
  }
  return summary;
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
  try{const result=await run(parseOptions(process.argv.slice(2)));process.stdout.write(JSON.stringify(result)+'\n');process.exitCode=result.status==='passed'?0:2;}
  catch{process.stderr.write('Local E2E setup failed; consult private evidence.\n');process.exitCode=2;}
}
