/* UNRUN controlled mapping model. Example quantities are fixture inputs only,
 * never product defaults/fit/native mmap evidence. */
#if defined(__linux__) && defined(__x86_64__)
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>
#include "../candidate_arena_linux.h"
static _Alignas(max_align_t) unsigned char backing[1024*1024];
static unsigned calls;static int map_failure;
static const unsigned char candidate_bytes[3]={'s','r','c'};
static sl_candidate_profile candidate;
static sl_candidate_record candidate_rows[2];
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 calls++;call->source=source;call->number=number;call->arguments[0]=a0;call->arguments[1]=a1;
 call->arguments[2]=a2;call->arguments[3]=a3;call->arguments[4]=a4;call->arguments[5]=a5;
 assert((unsigned long)a1<=sizeof(backing));
 call->result=map_failure?-12:(long)backing;return call->result;
}
#include "../candidate_arena_linux.c"
#include "../containing_charge_linux.c"
static void quantities(sl_linux_candidate_arena *a){
 unsigned i;memset(a,0,sizeof(*a));memset(backing,0,sizeof(backing));calls=0;map_failure=0;
 a->quantities.outer_observations=1; /* synthetic source quantity, never a product default */
 a->quantities.deadline_roots=2;a->quantities.deadline_observations=3; /* independent synthetic roster geometry */
 a->quantities.deadline_polls=2;a->quantities.deadline_poll_fds=1;
 a->quantities.creator_file_bytes=64;a->quantities.creator_file_reads=2; /* synthetic geometry only */
 /* Resource-constructor fixture, not a real decoded/authorized candidate. */
 candidate.source=candidate_bytes;candidate.source_extent=3;candidate.original=(sl_candidate_span){candidate_bytes,3};candidate.returned=1;candidate.actual_rows[3]=candidate_rows;candidate.actual_capacities[3]=2;a->candidate_original=&candidate;
 for(i=0;i<3;i++){a->quantities.raw_frames[i]=1;a->quantities.public_writes[i]=1;a->quantities.forward_waits[i]=1;a->quantities.forward_iterations[i]=1;}
 a->quantities.archive_path_bytes=64;a->quantities.archive_declarations=2;a->quantities.public_argument_count=8;a->quantities.public_argument_bytes=256;a->quantities.public_environment_count=4;a->quantities.public_environment_bytes=128;
 a->quantities.reach_visits=2;a->quantities.reach_helpers=8;a->quantities.reach_strings=32;a->quantities.reach_steps=25;
 a->quantities.access_visits=2;a->quantities.access_helpers=8;a->quantities.access_strings=32;a->quantities.self_context_bytes=64;a->quantities.domain_types=5;a->quantities.domain_steps=8;a->quantities.selinux_rows=1;a->quantities.selinux_path=64;a->quantities.policy_frames=2;a->quantities.policy_frame_bytes=128;a->quantities.policy_body=256;
 a->quantities.parser_bytes=4096;a->quantities.parser_attempts=8;a->quantities.diagnostics=2;a->quantities.diagnostic_arguments=4;a->quantities.diagnostic_bytes=128;a->quantities.diagnostic_format=128;
 a->quantities.children_frames=2;a->quantities.children_bytes=128;a->quantities.children_views=8;
 a->quantities.mount_location_bytes=64;a->quantities.mountinfo_bytes=128;a->quantities.mountinfo_attempts=2;a->quantities.mountinfo_rows=1;
 a->quantities.pidfd_info_bytes=64;a->quantities.pidfd_info_attempts=2;
 a->quantities.credential_status_bytes=64;a->quantities.credential_status_attempts=2;a->quantities.credential_group_capacity=1;
 a->quantities.services=a->quantities.collector_joins=1;a->quantities.requests=2;a->quantities.terminal_calls=1;a->quantities.security_label_bytes=256;
 a->quantities.scheduler_interval_ns=1;a->quantities.final_io_attempts[0]=a->quantities.final_io_attempts[1]=1;
 a->quantities.prefix_attempts=a->quantities.terminal_reply_attempts=a->quantities.input_frames=a->quantities.input_steps=1;
 a->quantities.body_bytes=8;for(i=0;i<5;i++)a->quantities.custody_attempts[i]=1;
 a->quantities.scan_frames=a->quantities.fd_views=a->quantities.fd_actions=1;a->quantities.scan_bytes=32;
 a->quantities.membership_frames=1;a->quantities.membership_bytes=16;a->quantities.membership_path=32;
 a->quantities.executable_path=a->quantities.directory_path=64;
 a->quantities.image_frames=1;a->quantities.image_bytes=16;a->quantities.image_writes=a->quantities.image_readbacks=1;
}
int main(void){
 sl_linux_candidate_arena a;sl_linux_public_controller *c;sl_linux_public_startup *s;
 /* Actual charge body and actual pure geometry, with synthetic domain/basis.
  * These bytes supply no production authority or native reservation proof. */
 quantities(&a);
 sl_linux_candidate_layout_plan charge_plan={.quantities_original=&a.quantities,.candidate_original=&candidate};
 assert(!sl_linux_plan_candidate_arena(&charge_plan)&&!calls);
 unsigned char domain=1,graph=2,actor=3,basis=4;
 sl_linux_charge_item items[1]={{&actor,1,&basis,1,17,SL_CHARGE_STACKS_TLS}},copies[1]={0};
 sl_linux_containing_source source={&graph,1,&domain,1,ULONG_MAX,4096,items,1};
 sl_linux_containing_charge charge={.source_original=&source,.plan_original=&charge_plan,.copies_original=copies,.capacity=1};
 assert(!sl_linux_construct_containing_charge(&charge)&&!calls);
 assert(sl_linux_containing_charge_matches_original(&charge)&&charge.total==charge.arena_bytes+17);
 sl_linux_charge_item replacement_copies[1]={0};
 sl_linux_containing_charge replacement={.source_original=&source,.plan_original=&charge_plan,.copies_original=replacement_copies,.capacity=1};
 assert(!sl_linux_construct_containing_charge(&replacement)&&sl_linux_containing_charge_matches_original(&replacement));
 sl_linux_charge_attachment attachment={0};assert(sl_linux_attach_containing_charge(&attachment,&charge));
 assert(sl_linux_charge_attachment_matches_original(&attachment,&charge)&&!sl_linux_charge_attachment_matches_original(&attachment,&replacement));
 sl_linux_charge_attachment held_attachment=attachment;
 assert(!sl_linux_attach_containing_charge(&attachment,&replacement)&&!memcmp(&attachment,&held_attachment,sizeof(attachment)));
 sl_linux_containing_charge retained=charge;
 assert(sl_linux_construct_containing_charge(&charge)==-EINVAL&&!memcmp(&retained,&charge,sizeof(charge)));
 charge.remaining--;assert(!sl_linux_containing_charge_matches_original(&charge));charge.remaining++;
 items[0].bytes++;assert(!sl_linux_containing_charge_matches_original(&charge));items[0].bytes--;
 source.available_bytes--;assert(!sl_linux_containing_charge_matches_original(&charge));source.available_bytes++;
 assert(sl_linux_containing_charge_matches_original(&charge));
 unsigned char unrelated=0;
 assert(sl_linux_containing_charge_excludes_output(&charge,&unrelated,1));
 assert(!sl_linux_containing_charge_excludes_output(&charge,&charge,sizeof(charge)));
 assert(!sl_linux_containing_charge_excludes_output(&charge,copies,sizeof(copies)));
 assert(!sl_linux_containing_charge_excludes_output(&charge,&actor,1));
 assert(!sl_linux_containing_charge_excludes_output(&charge,&basis,1));
 assert(!sl_linux_containing_charge_excludes_output(&charge,&domain,1));
 assert(!sl_linux_containing_charge_excludes_output(&charge,&graph,1));
 assert(!sl_linux_containing_charge_excludes_output(&charge,&charge_plan,sizeof(charge_plan)));
 sl_linux_charge_item denied_copy[1]={0};
 sl_linux_containing_source denied_source=source;denied_source.available_bytes=1;
 sl_linux_containing_charge denied={.source_original=&denied_source,.plan_original=&charge_plan,.copies_original=denied_copy,.capacity=1};
 assert(sl_linux_construct_containing_charge(&denied)==-ENOSPC&&denied.returned&&!calls);
 assert(!sl_linux_containing_charge_matches_original(&denied)&&denied.total==charge.total);
 quantities(&a);a.prepared_quantities.services=1;
 sl_linux_candidate_arena dirty=a;
 assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls&&!memcmp(&a,&dirty,sizeof(a)));
 quantities(&a);candidate.source=(void*)&a;candidate.original.bytes=candidate.source;
 sl_linux_candidate_arena alias=a;sl_candidate_profile source_saved=candidate;
 assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls&&!memcmp(&a,&alias,sizeof(a))&&!memcmp(&candidate,&source_saved,sizeof(candidate)));
 quantities(&a);candidate.actual_rows[3]=(void*)&a;
 alias=a;source_saved=candidate;
 assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls&&!memcmp(&a,&alias,sizeof(a))&&!memcmp(&candidate,&source_saved,sizeof(candidate)));
 quantities(&a);candidate.source_extent=ULONG_MAX;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls&&!a.mapping_returned);
 quantities(&a);candidate.original.bytes=0;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls&&!a.mapping_returned);
 quantities(&a);a.quantities.raw_frames[0]=ULONG_MAX;
 assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls&&!a.mapping_returned&&!a.bound);
 quantities(&a);a.quantities.deadline_roots=0;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.deadline_observations=ULONG_MAX;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.deadline_polls=ULONG_MAX;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.deadline_poll_fds=0;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.domain_steps=ULONG_MAX;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.self_context_bytes=33;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.parser_bytes=4097;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.parser_attempts=ULONG_MAX;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.requests=1;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.terminal_calls=ULONG_MAX;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 quantities(&a);a.quantities.input_steps=0;
 assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls);
 /* Independent pure-plan fixture; it issues no synthetic native map. The
  * same source quantities remain separate from the output arena occurrence. */
 quantities(&a);sl_linux_candidate_quantities plan_source=a.quantities;
 sl_linux_candidate_layout_plan plan={.quantities_original=&plan_source,.candidate_original=&candidate};
 assert(sl_linux_plan_candidate_arena(&plan)==0&&plan.attempted&&plan.returned&&!calls&&plan.layout.bytes);
 sl_linux_candidate_layout_plan plan_saved=plan;
 assert(sl_linux_plan_candidate_arena(&plan)==-1&&!memcmp(&plan,&plan_saved,sizeof(plan))&&!calls);
 a.plan_original=&plan;assert(sl_linux_construct_candidate_arena(&a)==0&&calls==1&&a.bound);
 assert(!memcmp(&a.layout,&plan.layout,sizeof(a.layout))&&!memcmp(&plan,&plan_saved,sizeof(plan)));
 quantities(&a);sl_linux_candidate_quantities changed_source=plan_source;changed_source.services++;
 sl_linux_candidate_layout_plan changed_plan=plan;
 changed_plan.quantities_original=&changed_source;changed_plan.actual_quantities=&changed_source;a.plan_original=&changed_plan;
 alias=a;assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls&&!memcmp(&alias,&a,sizeof(a)));
 quantities(&a);sl_linux_candidate_layout_plan changed_layout=plan;changed_layout.layout.deadlines.offset++;a.plan_original=&changed_layout;
 assert(sl_linux_construct_candidate_arena(&a)==-1&&!calls&&a.attempted&&a.returned&&!a.mapping_returned);
 assert(!memcmp(&plan,&plan_saved,sizeof(plan))); /* original plan stays held */
 quantities(&a);map_failure=1;
 assert(sl_linux_construct_candidate_arena(&a)==-12&&calls==1&&a.layout_complete&&a.mapping_returned&&!a.bound);
 assert(a.mapping.result==-12&&a.raw_mapping_result==-12);
 assert(sl_linux_construct_candidate_arena(&a)==-1&&calls==1&&a.mapping.result==-12);
 quantities(&a);assert(sl_linux_construct_candidate_arena(&a)==0&&calls==1&&a.bound);
 assert(a.deadline_capacity==2&&a.layout.deadline_sources.bytes==2*sizeof(sl_linux_deadline_source));
 assert(a.outer_continuation==(void*)(backing+a.layout.outer_continuation.offset)&&a.outer_continuation->birth_original==a.outer_birth);
 assert(a.outer_supervisor==(void*)(backing+a.layout.outer_supervisor.offset)&&a.outer_supervisor->continuation_original==a.outer_continuation);
 assert(a.outer_supervisor->turns_original==(void*)(backing+a.layout.outer_turns.offset)&&a.outer_supervisor->observations_original==a.outer_observations);
 assert(a.outer_supervisor->capacity==1&&!a.outer_supervisor->sequence_original&&!a.outer_supervisor->attempted);
 assert(a.failed_parent_writer==(void*)(backing+a.layout.failed_parent_writer.offset)&&a.failed_parent_reader==(void*)(backing+a.layout.failed_parent_reader.offset));
 assert(a.layout.failed_parent_writer.bytes==sizeof(sl_linux_failed_parent_file)&&a.layout.failed_parent_reader.bytes==sizeof(sl_linux_failed_parent_file));
 assert(a.layout.failed_parent_writer.offset+sizeof(sl_linux_failed_parent_file)<=a.layout.failed_parent_reader.offset);
 assert((uintptr_t)a.failed_parent_writer%_Alignof(sl_linux_failed_parent_file)==0&&(uintptr_t)a.failed_parent_reader%_Alignof(sl_linux_failed_parent_file)==0);
 assert(!a.failed_parent_writer->attempted&&!a.failed_parent_reader->attempted&&!a.failed_parent_writer->directory_original&&!a.failed_parent_reader->writer_original);
 assert(a.public_inputs==(void*)(backing+a.layout.public_inputs.offset)&&!a.public_inputs->attempted);
 assert(a.public_inputs->source_original==(void*)(backing+a.layout.public_input_source.offset));
 assert(a.public_inputs->source_original->argument_capacity==8&&a.public_inputs->source_original->environment_capacity==4);
 assert(a.public_inputs->source_original->argument_bytes==256&&a.public_inputs->source_original->environment_bytes==128);
 assert(!a.public_inputs->source_original->argc&&!a.public_inputs->source_original->argv&&!a.public_inputs->source_original->environment);
 assert(a.layout.public_argument_views.bytes==8*sizeof(sl_linux_public_string)&&a.layout.public_environment_views.bytes==4*sizeof(sl_linux_public_string));
 assert(a.creator_file==(void*)(backing+a.layout.creator_file.offset));
 assert(a.creator_file->installation_original==&a.startup->installation&&!a.creator_file->attempted);
 assert(a.creator_file->body==(void*)(backing+a.layout.creator_file_body.offset)&&a.creator_file->body_capacity==64);
 assert(a.creator_file->reads==(void*)(backing+a.layout.creator_file_reads.offset)&&a.creator_file->read_capacity==2);
 assert(a.layout.creator_file_reads.bytes==2*sizeof(sl_linux_creator_read));
 assert(a.creator_elf==(void*)(backing+a.layout.creator_elf.offset)&&a.creator_elf->file_original==a.creator_file&&!a.creator_elf->attempted);
 assert(a.creator_elf->capacity==64/56&&a.layout.creator_elf_programs.bytes==(64/56)*sizeof(sl_linux_elf_program));
 assert(a.layout.deadlines.bytes==2*sizeof(sl_linux_deadline)&&a.layout.deadline_sequences.bytes==2*sizeof(sl_linux_deadline_sequence));
 assert(a.layout.deadline_observations.bytes==6*sizeof(sl_linux_deadline_observation));
 for(unsigned i=0;i<2;i++){
  sl_linux_deadline_sequence *d=&a.deadline_sequences[i];
  assert(d->root_original==&a.deadlines[i]&&d->capacity==3);
  assert(d->observations_original==(void*)(backing+a.layout.deadline_observations.offset+i*3*sizeof(sl_linux_deadline_observation)));
  assert(!d->attempted&&!d->prepared&&!d->next&&!d->actual_root&&!a.deadlines[i].attempted&&!a.deadlines[i].clock.source);
  for(unsigned j=0;j<2;j++){
   unsigned slot=i*2+j;sl_linux_deadline_poll *p=&a.deadline_polls[slot];
   assert(p->sequence_original==d&&!p->count&&!p->attempted&&!p->poll.source);
   assert(p->descriptors_original==(void*)(backing+a.layout.deadline_poll_descriptors.offset+slot*sizeof(struct pollfd)));
   assert(p->outputs_original==(void*)(backing+a.layout.deadline_poll_outputs.offset+slot*sizeof(struct pollfd)));
  }
 }
 s=a.startup;c=s->controller_original;
 assert(s->control_roster.startup_original==s&&s->genfs_correspondence_capacity==1&&s->genfs_correspondences[0].policy_original==&s->candidate_policies[0]&&s->genfs_correspondences[0].capacity==2);
 assert(s==(void*)(backing+a.layout.startup.offset));assert(c==(void*)(backing+a.layout.controller.offset));
 assert(c->private_arguments.security_quantity_original==&a.prepared_quantities.security_label_bytes&&c->private_arguments.terminal_quantity_original==&a.prepared_quantities.terminal_calls&&c->private_arguments.representation_quantity_original==&a.prepared_quantities.body_bytes);
 assert(c->entry.birth.cgroup_fd==-1&&c->entry.birth.payload_fd==-1&&!c->reader_count);
 assert(a.services==c->initial_service&&a.service_capacity==1&&a.collector_join_capacity==1);
 assert(a.collector_joins[0].controller_original==c&&a.services[0].controller_original==c);
 assert(c->collector_join_roster==a.collector_joins&&c->collector_join_capacity==a.collector_join_capacity&&!c->collector_join_index);
 assert(a.services[0].root.birth_original==&c->entry.birth);
 assert(a.services[0].raw_joins[2].worker_original==&c->readers[2]);
 assert(a.services[0].forward_releases[2].worker_original==&c->forwarders[2]);
 assert(a.services[0].forward_joins[2].worker_original==&c->forwarders[2]);
 assert(c->readers[0].frames!=c->readers[1].frames);
 assert(c->collection.body!=c->collection.readback&&c->collection.storage_fd==-1);
 assert(c->final_capture.controller_original==c&&c->final_capture.capacity==70&&c->final_capture.bytes==8+32*70+16384*3+80*18+80+sizeof(siginfo_t)+sizeof(struct rusage)+22*81+3);
 assert(c->final_capture.records==(void*)(backing+a.layout.final_records.offset)&&c->final_capture.readback==(void*)(backing+a.layout.final_readback.offset));
 assert(c->final_capture.writes.calls!=c->final_capture.reads.calls&&c->final_capture.writes.capacity==1&&c->final_capture.reads.capacity==1);
 assert(c->final_reader_task.controller_original==c&&c->final_reader_task.view_original==&s->final_reader&&c->final_reader_task.capture_original==&c->final_capture);
 assert(c->final_reader_task.bytes==(void*)(backing+a.layout.independent_readback.offset)&&c->final_reader_task.bytes!=c->final_capture.readback&&c->final_reader_task.capacity==c->final_capture.bytes);
 assert(c->final_reader_task.reads==(void*)(backing+a.layout.independent_reads.offset)&&c->final_reader_task.reads!=c->final_capture.reads.calls);
 assert(c->final_reader_join_capacity==1&&c->final_reader_joins[0].reader_original==&c->final_reader_task&&c->final_reader_joins[0].wait_original==&c->schedule.waits[1]&&c->schedule.wait_capacity==2);
 for(unsigned i=0;i<3;i++)assert(s->archive_labels[i].startup_original==s&&s->archive_labels[i].kind==(enum sl_linux_archive_label_kind)i&&s->archive_labels[i].capacity==a.prepared_quantities.security_label_bytes&&s->archive_labels[i].bytes==(void*)(backing+a.layout.archive_labels.offset+i*a.prepared_quantities.security_label_bytes));
 assert(s->archive_context_capacity==3&&s->archive_contexts[0].archive_original==&s->archive_labels[0]&&s->archive_contexts[2].archive_original==&s->archive_labels[2]);
 assert(s->archive_contexts[0].parse_original==&s->parses[0]&&s->archive_contexts[0].memory.bytes!=s->archive_contexts[1].memory.bytes&&s->archive_contexts[0].memory.bytes!=s->control_contexts[0].memory.bytes&&s->archive_contexts[0].diagnostics.frames!=s->archive_contexts[1].diagnostics.frames);
 assert(s->archive_access_capacity==25&&s->archive_accesses[0].context_original==&s->archive_contexts[0]&&s->archive_accesses[6].context_original==&s->archive_contexts[1]&&s->archive_accesses[12].context_original==&s->archive_contexts[2]);
 assert(s->archive_fscreate_context_capacity==1&&s->archive_fscreate_contexts==s->archive_contexts+3&&s->archive_fscreate_contexts[0].parse_original==&s->parses[0]);
 assert(s->archive_fscreate_contexts[0].memory.bytes!=s->archive_contexts[2].memory.bytes&&s->archive_fscreate_contexts[0].diagnostics.frames!=s->archive_contexts[2].diagnostics.frames);
 assert(s->archive_creation_capacity==1&&s->archive_creations[0].startup_original==s&&s->archive_creations[0].directory_original==&s->archive_contexts[2]&&s->archive_creations[0].fscreate_original==&s->archive_fscreate_contexts[0]);
 assert(s->archive_creations[0].types.capacity==a.prepared_quantities.domain_types&&s->archive_accesses[19].creation_original==&s->archive_creations[0]&&s->archive_accesses[24].creation_original==&s->archive_creations[0]);
 assert(s->archive_accesses[19].inventory.sources!=s->archive_accesses[24].inventory.sources&&s->archive_accesses[19].inventory.custody.helpers!=s->archive_accesses[24].inventory.custody.helpers);
 assert(s->archive_accesses[18].purpose==18&&s->archive_accesses[18].policy_index==0&&s->archive_accesses[18].owner_original==&s->contexts[0]&&s->archive_accesses[18].roles_original==&s->candidate_roles[0]&&s->archive_accesses[18].payload_original==&s->candidate_payloads[0].reachable);
 assert(s->archive_accesses[0].sources!=s->archive_accesses[1].sources&&s->archive_accesses[0].inventory.sources!=s->archive_accesses[1].inventory.sources&&s->archive_accesses[0].inventory.custody.helpers!=s->archive_accesses[1].inventory.custody.helpers);
 assert(s->mounts.bytes==(void*)(backing+a.layout.mountinfo_bytes.offset)&&s->mounts.row_capacity==1);
 assert(s->control_capacity==16&&s->controls[0].capacity==256&&s->controls[0].bytes!=s->controls[1].bytes);
 assert(s->candidate_target.startup_original==s&&s->candidate_target.status_capacity==1&&s->candidate_target.statuses==(void*)(backing+a.layout.target_statuses.offset));assert(s->control_access_capacity==32&&s->control_access[0].inventory.source_capacity==5&&s->control_access[0].inventory.custody.capacity==2&&s->control_access[0].inventory.custody.helper_capacity==8&&s->control_access[0].inventory.sources!=s->control_access[1].inventory.sources);
 assert(s->control_access[0].kind_original==SL_CONTROL_WRITE_APPEND&&s->control_access[1].kind_original==SL_CONTROL_RELABEL_FROM&&s->candidate_route_capacity==32&&s->candidate_routes[1].roles_original==s->candidate_routes[0].roles_original);assert(s->control_context_capacity==16&&s->control_contexts[0].memory.bytes!=s->control_contexts[1].memory.bytes&&s->control_contexts[0].memory.bytes!=s->contexts[0].memory.bytes);
 assert(s->context_capacity==1&&s->self_context.capacity==64&&(uintptr_t)s->self_context.bytes%_Alignof(sl_linux_lsm_header)==0);
 assert(s->self_context.initialized_calls[0]==&s->self_context.before&&s->self_context.initialized_calls[1]==&s->self_context.query&&s->self_context.initialized_calls[2]==&s->self_context.after);
 assert(!atomic_load(&s->self_context.before.entered)&&!atomic_load(&s->self_context.query.returned)&&!atomic_load(&s->self_context.after.returned));
 assert(s->contexts[0].memory.bytes!=s->parses[0].parser.memory.bytes&&s->contexts[0].memory.attempts!=s->parses[0].parser.memory.attempts);
 assert(s->contexts[0].diagnostics.frames!=s->parses[0].parser.diagnostics.frames);
 assert(s->reachability_capacity==1&&s->reachability[0].graph.matrix.edge_capacity==25&&s->reachability[0].graph.matrix.match_capacity==10&&s->reachability[0].graph.type_capacity==5&&s->reachability[0].graph.step_capacity==25&&s->reachability[0].graph.matrix.custody.capacity==2);
 assert(s->domain_capacity==1&&s->domains[0].capacity==5&&s->domains[0].step_capacity==8&&s->domains[0].entries!=(void*)s->domains[0].steps);
 assert(s->parse_capacity==1&&s->parses[0].parser.memory.capacity==4096);
 assert((uintptr_t)s->parses[0].parser.memory.bytes%_Alignof(max_align_t)==0);
 assert(s->parses[0].parser.memory.bytes!=s->policies[0].body);
 assert(s->parses[0].parser.diagnostics.frames[0].arguments!=s->parses[0].parser.diagnostics.frames[1].arguments);
 assert(s->immutable.frames[0].source_bytes!=s->immutable.frames[0].readback_bytes);
 assert(s->immutable.frames[0].writes!=s->immutable.frames[0].readbacks);
 assert(c->channel.requests[0].input.frames[0].steps!=0);
 assert(s->candidate_ptrace_capacity==2*a.prepared_quantities.selinux_rows);
 assert(s->candidate_ptrace[0].roles_original==s->candidate_ptrace[1].roles_original);
 assert(s->candidate_ptrace[0].inventory.sources!=s->candidate_ptrace[1].inventory.sources);
 assert(s->candidate_ptrace[0].inventory.custody.visits!=s->candidate_ptrace[1].inventory.custody.visits);
 a.quantities.image_frames=100;
 assert(a.prepared_quantities.image_frames==1&&s->immutable.capacity==1);
 assert(sl_linux_construct_candidate_arena(&a)==-1&&calls==1);
 return 0;
}
#endif
