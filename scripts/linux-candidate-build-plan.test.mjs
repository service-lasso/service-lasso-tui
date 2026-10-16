// UNRUN source fixtures; no admission or native qualification is supplied.
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import {linuxCandidateBuildPlan} from './native-owner-build-plan.mjs';

function fixture(){return Object.fromEntries(['go','node','compiler','sourceRoot','profileOriginal','abiGeometryOriginal','nativeGeometryOriginal','generatedGo','generatedEntry','overlay','payload','objectDirectory','ownerOutput'].map(key=>[key,path.resolve('.tmp/fixture',key)]));}
test('candidate source test and payload select the same held genuine-source lane',()=>{
 const input=fixture(),plan=linuxCandidateBuildPlan(input);
 for(const command of [plan.test,plan.payload]){
  assert.ok(command.argv.includes('-tags=service_lasso_native_candidate'));
  assert.ok(command.argv.includes('-overlay='+input.overlay));
  assert.ok(command.argv.includes('-mod=readonly'));
 }
 assert.equal(plan.overlayMappings[0].backing,input.generatedGo);
 assert.equal(plan.owner.generatedEntry,input.generatedEntry);
 assert.equal(plan.emit.command,input.node);
 assert.deepEqual(plan.emit.argv,[path.join(input.sourceRoot,'scripts/emit-linux-candidate-profile.mjs'),input.profileOriginal,input.abiGeometryOriginal,input.nativeGeometryOriginal,input.generatedGo,input.generatedEntry]);
 assert.ok(plan.order.indexOf('emit-paired-candidate-source')<plan.order.indexOf('test-candidate-source'));
 assert.equal(plan.environment.GOWORK,'off');
 assert.equal(plan.sourceInputs.generatedGo,input.generatedGo);
 assert.equal(plan.link.output,input.ownerOutput);
 assert.ok(plan.prerequisites.includes('new-complete-input-root'));
});
test('missing and aliased held originals refuse before a plan is produced',()=>{
 const input=fixture();
 for(const key of Object.keys(input))assert.throws(()=>linuxCandidateBuildPlan({...input,[key]:''}));
 for(const key of ['profileOriginal','abiGeometryOriginal','nativeGeometryOriginal','generatedGo','generatedEntry','overlay','payload','objectDirectory'])assert.throws(()=>linuxCandidateBuildPlan({...input,[key]:input.ownerOutput}));
});
