import test from "node:test";
import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { readNativeProducerHandoff } from "./native-producer-handoff.mjs";
import { observeNativeProducerChild } from "./native-producer-child.mjs";

// Real original child/pipe source cases; not Core, descendant, native driver,
// installed archive or whole pipeline deadline acceptance.
async function handoff(program,check) {
  const child=spawn(process.execPath,["--input-type=module","-e",program],{stdio:["pipe","pipe","pipe"]});
  const owner=observeNativeProducerChild(child);
  try{await check(child,owner);}
  finally{child.stdin.end();await owner.completion;}
}
test("handoff preserves original owner listeners and exact ready bytes",async()=>{
  await handoff('process.stdout.write(JSON.stringify({event:"input-held",writerPID:process.pid})+"\\n");process.stdin.resume();',async(child,owner)=>{
    const ready=await readNativeProducerHandoff(child);
    assert.equal(ready.event,"input-held");assert.equal(ready.writerPID,child.pid);
    assert.equal(owner.closeObserved,false);assert.equal(child.listenerCount("exit"),1);
    assert.equal(child.stdout.listenerCount("data"),0);
  });
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
  await handoff('import {closeSync} from "node:fs";closeSync(1);process.stdin.resume();',async(child,owner)=>{
    await assert.rejects(readNativeProducerHandoff(child),/input writer stream ended/);
    assert.equal(owner.closeObserved,false);assert.equal(child.stdout.listenerCount("data"),0);
  });
});
