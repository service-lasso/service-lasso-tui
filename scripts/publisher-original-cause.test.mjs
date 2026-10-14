import test from "node:test";
import assert from "node:assert/strict";
import { providerJSON, writePublicationJournalHandle } from "./publish-scoped-candidate.mjs";
import { providerJSON as compatibilityProviderJSON } from "./publish-candidate.mjs";

// Source-authored cases remain UNRUN until separate exact-command admission.
test("provider fetch original remains private behind fixed publication failure",async()=>{
 const original=new Error("private credential-bearing URL and native owner");
 for(const provider of [providerJSON,compatibilityProviderJSON])await assert.rejects(provider(async()=>{throw original;},"private-token","/immutable-releases"),error=>{
  assert.equal(error.message,"provider request or metadata denied");
  assert.equal(error.cause,original);
  assert.equal(JSON.stringify(error),"{}");
  assert.equal(String(error).includes(original.message),false);
  return true;
 });
});

test("provider streamed body original is retained rather than replaced",async()=>{
 const original=new Error("private response stream capture");
 const fetchImpl=async()=>({status:200,headers:{get:()=>null},body:{async *[Symbol.asyncIterator](){yield Buffer.from("{");throw original;}}});
 for(const provider of [providerJSON,compatibilityProviderJSON])await assert.rejects(provider(fetchImpl,"private-token","/immutable-releases"),error=>error.cause===original&&error.message==="provider request or metadata denied");
});

test("provider thrown undefined is still an actual retained cause",async()=>{
 await assert.rejects(providerJSON(async()=>{throw undefined;},"private-token","/immutable-releases"),error=>{
  assert.equal(Object.hasOwn(error,"cause"),true);
  assert.equal(error.cause,undefined);
  assert.equal(error.message,"provider request or metadata denied");
  return true;
 });
});

test("journal primary and close originals survive together without public leakage",async()=>{
 for(const phase of ["write","sync"]){
  const primary=new Error(`private-${phase}-original`),cleanup=new Error("private-close-original");const calls=[];
  const handle={async writeFile(content){calls.push(["write",content]);if(phase==="write")throw primary;},async sync(){calls.push(["sync"]);throw primary;},async close(){calls.push(["close"]);throw cleanup;}};
  await assert.rejects(writePublicationJournalHandle(handle,"original-journal-bytes"),error=>{
   assert.equal(error.message,"publication journal write or close denied");
   assert.deepEqual(error.cause.errors,[primary,cleanup]);
   assert.equal(JSON.stringify(error),"{}");
   assert.equal(String(error).includes("private-"),false);
   return true;
  });
  assert.deepEqual(calls,phase==="write"?[["write","original-journal-bytes"],["close"]]:[["write","original-journal-bytes"],["sync"],["close"]]);
 }
});
