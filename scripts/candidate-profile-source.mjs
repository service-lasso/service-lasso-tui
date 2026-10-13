// Source-only candidate representation. UNRUN; parsing/generation is not authority.
// No final executable hashes are accepted as generator inputs.
const MAGIC = Buffer.from([0x53,0x4c,0x43,0x50,0x52,0x4f,0x46,0x31]);
const SECTION_IDS = Object.freeze([1,2,3,4,5,6,7,8,9]);
const MAX_SAFE = BigInt(Number.MAX_SAFE_INTEGER);
function extent(value, remaining, what) {
  if (value > MAX_SAFE || value > BigInt(remaining)) throw new Error(what + ' extent exceeds original backing');
  return Number(value);
}
function u64(bytes, offset) {
  if (offset < 0 || offset > bytes.length - 8) throw new Error('candidate profile truncated integer');
  return bytes.readBigUInt64LE(offset);
}
// Each span is an alias of SAME original bytes, never a replacement authority.
function records(section, fields, kind) {
  if (section.length < 8) throw new Error(kind + ' count missing');
  const count = extent(u64(section,0), section.length, kind + ' count');
  const rows = []; let at = 8;
  for (let i=0;i<count;i++) {
    if (at > section.length-fields.length*8) throw new Error(kind + ' extents truncated');
    const lengths = fields.map((name,j)=>u64(section,at+j*8)); at += fields.length*8;
    const row = { original:section, index:i, spans:Object.create(null) };
    for (let j=0;j<fields.length;j++) {
      const length=extent(lengths[j],section.length-at,kind + ' ' + fields[j]);
      row.spans[fields[j]]={offset:at,extent:length,bytes:section.subarray(at,at+length)};at+=length;
    }
    rows.push(row);
  }
  if(at!==section.length)throw new Error(kind + ' trailing bytes');
  return rows;
}
export function decodeCandidateProfile(original) {
  if (!Buffer.isBuffer(original) || original.length<32 || !original.subarray(0,8).equals(MAGIC)) throw new Error('candidate profile header invalid');
  if(original.readUInt32LE(8)!==1 || original.readUInt32LE(12)!==SECTION_IDS.length || u64(original,16)!==BigInt(original.length) || u64(original,24)!==0n)throw new Error('candidate profile closed header invalid');
  const sections=[];let at=32;
  for(const id of SECTION_IDS){
    if(at>original.length-16||original.readUInt32LE(at)!==id||original.readUInt32LE(at+4)!==0)throw new Error('candidate profile section ordering/flags invalid');
    const length=extent(u64(original,at+8),original.length-at-16,'section');at+=16;
    sections.push({id,offset:at,extent:length,bytes:original.subarray(at,at+length)});at+=length;
  }
  if(at!==original.length)throw new Error('candidate profile trailing bytes');
  if(sections[0].extent!==32||sections[1].extent!==32||sections[2].extent!==72||sections[3].extent!==24)throw new Error('candidate profile fixed field extent invalid');
  const q=sections[3].bytes;
  const quantities={securityLabelBytes:u64(q,0),terminalCalls:u64(q,8),representationBytes:u64(q,16)};
  if(Object.values(quantities).some(value=>value===0n))throw new Error('candidate profile zero reservation quantity');
  const evidence=records(sections[4].bytes,['source','authorCorrespondence','subject','scope','conditions','meaning','body'],'authorization evidence');
  // Actual evidence is retained whole; parse/index/digest never authenticates author.
  for(const row of evidence)for(const name of ['source','authorCorrespondence','subject','scope','meaning','body'])if(!row.spans[name].extent)throw new Error('candidate evidence required original empty');
  const roles=records(sections[5].bytes,['evidenceIndex','statementOffset','statementExtent','policyUser','policyRole','policyType','actorMeaning','episodeMeaning'],'maintenance role');
  for(const row of roles){
    for(const name of ['evidenceIndex','statementOffset','statementExtent','policyUser','policyRole','policyType'])if(row.spans[name].extent!==8)throw new Error('candidate role integer span invalid');
    const index=u64(row.spans.evidenceIndex.bytes,0);if(index>=BigInt(evidence.length))throw new Error('candidate role evidence index invalid');
    const body=evidence[Number(index)].spans.body;
    const offset=extent(u64(row.spans.statementOffset.bytes,0),body.extent,'statement offset');
    const length=extent(u64(row.spans.statementExtent.bytes,0),body.extent-offset,'statement');
    if(!length||!row.spans.actorMeaning.extent||!row.spans.episodeMeaning.extent)throw new Error('candidate role meaning/statement missing');
    row.evidenceOriginal=evidence[Number(index)];row.statementOriginal=body.bytes.subarray(offset,offset+length);
  }
  const entryRelations=records(sections[6].bytes,['sourceEvidence','meaning','policyInterpretation'],'entry relation');
  const protectedRoutes=records(sections[7].bytes,['sourceEvidence','meaning','policyInterpretation'],'protected route');
  const episodes=records(sections[8].bytes,['sourceEvidence','scope','conditions','preservationMeaning'],'maintenance episode');
  return {original,sections,quantities,evidence,roles,entryRelations,protectedRoutes,episodes};
}

