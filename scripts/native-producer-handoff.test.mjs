import test from "node:test";
import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { readNativeProducerHandoff } from "./native-producer-handoff.mjs";
import { observeNativeProducerChild, nativeProducerClosedSuccessfully } from "./native-producer-child.mjs";

// Real original child/pipe source cases; not Core, descendant, native driver,
// installed archive or whole pipeline deadline acceptance.
async function handoff(program,check,extraPipe=false) {
  const child=spawn(process.execPath,["--input-type=module","-e",program],{stdio:extraPipe?["pipe","pipe","pipe","pipe"]:["pipe","pipe","pipe"]});
  const extraOriginals=child.stdio.slice(3);
  const owner=observeNativeProducerChild(child);
  try{await check(child,owner);}
  finally{
    child.stdout.resume();child.stderr.resume();
    for(const pipe of extraOriginals){if(pipe.readable)pipe.resume();if(pipe.writable&&!pipe.writableEnded)pipe.end();}
    child.stdin.end();await owner.completion;
  }
  return owner;
}
test("handoff preserves original owner listeners and exact ready bytes",async()=>{
  await handoff('process.stdout.write(JSON.stringify({event:"input-held",writerPID:process.pid})+"\\n");process.stdin.resume();',async(child,owner)=>{
    const ready=await readNativeProducerHandoff(child);
    assert.equal(ready.event,"input-held");assert.equal(ready.writerPID,child.pid);
    assert.equal(owner.closeObserved,false);assert.equal(child.listenerCount("exit"),1);
    assert.equal(child.stdout.listenerCount("data"),0);
  });
});

test("explicit actual extra pipe shares handoff while all original pipes close once",async()=>{
  const closed=await handoff('import {writeSync} from "node:fs";writeSync(3,JSON.stringify({event:"input-held",writerPID:process.pid})+"\\n");process.stdin.resume();',async(child,owner)=>{
    const pipe=child.stdio[3],ready=await readNativeProducerHandoff(child,pipe);
    assert.equal(ready.event,"input-held");assert.equal(ready.writerPID,child.pid);
    assert.equal(owner.exitObserved,false);assert.equal(owner.closeObserved,false);
    assert.equal(pipe.listenerCount("data"),0);
    assert.equal(new Set(owner.pipes.map(row=>row.stream)).size,owner.pipes.length);
    assert.deepEqual(owner.pipes.find(row=>row.stream===child.stdout).indices,[1]);
    assert.deepEqual(owner.pipes.find(row=>row.stream===pipe).indices,[3]);
  },true);
  assert.equal(nativeProducerClosedSuccessfully(closed),true);
  assert.ok(closed.pipes.every(pipe=>pipe.closeCount===1));
});
test("oversize handoff rejects before concatenating beyond quota",async()=>{
  await handoff('process.stdout.write("x".repeat(1048577));process.stdin.resume();',async child=>{
    await assert.rejects(readNativeProducerHandoff(child),error=>error.message==="input handoff quota"&&error.cause.limitBytes===1048576&&error.cause.retainedBytes<=1048576&&error.cause.receivedChunkBytes>1048576-error.cause.retainedBytes);
    assert.equal(child.stdout.listenerCount("data"),0);
  });
});
test("malformed handoff retains parser cause and removes readiness listener",async()=>{
  await handoff('process.stdout.write("{bad}\\n");process.stdin.resume();',async child=>{
    await assert.rejects(readNativeProducerHandoff(child),error=>error.message==="input handoff shape"&&error.cause instanceof Error);
    assert.equal(child.stdout.listenerCount("data"),0);
  });
});
test("natural EOF without a record is not a ready or close certificate",async()=>{
  // Windows libuv fs close deliberately leaves fd0..2 open. Use an actually
  // owned inherited fd3 there; other hosts retain the original stdout route.
  const windows=process.platform==="win32";
  const closed=await handoff(`import {closeSync} from "node:fs";closeSync(${windows?3:1});process.stdin.resume();`,async(child,owner)=>{
    const pipe=windows?child.stdio[3]:child.stdout;
    await assert.rejects(windows?readNativeProducerHandoff(child,pipe):readNativeProducerHandoff(child),/input writer stream ended/);
    assert.equal(owner.pipes.find(row=>row.stream===pipe).ended,true);
    assert.equal(owner.exitObserved,false);
    assert.equal(owner.closeObserved,false);assert.equal(pipe.listenerCount("data"),0);
  },windows);
  // A real end/CRT-close error cannot be hidden by a preceding EOF: it must
  // survive as a failed child/pipe observation after releasing the live stdin.
  assert.equal(nativeProducerClosedSuccessfully(closed),true);
  assert.ok(closed.pipes.every(pipe=>pipe.closeCount===1));
});

test("handoff rejects foreign absent stdin and late-added pipes before listeners",async()=>{
  await handoff('process.stdin.resume();',async(child,owner)=>{
    await handoff('process.stdin.resume();',async foreign=>{
      const pipe=foreign.stdio[3],before=pipe.listenerCount("data"),exitListeners=child.listenerCount("exit");
      const original=child.stdio[3];child.stdio[3]=pipe;
      try{
        for(const selected of [pipe,undefined,null,child.stdin,0,1,3,-1,{},owner.pipes[0]]){
          await assert.rejects(readNativeProducerHandoff(child,selected),/input handoff pipe original/);
          assert.equal(child.listenerCount("exit"),exitListeners);
        }
        assert.equal(pipe.listenerCount("data"),before);
      }finally{child.stdio[3]=original;}
    },true);
  });
});
