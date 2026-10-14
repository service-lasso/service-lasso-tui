import test from "node:test";
import assert from "node:assert/strict";
import { PublicationError, withPublicationCleanup } from "./publication-failure.mjs";
import { streamResponseToHandle } from "./verify-scoped-candidate-publication.mjs";
import { streamResponseToHandle as compatibilityStreamResponseToHandle } from "./verify-candidate-publication.mjs";

// Authored source cases only; no native/provider/capture qualification or run.
test("publication operation and cleanup retain both actual originals",async()=>{
 const primary=new Error("private operation bytes"),cleanup=new Error("private native cleanup owner");const calls=[];
 await assert.rejects(withPublicationCleanup(async()=>{calls.push("operation");throw primary;},async()=>{calls.push("cleanup");throw cleanup;},"candidate verification and cleanup denied"),error=>{
  assert.deepEqual(error.cause.errors,[primary,cleanup]);assert.equal(error.message,"candidate verification and cleanup denied");assert.equal(JSON.stringify(error),"{}");return true;
 });
 assert.deepEqual(calls,["operation","cleanup"]);
 const semantic=new PublicationError("candidate checksum mismatch");
 await assert.rejects(withPublicationCleanup(async()=>{throw semantic;},async()=>{},"candidate verification denied"),error=>error===semantic);
 const originalBytes=Buffer.from("private successful read bytes");
 await assert.rejects(withPublicationCleanup(async()=>originalBytes,async()=>{throw cleanup;},"candidate read and close denied"),error=>error.cause===cleanup&&error.operationResult===originalBytes&&JSON.stringify(error)==="{}");
});

test("actual streamed bytes produce the useful receipt after successful close",async()=>{
 for(const stream of [streamResponseToHandle,compatibilityStreamResponseToHandle]){
 const writes=[];let closes=0;
 const response={headers:{get:()=>"5"},body:{async *[Symbol.asyncIterator](){yield Buffer.from("he");yield Buffer.from("llo");}}};
 const handle={async write(bytes){writes.push(bytes);return {bytesWritten:bytes.length,buffer:bytes};},async close(){closes++;}};
 assert.deepEqual(await stream(response,handle,5),{size:5,sha256:"2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"});
 assert.equal(Buffer.concat(writes).toString(),"hello");assert.equal(closes,1);
 }
});

test("stream original and close original survive instead of an accepted receipt",async()=>{
 for(const stream of [streamResponseToHandle,compatibilityStreamResponseToHandle]){
 const primary=new Error("private stream original"),cleanup=new Error("private close original");let closes=0;
 const response={headers:{get:()=>null},body:{async *[Symbol.asyncIterator](){yield Buffer.from("he");throw primary;}}};
 const handle={async write(bytes){return {bytesWritten:bytes.length,buffer:bytes};},async close(){closes++;throw cleanup;}};
 await assert.rejects(stream(response,handle,5),error=>{
  assert.deepEqual(error.cause.errors,[primary,cleanup]);assert.equal(JSON.stringify(error),"{}");assert.equal(error.message,"candidate asset stream and close denied");return true;
 });assert.equal(closes,1);
 }
});

test("partial native write retains actual observation and refuses a receipt",async()=>{
 for(const stream of [streamResponseToHandle,compatibilityStreamResponseToHandle]){
 const bytes=Buffer.from("hello"),observation={bytesWritten:1,buffer:bytes};let closes=0;
 const response={headers:{get:()=>null},body:{async *[Symbol.asyncIterator](){yield bytes;}}};
 const handle={async write(){return observation;},async close(){closes++;}};
 await assert.rejects(stream(response,handle,5),error=>error.cause===observation&&error.message==="public asset write incomplete"&&JSON.stringify(error)==="{}");
 assert.equal(closes,1);
 }
});
