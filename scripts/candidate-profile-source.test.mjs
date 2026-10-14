// UNRUN source fixtures. Synthetic evidence is never human authorization.
import test from 'node:test';
import assert from 'node:assert/strict';
import {decodeCandidateProfile,encodeCandidateProfileSections,emitCandidateProfileSources} from './candidate-profile-source.mjs';
function nativeFixture(){
 const scalar='scheduler_interval_ns services outer_observations deadline_roots deadline_observations deadline_polls deadline_poll_fds creator_file_bytes creator_file_reads collector_joins requests prefix_attempts terminal_reply_attempts input_frames input_steps body_bytes scan_frames scan_bytes fd_views fd_actions membership_frames membership_bytes membership_path mountinfo_bytes mountinfo_attempts mountinfo_rows mount_location_bytes selinux_rows selinux_path policy_frames policy_frame_bytes policy_body parser_bytes parser_attempts diagnostics diagnostic_arguments diagnostic_bytes diagnostic_format self_context_bytes domain_types domain_steps access_visits access_helpers access_strings reach_visits reach_helpers reach_strings reach_steps children_frames children_bytes children_views executable_path directory_path image_frames image_bytes image_writes image_readbacks credential_status_bytes credential_status_attempts credential_group_capacity pidfd_info_bytes pidfd_info_attempts public_argument_count public_argument_bytes public_environment_count public_environment_bytes security_label_bytes terminal_calls maintenance_role_rows archive_path_bytes archive_declarations'.split(' ');
 const result=Object.fromEntries(scalar.map(key=>[key,'1']));result.requests='2';for(const key of ['raw_frames','public_writes','forward_waits','forward_iterations'])result[key]=['1','2','3'];result.custody_attempts=['1','2','3','4','5'];result.final_io_attempts=['1','2'];return result;
}
function integer(value){const bytes=Buffer.alloc(8);bytes.writeBigUInt64LE(value);return bytes;}
function records(rows){const parts=[integer(BigInt(rows.length))];for(const row of rows){parts.push(...row.map(value=>integer(BigInt(value.length))),...row);}return Buffer.concat(parts);}
function fixture(){
 const evidence=records([[...['source','author-fixture','subject','scope','conditions','meaning','original-statement'].map(value=>Buffer.from(value))]]);
 const role=records([[integer(0n),integer(0n),integer(8n),integer(1n),integer(1n),integer(1n),Buffer.from('actor fixture'),Buffer.from('preservation fixture')]]);
 const policy=Buffer.alloc(72);policy.writeBigUInt64LE(1n,32);
 return encodeCandidateProfileSections([Buffer.alloc(32,1),Buffer.alloc(32,2),policy,Buffer.concat([integer(1n),integer(1n),integer(1n)]),evidence,role,records([]),records([]),records([])].map((bytes,i)=>({id:i+1,bytes})));
}
test('closed profile retains SAME original evidence spans and acyclic identities',()=>{
 const original=fixture(),parsed=decodeCandidateProfile(original);assert.equal(parsed.original,original);assert.equal(parsed.roles[0].evidenceOriginal,parsed.evidence[0]);assert.equal(parsed.roles[0].statementOriginal.buffer,original.buffer);assert.equal(parsed.roles[0].statementOriginal.toString(),'original');assert.equal(parsed.quantities.representationBytes,1n);
 assert.deepEqual(encodeCandidateProfileSections(parsed.sections),original);
});
test('truncation extra bytes ordering and bad statement relation refuse',()=>{
 const original=fixture();const cases=[original.subarray(0,original.length-1),Buffer.concat([original,Buffer.from([0])])];
 const ordering=Buffer.from(original);ordering.writeUInt32LE(2,32);cases.push(ordering);
 const relation=Buffer.from(original),parsed=decodeCandidateProfile(relation);parsed.roles[0].spans.evidenceIndex.bytes.writeBigUInt64LE(1n);cases.push(relation);
 for(const bytes of cases)assert.throws(()=>decodeCandidateProfile(bytes));assert.equal(decodeCandidateProfile(original).roles.length,1);
});
test('paired representations retain identical complete original bytes without final hash inputs',()=>{
 const original=fixture(),output=emitCandidateProfileSources(original,{goIntBits:64,cSizeTBits:64},nativeFixture());
 const c=output.cSource.match(/sl_candidate_profile_bytes\[[0-9]+\]=\{([^}]+)\}/),go=output.goSource.match(/nativeCandidateProfileBytes=\[[0-9]+\]byte\{([^}]+)\}/);
 assert.ok(c&&go);const bytes=text=>Buffer.from(text.split(',').map(value=>Number.parseInt(value,16)));assert.deepEqual(bytes(c[1]),original);assert.deepEqual(bytes(go[1]),original);assert.equal(output.original.original,original);
 assert.throws(()=>emitCandidateProfileSources(original,{goIntBits:64,cSizeTBits:64,finalNativeHash:'untrusted'}));assert.throws(()=>emitCandidateProfileSources(original,{goIntBits:16,cSizeTBits:64}));
});
test('complete native geometry is closed, retained and exactly joined',()=>{
 const original=fixture(),native=nativeFixture(),abi={goIntBits:64,cSizeTBits:64};
 const output=emitCandidateProfileSources(original,abi,native);assert.equal(output.nativeGeometryOriginal,native);assert.equal(output.nativeGeometry.values.requests,2n);assert.match(output.cSource,/sl_candidate_construct_native_arena/);assert.match(output.cSource,/\.custody_attempts=\{1UL,2UL,3UL,4UL,5UL\}/);
 assert.match(output.cSource,/sl_linux_public_input_source_excludes_output/);assert.match(output.cSource,/sl_linux_capture_entry_public_inputs\(a->public_inputs,input_source,argc,argv,environment\)/);assert.match(output.cSource,/input_returned/);
 assert.match(output.cSource,/sl_linux_attach_containing_charge\(&sl_candidate_preparation_original.charge,charge\)/);assert.match(output.cSource,/sl_linux_charge_attachment_matches_original\(&sl_candidate_preparation_original.charge,charge\)/);assert.match(output.cSource,/sl_linux_containing_charge_excludes_output\(charge,a->base,a->layout.bytes\)/);assert.match(output.cSource,/sl_candidate_dispatch_prepared_outer/);assert.match(output.cSource,/sl_linux_enter_outer_continuation/);assert.match(output.cSource,/child.published,memory_order_acquire/);assert.match(output.cSource,/sl_linux_supervise_outer_parent/);assert.match(output.cSource,/sequence->actual_capacity-sequence->next<4\*a->outer_supervisor->capacity/);
 const cases=[];const missing=nativeFixture();delete missing.services;cases.push(missing);cases.push({...nativeFixture(),unknown:'1'},{...nativeFixture(),services:'01'},{...nativeFixture(),services:'0'},{...nativeFixture(),services:'18446744073709551616'},{...nativeFixture(),raw_frames:['1','2']},{...nativeFixture(),requests:'1'},{...nativeFixture(),security_label_bytes:'2'},{...nativeFixture(),terminal_calls:'2'},{...nativeFixture(),body_bytes:'2'});
 const sparse=nativeFixture();delete sparse.raw_frames[1];cases.push(sparse);
 for(const value of cases)assert.throws(()=>emitCandidateProfileSources(original,abi,value));assert.throws(()=>emitCandidateProfileSources(original,abi));
 const zeroRoles=nativeFixture();zeroRoles.maintenance_role_rows='0';assert.equal(emitCandidateProfileSources(original,abi,zeroRoles).nativeGeometry.values.maintenance_role_rows,0n);
});
