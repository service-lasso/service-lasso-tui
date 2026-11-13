import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { mkdtemp, rmdir } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { observeNativeProducerChild, nativeProducerClosedSuccessfully } from "./native-producer-child.mjs";

// These are actual child lifecycle regression sources, not installed Core/TUI
// or five-action native acceptance. They remain unrun until source admission.
test("original producer close owns actual successful child and pipes",async()=>{
 const child=spawn(process.execPath,["-e","process.stdin.resume();process.stdin.on('end',()=>{process.stdout.write('observed output');process.stderr.write('private diagnostic');});"],{stdio:["pipe","pipe","pipe"]});
 const owner=observeNativeProducerChild(child);child.stdout.resume();child.stderr.resume();child.stdin.end();const closed=await owner.completion;
 assert.equal(closed.child,child);assert.equal(closed,owner);assert.equal(nativeProducerClosedSuccessfully(closed),true);assert.equal(closed.pipes.length,3);
 assert.equal(closed.pipes.find(pipe=>pipe.name==="stdin").finished,true);
 assert.equal(closed.pipes.find(pipe=>pipe.name==="stdout").ended,true);
 assert.equal(closed.pipes.find(pipe=>pipe.name==="stderr").ended,true);
 assert.equal(nativeProducerClosedSuccessfully({...closed}),false);assert.throws(()=>{closed.exitCode=17;});assert.throws(()=>{closed.pipes[0].closed=false;});assert.throws(()=>observeNativeProducerChild(child));assert.equal(nativeProducerClosedSuccessfully(closed),true);
});

for(const name of ["stdout","stderr","stdio[3]"]){
 test(`zero child exit cannot replace original ${name} EOF after local destroy`,async()=>{
  const child=spawn(process.execPath,["-e","process.stdin.resume();process.stdin.on('end',()=>process.exit(0));"],{stdio:name==="stdio[3]"?["pipe","pipe","pipe","pipe"]:["pipe","pipe","pipe"]});
  const owner=observeNativeProducerChild(child);
  const selected=name==="stdio[3]"?child.stdio[3]:child[name];selected.destroy();
  for(const stream of [child.stdout,child.stderr])if(stream!==selected)stream.resume();
  child.stdin.end();
  const closed=await owner.completion;
  const destroyed=closed.pipes.find(pipe=>pipe.name===name);
  assert.equal(closed.exitObserved,true);assert.equal(closed.exitCode,0);
  assert.equal(closed.closeObserved,true);assert.equal(destroyed.closed,true);
  assert.equal(destroyed.closeCount,1);assert.equal(destroyed.ended,false);assert.equal(nativeProducerClosedSuccessfully(closed),false);
 });
}
test("original failed exit remains failed after actual close",async()=>{
 const child=spawn(process.execPath,["-e","process.exitCode=17"],{stdio:["pipe","pipe","pipe"]});const owner=observeNativeProducerChild(child);child.stdout.resume();child.stderr.resume();child.stdin.end();const closed=await owner.completion;
 assert.equal(closed.exitObserved,true);assert.equal(closed.exitCode,17);assert.equal(closed.closeCode,17);assert.equal(nativeProducerClosedSuccessfully(closed),false);
});

test("actual extra pipe destroy retains its original error through child close",async()=>{
  const child=spawn(process.execPath,["-e","process.stdin.resume();"],{stdio:["pipe","pipe","pipe","pipe"]});
  const owner=observeNativeProducerChild(child),original=new Error("controlled extra pipe close");
  child.stdio[3].destroy(original);child.stdout.resume();child.stderr.resume();child.stdin.end();
  const closed=await owner.completion,pipe=closed.pipes.find(row=>row.stream===child.stdio[3]);
  assert.deepEqual(pipe.errors,[original]);assert.deepEqual(closed.errors,[original]);
  assert.equal(pipe.closeCount,1);assert.equal(pipe.ended,false);
  assert.equal(closed.exitCode,0);assert.equal(nativeProducerClosedSuccessfully(closed),false);
});
test("actual failed launch does not fabricate an exit observation",async()=>{
 const root=await mkdtemp(path.join(os.tmpdir(),"sl-tui-producer-child-"));try{
  const child=spawn(path.join(root,"absent-executable"),[],{stdio:["pipe","pipe","pipe"]});const owner=observeNativeProducerChild(child);child.stdout.resume();child.stderr.resume();child.stdin.end();const closed=await owner.completion;
  assert.equal(closed.child,child);assert.equal(closed.closeObserved,true);assert.equal(closed.exitObserved,false);assert.ok(closed.errors.length>0);assert.equal(nativeProducerClosedSuccessfully(closed),false);
 }finally{await rmdir(root);}
});

for(const name of ["child","stdin","stdout","stderr"]){
 test(`original ${name} retains every repeated opaque and falsy error occurrence`,async()=>{
  const child=spawn(process.execPath,["-e","process.stdin.resume();process.stdin.on('end',()=>process.exit(0));"],{stdio:["pipe","pipe","pipe"]});
  const owner=observeNativeProducerChild(child);const emitter=name==="child"?child:child[name];
  const opaque=Object.freeze({privateOriginal:"never render"});const originals=[opaque,opaque,undefined,null,false,0,""];
  for(const original of originals)emitter.emit("error",original);
  assert.deepEqual(owner.errors,originals);
  if(name!=="child")assert.deepEqual(owner.pipes.find(pipe=>pipe.name===name).errors,originals);
  child.stdout.resume();child.stderr.resume();child.stdin.end();const closed=await owner.completion;
  assert.equal(closed,owner);assert.equal(closed.exitCode,0);assert.equal(closed.closeCode,0);
  assert.deepEqual(closed.errors,originals);assert.equal(nativeProducerClosedSuccessfully(closed),false);
 });
}