export function encodeCandidateProfileSections(sections) {
  if(!Array.isArray(sections)||sections.length!==SECTION_IDS.length||sections.some((row,i)=>row.id!==SECTION_IDS[i]||!Buffer.isBuffer(row.bytes)))throw new Error('candidate source sections invalid');
  let total=32;for(const row of sections){if(row.bytes.length>Number.MAX_SAFE_INTEGER-total-16)throw new Error('candidate source geometry overflow');total+=16+row.bytes.length;}
  const output=Buffer.alloc(total);MAGIC.copy(output);output.writeUInt32LE(1,8);output.writeUInt32LE(SECTION_IDS.length,12);output.writeBigUInt64LE(BigInt(total),16);
  let at=32;for(const row of sections){output.writeUInt32LE(row.id,at);output.writeBigUInt64LE(BigInt(row.bytes.length),at+8);at+=16;row.bytes.copy(output,at);at+=row.bytes.length;}
  decodeCandidateProfile(output);return output;
}
// buildGeometry is source-selected target ABI metadata, not a native authority.
// Returned sources are review artifacts; this function does not write/build/run.
// Exact source ABI roster from candidate_arena_linux.h; resource configuration
// only. Caller retains the complete original record through build/review.
const NATIVE_ARRAY_FIELDS=Object.freeze({raw_frames:3,public_writes:3,forward_waits:3,forward_iterations:3,custody_attempts:5,final_io_attempts:2});
const NATIVE_SCALAR_FIELDS=Object.freeze(('scheduler_interval_ns services outer_observations deadline_roots deadline_observations deadline_polls deadline_poll_fds creator_file_bytes creator_file_reads collector_joins requests prefix_attempts terminal_reply_attempts input_frames input_steps body_bytes scan_frames scan_bytes fd_views fd_actions membership_frames membership_bytes membership_path mountinfo_bytes mountinfo_attempts mountinfo_rows mount_location_bytes selinux_rows selinux_path policy_frames policy_frame_bytes policy_body parser_bytes parser_attempts diagnostics diagnostic_arguments diagnostic_bytes diagnostic_format self_context_bytes domain_types domain_steps access_visits access_helpers access_strings reach_visits reach_helpers reach_strings reach_steps children_frames children_bytes children_views executable_path directory_path image_frames image_bytes image_writes image_readbacks credential_status_bytes credential_status_attempts credential_group_capacity pidfd_info_bytes pidfd_info_attempts public_argument_count public_argument_bytes public_environment_count public_environment_bytes security_label_bytes terminal_calls maintenance_role_rows archive_path_bytes archive_declarations').split(' '));
export function decodeNativeGeometry(original,bits,shared){
 if(!original||typeof original!=='object'||Array.isArray(original)||![32,64].includes(bits))throw new Error('native geometry original/ABI missing');
 const keys=[...Object.keys(NATIVE_ARRAY_FIELDS),...NATIVE_SCALAR_FIELDS].sort();
 if(Object.keys(original).sort().join(',')!==keys.join(','))throw new Error('native geometry closed field roster mismatch');
 const maximum=(1n<<BigInt(bits))-1n,values={};
 const value=(v,zero=false)=>{if(typeof v!=='string'||! /^(0|[1-9][0-9]*)$/.test(v))throw new Error('native geometry canonical quantity missing');const n=BigInt(v);if((!zero&&!n)||n>maximum)throw new Error('native geometry unsigned ABI extent invalid');return n;};
 for(const key of NATIVE_SCALAR_FIELDS)values[key]=value(original[key],key==='maintenance_role_rows');
 for(const [key,count]of Object.entries(NATIVE_ARRAY_FIELDS)){if(!Array.isArray(original[key])||original[key].length!==count)throw new Error('native geometry array extent mismatch');values[key]=Array.from({length:count},(_,index)=>{if(!Object.prototype.hasOwnProperty.call(original[key],index))throw new Error('native geometry missing array member');return value(original[key][index]);});}
 if(values.security_label_bytes!==shared.securityLabelBytes||values.terminal_calls!==shared.terminalCalls||values.body_bytes!==shared.representationBytes||values.requests!==shared.terminalCalls+1n)throw new Error('native geometry shared quantity mismatch');
 return {original,values,abiBits:bits};
}
export function emitCandidateProfileSources(original, buildGeometry, nativeGeometry) {
  const parsed=decodeCandidateProfile(original);
  if(!buildGeometry||Object.keys(buildGeometry).sort().join(',')!=='cSizeTBits,goIntBits'||![32,64].includes(buildGeometry.goIntBits)||![32,64].includes(buildGeometry.cSizeTBits))throw new Error('candidate target ABI geometry missing');
  const bits=Math.min(buildGeometry.goIntBits,buildGeometry.cSizeTBits),maximum=(1n<<BigInt(bits-1))-1n;
  const q=parsed.quantities,T=q.terminalCalls,R=q.representationBytes,S=q.securityLabelBytes;
  const native=decodeNativeGeometry(nativeGeometry,buildGeometry.cSizeTBits,q);
  const count=120n*T+R+16n;
  for(const value of [S,T,R,T+1n,count,8n*T,112n*T])if(value<=0n||value>maximum||value>MAX_SAFE)throw new Error('candidate generated element geometry unrepresentable');
  if(S>0xffffffffn)throw new Error('candidate security label ABI extent invalid');
  const hex=Array.from(original,byte=>'0x'+byte.toString(16).padStart(2,'0')).join(',');
  // Exact generated static objects; no installed provider/runtime/stack claim.
  const staticChargeObjects=['sl_candidate_profile_bytes','sl_candidate_profile_quantities','sl_candidate_native_quantities',
    'sl_candidate_native_arena_original','sl_candidate_layout_original','sl_candidate_profile_original',
    'sl_candidate_preparation_original',...Array.from({length:5},(_,i)=>'sl_candidate_rows_'+i),
    'sl_candidate_static_charge_basis','sl_candidate_static_charge_roster','sl_candidate_static_object_sources'];
  const staticChargeCount=staticChargeObjects.length;
  const cSource=[
    '/* Generated source representation ONLY; not loaded/authenticated authority. */',
    '#ifndef SERVICE_LASSO_CANDIDATE_PROFILE_GENERATED_H',
    '#define SERVICE_LASSO_CANDIDATE_PROFILE_GENERATED_H',
    '#include <stdint.h>',
    '#include <limits.h>',
    '#if !defined(__linux__) || !defined(__x86_64__) || defined(__ILP32__)',
    '#error selected candidate native geometry requires Linux native x86_64 ABI',
    '#endif',
    '_Static_assert(sizeof(unsigned long)*CHAR_BIT=='+buildGeometry.cSizeTBits+',"native quantity unsigned-long ABI correspondence");',

    '#include "candidate_profile.h"',
    '#include "candidate_arena_linux.h"',
    '#include "containing_charge_linux.h"',
    '#include "public_main_linux.h"',
    '#include "outer_birth_linux.h"',
    '#include "outer_supervisor_linux.h"',
    '#include <errno.h>',
    '#include <string.h>',
    '#include "shared_range_linux.h"',
    '_Static_assert(sizeof(size_t)*CHAR_BIT=='+buildGeometry.cSizeTBits+',"native quantity size_t ABI correspondence");',
    'static const unsigned char sl_candidate_profile_bytes['+original.length+']={'+hex+'};',
    'typedef struct {uint64_t security_label_bytes,terminal_calls,representation_bytes,total_channel_rows;} sl_candidate_generated_quantities;',
    'static const sl_candidate_generated_quantities sl_candidate_profile_quantities={UINT64_C('+S+'),UINT64_C('+T+'),UINT64_C('+R+'),UINT64_C('+(T+1n)+')};',
    ...[parsed.evidence,parsed.roles,parsed.entryRelations,parsed.protectedRoutes,parsed.episodes].map((rows,index)=>'static sl_candidate_record sl_candidate_rows_'+index+'['+Math.max(1,rows.length)+'];'),
    'static const sl_linux_candidate_quantities sl_candidate_native_quantities={',
    ...NATIVE_SCALAR_FIELDS.map(key=>' .'+key+'='+native.values[key]+'UL,'),
    ...Object.keys(NATIVE_ARRAY_FIELDS).map(key=>' .'+key+'={'+native.values[key].map(n=>n+'UL').join(',')+'},'),
    '};',
    'static sl_linux_candidate_arena sl_candidate_native_arena_original;',
    'static sl_linux_candidate_layout_plan sl_candidate_layout_original;',
    'static sl_candidate_profile sl_candidate_profile_original;',
    'static int sl_candidate_charge_has_static_source(const sl_linux_containing_source *source);',
    'static sl_candidate_profile *sl_candidate_construct_profile(void){',
    ' sl_candidate_profile *p=&sl_candidate_profile_original;if(p->attempted)return p;',
    ' p->source=sl_candidate_profile_bytes;p->source_extent=sizeof(sl_candidate_profile_bytes);',
    ...[parsed.evidence,parsed.roles,parsed.entryRelations,parsed.protectedRoutes,parsed.episodes].map((rows,index)=>' p->rows['+index+']=sl_candidate_rows_'+index+';p->capacities['+index+']='+rows.length+';'),
    ' (void)sl_candidate_decode_profile(p);return p;',
    '}',
    'static sl_linux_candidate_layout_plan *sl_candidate_prepare_native_plan(void){',
    ' sl_linux_candidate_layout_plan *p=&sl_candidate_layout_original;if(p->attempted)return p;',
    ' sl_candidate_profile *candidate=sl_candidate_construct_profile();',
    ' if(!candidate->returned||candidate->result)return 0;',
    ' p->quantities_original=&sl_candidate_native_quantities;p->candidate_original=candidate;',
    ' (void)sl_linux_plan_candidate_arena(p);return p;',
    ' /* No allocation. The initial source owner constructs the full containing',
    '  * charge over this actual plan before calling the allocation seam. */',
    '}',
    'static sl_linux_candidate_arena *sl_candidate_construct_native_arena(const sl_linux_containing_charge *charge){',
    ' sl_linux_candidate_arena *a=&sl_candidate_native_arena_original;if(a->attempted)return a;',
    ' sl_linux_candidate_layout_plan *p=&sl_candidate_layout_original;',
    ' if(!charge||!sl_linux_containing_charge_excludes_output(charge,a,sizeof(*a))||',
    '    !sl_candidate_charge_has_static_source(charge->actual_source)||charge->actual_plan!=p||',
    '    !p->returned||p->result||p->actual_quantities!=&sl_candidate_native_quantities||',
    '    p->actual_candidate!=&sl_candidate_profile_original)return 0;',
    ' /* Necessary original correspondence only. The actual initial entry must',
    '  * first establish admitted source/provider/prior domain and complete',
    '  * actor/lifetime roster; this parameter is not authentication authority. */',
    ' a->candidate_original=p->actual_candidate;',
    ' a->quantities=sl_candidate_native_quantities;a->plan_original=p;',
    ' (void)sl_linux_construct_shared_candidate_arena(a);return a;',
    '}',
    '/* Prebirth factory originals remain held outside a mapping that may fail.',
    ' * After successful preparation these fields are readonly in both branches;',
    ' * all later public-owner outputs reside in the actual shared arena. */',
    'static struct {sl_linux_candidate_arena *arena_original;sl_linux_public_main_owner *owner_original;sl_linux_charge_attachment charge;',
    ' long input_result,attachment_result,birth_preparation_result;unsigned attempted,arena_returned,input_returned,attachment_returned,birth_preparation_returned,returned;} sl_candidate_preparation_original;',
    'static const sl_linux_charge_item sl_candidate_static_charge_roster['+staticChargeCount+'];',
    'typedef struct {const void *object_original;unsigned long object_extent;} sl_candidate_static_object_source;',
    'static const sl_candidate_static_object_source sl_candidate_static_object_sources['+staticChargeCount+'];',
    'static const unsigned long sl_candidate_static_charge_basis['+staticChargeCount+']={',
    ...staticChargeObjects.map(name=>' sizeof('+name+'),'),
    '};',
    'static const sl_candidate_static_object_source sl_candidate_static_object_sources['+staticChargeCount+']={',
    ...staticChargeObjects.map(name=>' {&'+name+',sizeof('+name+')},'),
    '};',
    'static const sl_linux_charge_item sl_candidate_static_charge_roster['+staticChargeCount+']={',
    ...staticChargeObjects.map((name,i)=>' {&sl_candidate_static_object_sources['+i+'],sizeof(sl_candidate_static_object_source),&sl_candidate_static_charge_basis['+i+'],sizeof(unsigned long),sizeof('+name+'),SL_CHARGE_STATIC_ROOTS},'),
    '};',
    '/* Exact static source portion, including its own ledger/basis backing.',
    ' * Not the full actor roster or domain credit. Initial caller must retain',
    ' * all installed/provider/stack/runtime/kernel/IO/overlap source costs too.',
    ' * Readonly descriptors locate mutable objects without claiming their',
    ' * future output bytes are immutable actor-proof backing. Raw sizeof',
    ' * extents do not establish actual section/page/provider rounding. */',
    'static int sl_candidate_charge_has_static_source(const sl_linux_containing_source *source){',
    ' if(!source||!source->items_original||!source->count||source->count>ULONG_MAX/sizeof(sl_linux_charge_item))return 0;',
    ' for(unsigned long i=0;i<sizeof(sl_candidate_static_charge_roster)/sizeof(sl_candidate_static_charge_roster[0]);i++){',
    '  const sl_linux_charge_item *expected=&sl_candidate_static_charge_roster[i];unsigned long matches=0;',
    '  for(unsigned long j=0;j<source->count;j++){const sl_linux_charge_item *item=&source->items_original[j];',
    '   if(item->actor_original==expected->actor_original&&item->actor_extent==expected->actor_extent&&',
    '      item->basis_original==expected->basis_original&&item->basis_extent==expected->basis_extent&&',
    '      item->bytes==expected->bytes&&item->category==expected->category)matches++;',
    '  }',
    '  if(matches!=1)return 0;',
    ' }',
    ' return 1;',
    '}',
    'static long sl_candidate_construct_native_charge(sl_linux_containing_charge *charge){',
    ' if(!charge||charge->attempted||charge->plan_original!=&sl_candidate_layout_original||',
    '    !sl_candidate_charge_has_static_source(charge->source_original)||',
    '    charge->source_original->count!=charge->capacity)return -1;',
    ' return sl_linux_construct_containing_charge(charge);',
    ' /* Exact compiled static portion is mandatory once in the source roster.',
    '  * Other actors, rounding, full source/domain authenticity and lifetime',
    '  * completeness still belong to the actual initial owning caller. */',
    '}',
    'static sl_linux_public_main_owner *sl_candidate_prepare_public_main(const sl_linux_containing_charge *charge,int argc,char **argv,char **environment){',
    ' if(sl_candidate_preparation_original.attempted)return 0;',
    ' sl_linux_public_input_source original_inputs={argc,argv,environment,sl_candidate_native_quantities.public_argument_count,sl_candidate_native_quantities.public_environment_count,sl_candidate_native_quantities.public_argument_bytes,sl_candidate_native_quantities.public_environment_bytes};',
    ' for(unsigned long i=0;i<sizeof(sl_candidate_static_object_sources)/sizeof(sl_candidate_static_object_sources[0]);i++)',
    '  if(!sl_linux_public_input_source_excludes_output(&original_inputs,sl_candidate_static_object_sources[i].object_original,sl_candidate_static_object_sources[i].object_extent))return 0;',
    ' if(!sl_linux_containing_charge_excludes_output(charge,&sl_candidate_preparation_original,sizeof(sl_candidate_preparation_original)))return 0;',
    ' if(!sl_linux_attach_containing_charge(&sl_candidate_preparation_original.charge,charge))return 0;',
    ' sl_candidate_preparation_original.attempted=1;',
    ' sl_linux_candidate_arena *a=sl_candidate_construct_native_arena(charge);',
    ' sl_candidate_preparation_original.arena_original=a;sl_candidate_preparation_original.arena_returned=1;',
    ' if(!a||!a->returned||a->result||!a->bound||!a->public_main_owner)goto done;',
    ' sl_candidate_preparation_original.owner_original=a->public_main_owner;',
    ' sl_linux_shared_range input_range={&a->mapping,a->layout.public_input_source.offset,a->layout.public_input_source.bytes};',
    ' sl_linux_public_input_source *input_source=sl_linux_shared_range_address(&input_range,sizeof(*input_source),_Alignof(sl_linux_public_input_source));',
    ' if(!input_source||!a->public_inputs||a->public_inputs->source_original!=input_source)goto done;',
    ' sl_candidate_preparation_original.input_result=sl_linux_capture_entry_public_inputs(a->public_inputs,input_source,argc,argv,environment);',
    ' sl_candidate_preparation_original.input_returned=1;if(sl_candidate_preparation_original.input_result)goto done;',
    ' if(!sl_linux_public_inputs_exclude_output(a->public_inputs,a->public_main_owner,sizeof(*a->public_main_owner))||',
    '    !sl_linux_public_inputs_exclude_output(a->public_inputs,a->outer_birth,sizeof(*a->outer_birth))||',
    '    !sl_linux_public_inputs_exclude_output(a->public_inputs,&sl_candidate_preparation_original,sizeof(sl_candidate_preparation_original)))goto done;',
    ' sl_candidate_preparation_original.attachment_result=sl_linux_attach_public_main_owner(a->public_main_owner,a,argc,argv,environment);',
    ' sl_candidate_preparation_original.attachment_returned=1;',
    ' if(sl_candidate_preparation_original.attachment_result)goto done;',
    ' sl_candidate_preparation_original.birth_preparation_result=sl_linux_prepare_outer_birth(a->outer_birth);',
    ' sl_candidate_preparation_original.birth_preparation_returned=1;',
    'done:sl_candidate_preparation_original.returned=1;',
    ' if(!sl_candidate_preparation_original.input_returned||sl_candidate_preparation_original.input_result||',
    '    !sl_candidate_preparation_original.attachment_returned||sl_candidate_preparation_original.attachment_result||',
    '    !sl_candidate_preparation_original.birth_preparation_returned||sl_candidate_preparation_original.birth_preparation_result)return 0;',
    ' return sl_candidate_preparation_original.owner_original;',
    ' /* No startup/admission/capture/birth here. The actual owning entry must',
    '  * join its authentic containing reservation, source/provider/D and outer',
    '  * terminal custody before any dependent native effect is activated. */',
    '}',
    'static long sl_candidate_dispatch_prepared_outer(sl_linux_public_main_owner *owner,const sl_linux_containing_charge *charge,sl_linux_deadline_sequence *sequence){',
    ' sl_linux_candidate_arena *a=sl_candidate_preparation_original.arena_original;',
    ' if(!owner||!a||owner!=sl_candidate_preparation_original.owner_original||owner!=a->public_main_owner||',
    '    !sl_candidate_preparation_original.returned||!sl_candidate_preparation_original.input_returned||sl_candidate_preparation_original.input_result||',
    '    !sl_candidate_preparation_original.attachment_returned||sl_candidate_preparation_original.attachment_result||',
    '    !sl_candidate_preparation_original.birth_preparation_returned||sl_candidate_preparation_original.birth_preparation_result||',
    '    owner->prepared_arena_original!=a||owner->attempted||!a->bound||a->result||!a->returned||',
    '    !sl_linux_charge_attachment_matches_original(&sl_candidate_preparation_original.charge,charge)||charge->actual_plan!=&sl_candidate_layout_original||',
    '    !sl_linux_public_inputs_match_original(a->public_inputs)||!sequence||!a->deadline_sequences||!a->deadline_capacity||',
    '    a->deadline_capacity>SIZE_MAX/sizeof(*sequence))return -EINVAL;',
    ' uintptr_t start=(uintptr_t)a->deadline_sequences,selected=(uintptr_t)sequence;',
    ' size_t extent=a->deadline_capacity*sizeof(*sequence);',
    ' if(extent>UINTPTR_MAX-start||selected<start||selected-start>=extent||(selected-start)%sizeof(*sequence))return -EINVAL;',
    ' if(owner->argc!=a->public_inputs->source_snapshot.argc||owner->argv!=a->public_inputs->source_snapshot.argv||',
    '    owner->environment!=a->public_inputs->source_snapshot.environment||!a->outer_continuation||!a->outer_supervisor)return -EINVAL;',
    ' if(a->plan_original!=charge->actual_plan||memcmp(&a->layout,&charge->actual_plan->layout,sizeof(a->layout)))return -EINVAL;',
    ' sl_linux_shared_range writer_range={&a->mapping,0,a->layout.bytes};',
    ' if(sl_linux_shared_range_address(&writer_range,a->layout.bytes,1)!=a->base||',
    '    !sl_linux_containing_charge_excludes_output(charge,a->base,a->layout.bytes))return -EINVAL;',
    ' sl_linux_public_input_source originals=a->public_inputs->source_snapshot;',
    ' if(!sl_linux_public_input_source_excludes_output(&originals,a->base,a->layout.bytes)||',
    '    !sl_linux_deadline_sequence_excludes_output(sequence,a->outer_continuation,sizeof(*a->outer_continuation))||',
    '    !sl_linux_deadline_sequence_excludes_output(sequence,a->outer_birth,sizeof(*a->outer_birth))||',
    '    !sl_linux_deadline_sequence_excludes_output(sequence,a->outer_supervisor,sizeof(*a->outer_supervisor))||',
    '    a->outer_supervisor->sequence_original)return -EINVAL;',
    ' if(!a->outer_supervisor->capacity||a->outer_supervisor->capacity!=a->outer_observation_capacity||',
    '    a->outer_supervisor->capacity>ULONG_MAX/4||sequence->failed||sequence->expired||sequence->exhausted||',
    '    sequence->next>sequence->actual_capacity||sequence->actual_capacity-sequence->next<4*a->outer_supervisor->capacity)return -EINVAL;',
    ' a->outer_supervisor->sequence_original=sequence;',
    ' long raw=sl_linux_enter_outer_continuation(a->outer_continuation);',
    ' if(raw==0){',
    '  if(!atomic_load_explicit(&a->outer_continuation->child.published,memory_order_acquire)||',
    '     !a->outer_continuation->child.matched||a->outer_continuation->child.result)return raw;',
    '  if(sl_linux_construct_public_main_owner(owner))return raw;',
    '  if(sl_linux_admit_public_main_owner(owner))return raw;',
    '  (void)sl_linux_capture_public_main_owner(owner);',
    ' }else if(raw>0){',
    '  (void)sl_linux_supervise_outer_parent(a->outer_supervisor);',
    ' }',
    ' return raw;',
    ' /* Raw selects custody branch only. Existing distinct owners retain all',
    '  * outcomes; this return grants no public exit, FINAL, reap or cleanup.',
    '  * Complete admitted initial envelope/rawfork/provider/ownwriter source',
    '  * and failed-terminal archive/late-ACK still dominate invocation. */',
    '}','#endif',''
  ].join('\n');
  const go=[
    '//go:build linux','',
    '// Generated source representation and fixed backing; no authority grant.',
    'package lifetime','import "sync/atomic"',
    'var nativeCandidateProfileBytes=['+original.length+']byte{'+hex+'}',
    'var nativeCandidateQuantities=NativeStartupQuantities{SecurityLabelBytes:'+S+',TerminalCalls:'+T+',RepresentationBytes:'+R+'}',
    'var nativeCandidateEndpointCalls ['+count+']NativeEndpointIO',
    'var nativeCandidateSecurity ['+S+']byte',
    'var nativeCandidateBody ['+R+']byte',
    'var nativeCandidateTerminalCalls ['+T+']TerminalCall',
    'var nativeCandidateHeaderAttempts [8]TransferAttempt',
    'var nativeCandidateBodyAttempts ['+R+']TransferAttempt',
    'var nativeCandidateReplyAttempts [8]TransferAttempt',
    'var nativeCandidateReadback ['+R+']byte',
    'var nativeCandidateTerminalWriteAttempts ['+(8n*T)+']TransferAttempt',
    'var nativeCandidateTerminalReadAttempts ['+(112n*T)+']TransferAttempt',
    'type nativeCandidateConstruction struct{claimed atomic.Bool;profileOriginal []byte;reservation nativeStartupReservation;receiver Receiver;startup NativePayloadStartup;returned bool}',
    'var nativeCandidateOriginal nativeCandidateConstruction',
    '// Source ordinary startup calls this once; duplicate cannot reset originals.',
    'func newNativeCandidateStartupRoot()(*NativePayloadStartup,error){',
    ' o:=&nativeCandidateOriginal;if !o.claimed.CompareAndSwap(false,true){return nil,errOriginalEnrollment}',
    ' o.profileOriginal=nativeCandidateProfileBytes[:len(nativeCandidateProfileBytes):len(nativeCandidateProfileBytes)]',
    ' r:=&o.reservation;r.candidateQuantities=&nativeCandidateQuantities',
    ' r.sourceEndpointCalls=nativeCandidateEndpointCalls[:];r.sourceSecurity=nativeCandidateSecurity[:];r.sourceBody=nativeCandidateBody[:]',
    ' r.sourceTerminalCalls=nativeCandidateTerminalCalls[:];r.sourceHeaderAttempts=nativeCandidateHeaderAttempts[:];r.sourceBodyAttempts=nativeCandidateBodyAttempts[:];r.sourceReplyAttempts=nativeCandidateReplyAttempts[:]',
    ' r.sourceReadback=nativeCandidateReadback[:];r.sourceTerminalWriteAttempts=nativeCandidateTerminalWriteAttempts[:];r.sourceTerminalReadAttempts=nativeCandidateTerminalReadAttempts[:]',
    ' o.startup.candidateOriginal=o;o.startup.candidateProfile=o.profileOriginal;o.startup.reservation=r;o.startup.receiverOriginal=&o.receiver;o.returned=true;return &o.startup,nil',
    '}',''
  ];
  return {original:parsed,cSource,goSource:go.join('\n'),geometryOriginal:buildGeometry,nativeGeometryOriginal:native.original,nativeGeometry:native};
}
