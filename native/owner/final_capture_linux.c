#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include "gate_linux.h"
#include "shared_range_linux.h"
#include <sys/syscall.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
static void final_u64(unsigned char *bytes,uint64_t value){for(unsigned i=0;i<8;i++)bytes[i]=(unsigned char)(value>>(i*8));}
static long final_prepare_source(const uintptr_t *starts,const unsigned long *sizes,const void *source,unsigned long bytes){
 if(!bytes)return 0;if(!source)return -EINVAL;uintptr_t begin=(uintptr_t)source;
 if(bytes>UINTPTR_MAX-begin)return -EINVAL;
 for(unsigned n=0;n<5;n++)if(begin<starts[n]+sizes[n]&&starts[n]<begin+bytes)return -EINVAL;
 return 0;
}
long sl_linux_prepare_final_capture(sl_linux_final_capture *r){
 if(!r||r->prepare_attempted)return -EINVAL;
 sl_linux_public_controller *c=r->controller_original;
 if(!c||r!=&c->final_capture||!c->startup_original||r->count||r->extent||r->actual_records||!r->records||!r->capacity||!r->readback||!r->bytes||r->bytes>LONG_MAX||!r->writes.calls||!r->writes.capacity||!r->reads.calls||!r->reads.capacity)return -EINVAL;
 sl_linux_archive_files *files=&c->startup_original->archive_files;
 sl_candidate_profile *candidate=c->startup_original->actual_candidate;
 if(!candidate||!candidate->returned||candidate->result||!candidate->source||!candidate->source_extent||candidate->source_extent>LONG_MAX||candidate->original.bytes!=candidate->source||candidate->original.extent!=candidate->source_extent)return -EINVAL;
 if(!files->returned||files->result||!files->final.returned||files->final.result||files->final.fd<3||!files->directory_original||files->directory_original->fd<3||files->final.directory_original!=files->directory_original)return -EINVAL;
 uintptr_t starts[5]={(uintptr_t)r->records,(uintptr_t)r->readback,(uintptr_t)r->writes.calls,(uintptr_t)r->reads.calls,(uintptr_t)c};
 unsigned long counts[5]={r->capacity,r->bytes,r->writes.capacity,r->reads.capacity,1};
 unsigned long units[5]={sizeof(*r->records),1,sizeof(sl_linux_call),sizeof(sl_linux_call),sizeof(*c)},sizes[5];
 for(unsigned i=0;i<5;i++){
  if(counts[i]>UINTPTR_MAX/units[i])return -EINVAL;sizes[i]=counts[i]*units[i];if(sizes[i]>UINTPTR_MAX-starts[i])return -EINVAL;
  for(unsigned j=0;j<i;j++)if(starts[i]<starts[j]+sizes[j]&&starts[j]<starts[i]+sizes[i])return -EINVAL;
 }
 /* Reject source/output aliasing before ANY preparation write, including the
  * attempt marker and native-cell atomic initialization. Normal construction
  * already uses fresh nonfixed anonymous arena backing and static source;
  * this check preserves originals on a malformed component binding as well. */
 sl_linux_local_installation *installation=&c->startup_original->installation;
 uintptr_t source_starts[6]={(uintptr_t)candidate,(uintptr_t)candidate->original.bytes,(uintptr_t)c->startup_original,
 (uintptr_t)installation->prepared_executable_path,(uintptr_t)installation->prepared_directory_path,(uintptr_t)installation->payload.prepared_locator};
 unsigned long source_sizes[6]={sizeof(*candidate),candidate->original.extent,sizeof(*c->startup_original),
 installation->prepared_executable_path?installation->prepared_executable_capacity:0,installation->prepared_directory_path?installation->prepared_directory_capacity:0,installation->payload.prepared_locator?installation->payload.directory_capacity:0};
 for(unsigned i=0;i<6;i++){
  if(!source_sizes[i])continue;
  if(source_sizes[i]>UINTPTR_MAX-source_starts[i])return -EINVAL;
  for(unsigned j=0;j<5;j++)if(source_starts[i]<starts[j]+sizes[j]&&starts[j]<source_starts[i]+source_sizes[i])return -EINVAL;
 }
 sl_linux_local_membership *membership=&c->startup_original->membership;
 if(membership->prepared_capacity>ULONG_MAX/sizeof(*membership->prepared_frames)||
    final_prepare_source(starts,sizes,membership->prepared_frames,membership->prepared_capacity*sizeof(*membership->prepared_frames))||
    final_prepare_source(starts,sizes,membership->prepared_path,membership->prepared_path_capacity))return -EINVAL;
 for(unsigned long n=0;n<membership->prepared_capacity;n++)if(final_prepare_source(starts,sizes,membership->prepared_frames[n].bytes,membership->prepared_frames[n].capacity))return -EINVAL;
 sl_linux_local_mounts *mounts=&c->startup_original->mounts;
 if(mounts->actual_read_capacity>ULONG_MAX/sizeof(*mounts->actual_reads)||mounts->actual_row_capacity>ULONG_MAX/sizeof(*mounts->actual_rows)||mounts->actual_selinux_capacity>ULONG_MAX/sizeof(*mounts->actual_selinux_rows)||
    final_prepare_source(starts,sizes,mounts->actual_bytes,mounts->actual_capacity)||
    final_prepare_source(starts,sizes,mounts->actual_reads,mounts->actual_read_capacity*sizeof(*mounts->actual_reads))||
    final_prepare_source(starts,sizes,mounts->actual_rows,mounts->actual_row_capacity*sizeof(*mounts->actual_rows))||
    final_prepare_source(starts,sizes,mounts->actual_selinux_rows,mounts->actual_selinux_capacity*sizeof(*mounts->actual_selinux_rows)))return -EINVAL;
 sl_linux_public_startup *startup=c->startup_original;
 if(startup->actual_policy_capacity>ULONG_MAX/sizeof(*startup->actual_policies)||final_prepare_source(starts,sizes,startup->actual_policies,startup->actual_policy_capacity*sizeof(*startup->actual_policies)))return -EINVAL;
 for(unsigned long n=0;n<startup->actual_policy_capacity;n++){
  sl_linux_selinux_snapshot *p=&startup->actual_policies[n];
  if(p->actual_frame_capacity>ULONG_MAX/sizeof(*p->actual_frames)||final_prepare_source(starts,sizes,p->actual_frames,p->actual_frame_capacity*sizeof(*p->actual_frames))||
     final_prepare_source(starts,sizes,p->actual_path,p->actual_path_capacity)||final_prepare_source(starts,sizes,p->actual_body,p->actual_body_capacity))return -EINVAL;
  for(unsigned long j=0;j<p->actual_frame_capacity;j++)if(final_prepare_source(starts,sizes,p->actual_frames[j].actual_bytes,p->actual_frames[j].actual_capacity))return -EINVAL;
 }
 if(r->writes.count||r->writes.completed||r->reads.count||r->reads.completed)return -EINVAL;
 r->prepare_attempted=1;
 r->prepared_controller=c;r->file_original=&files->final;r->directory_original=files->directory_original;r->file_fd=files->final.fd;r->directory_fd=files->directory_original->fd;
 r->actual_records=r->records;r->actual_capacity=r->capacity;r->actual_readback=r->readback;r->actual_bytes=r->bytes;
 r->candidate_original=candidate;r->candidate_body=candidate->original;
 r->actual_writes=r->writes;r->actual_reads=r->reads;
 for(unsigned i=0;i<2;i++){sl_linux_final_io *io=i?&r->actual_reads:&r->actual_writes;for(unsigned long j=0;j<io->capacity;j++){atomic_init(&io->calls[j].entered,0);atomic_init(&io->calls[j].returned,0);}}
 atomic_init(&r->flush.entered,0);atomic_init(&r->flush.returned,0);atomic_init(&r->directory_flush.entered,0);atomic_init(&r->directory_flush.returned,0);atomic_init(&r->eof.entered,0);atomic_init(&r->eof.returned,0);
 r->prepared=1;return 0;
}
static long final_source_span(sl_linux_final_capture *r,const unsigned char *body,unsigned long length){
 if(!body&&length)return -EINVAL;
 /* A later readback or call/header write must never mutate an original body.
  * These are the SAME frozen regions checked before acquisition by prepare;
  * source ownership/last-borrower immutability remains the arena's obligation. */
 if(length){
  uintptr_t start=(uintptr_t)body;
  if(length>UINTPTR_MAX-start)return -EINVAL;
  uintptr_t starts[5]={(uintptr_t)r->actual_records,(uintptr_t)r->actual_readback,(uintptr_t)r->actual_writes.calls,(uintptr_t)r->actual_reads.calls,(uintptr_t)r->prepared_controller};
  unsigned long sizes[5]={r->actual_capacity*sizeof(*r->actual_records),r->actual_bytes,r->actual_writes.capacity*sizeof(sl_linux_call),r->actual_reads.capacity*sizeof(sl_linux_call),sizeof(*r->prepared_controller)};
  for(unsigned i=0;i<5;i++)if(start<starts[i]+sizes[i]&&starts[i]<start+length)return -EINVAL;
 }
 return 0;
}
static long final_add(sl_linux_final_capture *r,void *source,const unsigned char *body,unsigned long length,uint64_t kind,uint64_t stream,uint64_t frame,long raw){
 if(r->count>=r->actual_capacity||r->extent>r->actual_bytes||32>r->actual_bytes-r->extent||length>r->actual_bytes-r->extent-32||(!body&&length))return -ENOSPC;
 sl_linux_final_record *record=&r->actual_records[r->count++];record->source_original=source;record->body=body;record->length=length;record->offset=r->extent;
 final_u64(record->header,kind);final_u64(record->header+8,stream);final_u64(record->header+16,frame);final_u64(record->header+24,(uint64_t)raw);
 r->extent+=32+length;return 0;
}
static long final_exact(sl_linux_final_capture *r,sl_linux_final_io *io,const unsigned char *bytes,unsigned long length,unsigned long offset,unsigned reading){
 unsigned long progress=0;
 while(progress<length){
  if(io->count>=io->capacity)return -ENOSPC;
  sl_linux_call *call=&io->calls[io->count++];unsigned long remaining=length-progress;
  long result=sl_linux_call6(call,reading?"archive.final.capture.readback":"archive.final.capture.persist",reading?SYS_pread64:SYS_pwrite64,r->file_fd,(long)(bytes+progress),remaining,offset+progress,0,0);
  if(result<=0||(unsigned long)result>remaining)return result<0?result:-EIO;
  progress+=(unsigned long)result;io->completed+=(unsigned long)result;
 }
 return 0;
}
static long final_native_call(sl_linux_final_capture *r,sl_linux_call *original,uint64_t stream,uint64_t frame,uint64_t slot){
 if(r->count>=r->actual_capacity)return -ENOSPC;
 unsigned char *body=r->actual_records[r->count].observation;
 final_u64(body,(uint64_t)original->number);
 for(unsigned i=0;i<6;i++)final_u64(body+8*(i+1),(uint64_t)original->arguments[i]);
 final_u64(body+56,(uint64_t)original->result);
 final_u64(body+64,atomic_load_explicit(&original->entered,memory_order_acquire));
 final_u64(body+72,atomic_load_explicit(&original->returned,memory_order_acquire));
 return final_add(r,original,body,80,3,stream,frame,(long)slot);
}
static long installation_budget(sl_linux_final_capture *r,unsigned long *count,unsigned long *extent,unsigned long bytes){
 if(*count>=r->actual_capacity||*extent>r->actual_bytes||32>r->actual_bytes-*extent||bytes>r->actual_bytes-*extent-32)return -ENOSPC;
 (*count)++;*extent+=32+bytes;return 0;
}
/* Explicit containing origin, never a synthetic nested native mapping call.
 * The same actual acquisition may be referenced by several distinct ranges;
 * every emitted copy is counted independently. */
static long final_shared_range(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,
 const sl_linux_shared_range *range,uint64_t stream,uint64_t frame){
 if(!range->mapping_original)return 0;
 void *address=sl_linux_shared_range_address(range,range->bytes,1);if(!address)return -EINVAL;
 long result=final_source_span(r,(const unsigned char *)range->mapping_original,sizeof(sl_linux_call));if(result)return result;
 result=final_source_span(r,address,range->bytes);if(result)return result;
 if(!emit){result=installation_budget(r,count,extent,80);if(result)return result;return installation_budget(r,count,extent,80);}
 result=final_native_call(r,(sl_linux_call *)range->mapping_original,stream,frame,0);if(result)return result;
 if(r->count>=r->actual_capacity)return -ENOSPC;
 unsigned char *body=r->actual_records[r->count].observation;
 uint64_t fields[10]={range->offset,range->bytes,(uint64_t)(uintptr_t)address,0,0,0,0,0,0,0};
 for(unsigned i=0;i<10;i++)final_u64(body+8*i,fields[i]);
 return final_add(r,(void *)range,body,80,36,stream,frame,1);
}
static long final_phase_body(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const void *body,unsigned long length,uint64_t kind,uint64_t frame){
 if(!emit)return installation_budget(r,count,extent,length);
 return final_add(r,owner,body,length,kind,UINT64_MAX-10,frame,0);
}
static long final_bootstrap(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_birth *birth=&r->prepared_controller->entry.birth;if(!birth->attempted)return 0;
 sl_linux_bootstrap *b=birth->bootstrap;if(!b)return -EINVAL;
 long result=final_source_span(r,(const unsigned char*)b,sizeof(*b));if(result)return result;
 if(b->fd_action_count>ULONG_MAX/sizeof(*b->fd_actions)||(!b->fd_actions&&b->fd_action_count)||b->gate_count>8)return -EINVAL;
 result=final_source_span(r,(const unsigned char*)b->fd_actions,b->fd_action_count*sizeof(*b->fd_actions));if(result)return result;
 sl_linux_preexec_authority *a=&b->authority;
 for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++){
  result=final_shared_range(r,emit,count,extent,&a->credential_ranges[i][j],UINT64_MAX-10,40+3*i+j);if(result)return result;
 }
 if(b->candidate_preexec){
  sl_linux_candidate_preexec *p=r->prepared_controller->candidate_preexec_original;
  if(!p||p->shared!=b->candidate_preexec||p->actual_bootstrap!=b)return -EINVAL;
  result=final_shared_range(r,emit,count,extent,&p->containing_range,UINT64_MAX-10,46);if(result)return result;
 }
 sl_linux_credential_snapshot *credentials[2]={&a->credentials_before,&a->credentials_after};
 for(unsigned i=0;i<2;i++){
  sl_linux_credential_snapshot *c=credentials[i];
  if(c->read_count>c->read_capacity||c->read_count>ULONG_MAX/sizeof(*c->reads)||c->length>c->status_capacity||c->group_count>c->group_capacity||c->group_count>ULONG_MAX/sizeof(*c->groups))return -EINVAL;
  result=final_source_span(r,(const unsigned char*)c->reads,c->read_count*sizeof(*c->reads));if(result)return result;
  result=final_source_span(r,c->status,c->length);if(result)return result;
  result=final_source_span(r,(const unsigned char*)c->groups,c->group_count*sizeof(*c->groups));if(result)return result;
 }
 uint64_t states[3][10]={{birth->attempted,(uint64_t)birth->cgroup_fd,(uint64_t)birth->payload_fd,(uint64_t)birth->entry_gate_fd,b->fd_action_count,b->gate_count,(uint64_t)b->controlling_terminal_fd,(uint64_t)b->pidfd,atomic_load(&b->exec_entered),atomic_load(&b->failed)},
 {a->prepared,a->attempted,a->last_cap,(uint64_t)a->result,atomic_load(&a->inspected_ready),atomic_load(&a->returned),a->status_bytes,a->status_attempts,a->group_capacity,a->credential_mapping_bytes},
 {(uint64_t)a->credential_mapping_result,atomic_load(&b->fd_actions_prepared),0,0,0,0,0,0,0,0}};
 for(unsigned i=0;i<3;i++){
  if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,states[i][j]);result=final_add(r,b,body,80,7,UINT64_MAX-10,i,0);}if(result)return result;
 }
 sl_linux_call *calls[240]={&b->clone_parent,&b->clone_child,&b->create_session,&b->acquire_terminal,&b->gate_close,&b->execute,&b->failure_pause,&a->credential_mapping,&a->unshare,&a->uid_before,&a->uid_after,&a->gid_before,&a->gid_after,&a->securebits_set,&a->securebits_get,&a->ambient_clear,&a->cap_before,&a->cap_clear,&a->cap_after,&a->nnp_set,&a->nnp_get};unsigned used=21;
 for(unsigned i=0;i<8;i++)calls[used++]=&b->gate_reads[i];
 for(unsigned i=0;i<2;i++){calls[used++]=&a->namespace_before[i];calls[used++]=&a->namespace_after[i];}
 for(unsigned i=0;i<64;i++){calls[used++]=&a->bounding_drop[i];calls[used++]=&a->bounding_read[i];calls[used++]=&a->ambient_read[i];}
 for(unsigned i=0;i<2;i++){calls[used++]=&credentials[i]->open;calls[used++]=&credentials[i]->filesystem_query;calls[used++]=&credentials[i]->groups_query;calls[used++]=&credentials[i]->close;}
 for(unsigned i=0;i<used;i++)if(calls[i]->source){if(calls[i]!=&b->execute&&!atomic_load_explicit(&calls[i]->returned,memory_order_acquire))return -EINVAL;if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[i],UINT64_MAX-10,3,i);if(result)return result;}
 for(unsigned long i=0;i<b->fd_action_count;i++){
  sl_linux_fd_action *action=&b->fd_actions[i];if(action->call.source&&!atomic_load_explicit(&action->call.returned,memory_order_acquire))return -EINVAL;
  uint64_t fields[10]={action->kind,(uint64_t)action->source_fd,(uint64_t)action->target_fd,action->outer_original!=NULL,0,0,0,0,0,0};
  if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,fields[j]);result=final_add(r,action,body,80,7,UINT64_MAX-10,4,i);}if(result)return result;
  if(action->call.source){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&action->call,UINT64_MAX-10,4,i);if(result)return result;}
 }
 const void *bodies[15]={&b->arguments,b->gate_expected,b->gate_received,a->namespaces_before,a->namespaces_after,a->uids_before,a->uids_after,a->gids_before,a->gids_after,a->caps_before,a->caps_after,a->caps_clear,&a->header_before,&a->header_clear,&a->header_after};
 unsigned long sizes[15]={sizeof(b->arguments),8,8,sizeof(a->namespaces_before),sizeof(a->namespaces_after),sizeof(a->uids_before),sizeof(a->uids_after),sizeof(a->gids_before),sizeof(a->gids_after),sizeof(a->caps_before),sizeof(a->caps_after),sizeof(a->caps_clear),sizeof(a->header_before),sizeof(a->header_clear),sizeof(a->header_after)};
 for(unsigned i=0;i<15;i++){result=final_phase_body(r,emit,count,extent,b,bodies[i],sizes[i],8,5+i);if(result)return result;}
 for(unsigned i=0;i<2;i++){
  sl_linux_credential_snapshot *c=credentials[i];
  for(unsigned long j=0;j<c->read_count;j++){sl_linux_call *call=&c->reads[j];if(!atomic_load_explicit(&call->returned,memory_order_acquire))return -EINVAL;if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,call,UINT64_MAX-10,16+i,j);if(result)return result;}
  uint64_t fields[10]={c->attempted,c->returned,c->eof,(uint64_t)c->fd,(uint64_t)c->result,c->length,c->read_count,c->group_count,0,0};
  if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,fields[j]);result=final_add(r,c,body,80,7,UINT64_MAX-10,18+i,0);}if(result)return result;
  const void *data[5]={c->status,c->groups,c->uids,c->gids,&c->filesystem};unsigned long lengths[5]={c->length,c->group_count*sizeof(*c->groups),sizeof(c->uids),sizeof(c->gids),sizeof(c->filesystem)};
  for(unsigned j=0;j<5;j++){result=final_phase_body(r,emit,count,extent,c,data[j],lengths[j],j?8:14,24+5*i+j);if(result)return result;}
 }
 return 0;
}
/* Embedded settled entry is disjoint from the actual mutable capture owner.
 * Do not serialize controller-wide memory or waive output-region checks. */
static long final_entry_span(sl_linux_final_capture *r,const void *body,unsigned long length){
 uintptr_t starts[5]={(uintptr_t)r->actual_records,(uintptr_t)r->actual_readback,(uintptr_t)r->actual_writes.calls,(uintptr_t)r->actual_reads.calls,(uintptr_t)r};
 unsigned long sizes[5]={r->actual_capacity*sizeof(*r->actual_records),r->actual_bytes,r->actual_writes.capacity*sizeof(sl_linux_call),r->actual_reads.capacity*sizeof(sl_linux_call),sizeof(*r)};
 for(unsigned i=0;i<5;i++)if(sizes[i]>UINTPTR_MAX-starts[i])return -EINVAL;
 return final_prepare_source(starts,sizes,body,length);
}
static unsigned long final_service_index(sl_linux_public_controller *c,const void *source,unsigned kind){
 if(!source)return ULONG_MAX;
 for(unsigned long i=0;i<c->service_capacity;i++){
  sl_linux_service_observation *o=&c->service_roster[i];const void *member=kind==0?(const void*)&o->root:kind==1?(const void*)&o->selinux:kind==2?(const void*)&o->family:(const void*)&o->family_join;
  if(source==member)return i;
 }
 return ULONG_MAX-1;
}
static long final_service_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,uint64_t frame,uint64_t service){
 if(!emit)return installation_budget(r,count,extent,80);unsigned char *body=r->actual_records[r->count].observation;
 for(unsigned i=0;i<10;i++)final_u64(body+8*i,fields[i]);return final_add(r,owner,body,80,7,UINT64_MAX-19,service,(long)frame);
}
static long final_service_body(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const void *body,unsigned long length,uint64_t frame,unsigned kind,uint64_t service){
 long result=final_entry_span(r,body,length);if(result)return result;if(!emit)return installation_budget(r,count,extent,length);
 return final_add(r,owner,body,length,kind,UINT64_MAX-19,service,(long)frame);
}
static long final_services(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;
 if(!c->service_capacity)return 0;if(c->service_capacity>ULONG_MAX/sizeof(*c->service_roster)||c->service_index>c->service_capacity)return -EINVAL;
 long result=final_entry_span(r,c->service_roster,c->service_capacity*sizeof(*c->service_roster));if(result)return result;
 for(unsigned long i=0;i<c->service_capacity;i++){
  sl_linux_service_observation *o=&c->service_roster[i];if(!o->attempted)continue;
  if(!o->returned||o->controller_original!=c)return -EINVAL;
  unsigned long prior=final_service_index(c,o->selinux.prior_original,1);if(prior==ULONG_MAX-1)return -EINVAL;
  if(o->root.attempted&&o->root.birth_original!=&c->entry.birth)return -EINVAL;
  sl_linux_call *calls[2]={&o->root.wait,&o->schedule_yield};
  for(unsigned j=0;j<2;j++)if(calls[j]->source&&!atomic_load_explicit(&calls[j]->returned,memory_order_acquire))return -EINVAL;
  uint64_t states[3][10]={{i,o->prepared,o->attempted,o->returned,(uint64_t)o->authority_result,(uint64_t)o->selinux_result,(uint64_t)o->root_result,(uint64_t)o->yield_result,(uint64_t)o->collector_result,(uint64_t)o->source_result},
  {o->authority.attempted,o->authority.returned,o->authority.not_ready,o->authority.bootstrap_failed,(uint64_t)o->authority.result,o->authority.checked_fd_actions,o->authority.fd_failure_original!=NULL,o->authority.authority_original!=NULL,0,0},
  {o->selinux.attempted,o->selinux.returned,o->selinux.not_ready,(uint64_t)o->selinux.context_result,(uint64_t)o->selinux.result,prior,o->selinux.shared_original!=NULL,(uint64_t)o->family_result,o->collector_exhausted,(uint64_t)o->source_cut_result}};
  void *owners[3]={o,&o->authority,&o->selinux};for(unsigned j=0;j<3;j++){result=final_service_state(r,emit,count,extent,owners[j],states[j],j,i);if(result)return result;}
  for(unsigned j=0;j<2;j++)if(calls[j]->source){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[j],UINT64_MAX-19,i,j);if(result)return result;}
  result=final_service_body(r,emit,count,extent,&o->root,&o->root.information,sizeof(o->root.information),3,8,i);if(result)return result;
  result=final_service_body(r,emit,count,extent,&o->root,&o->root.usage,sizeof(o->root.usage),4,8,i);if(result)return result;
  sl_linux_family_join *join=&o->family_join;
  if(join->attempted){unsigned long worker_index=final_service_index(c,join->worker_original,2);if(worker_index>=c->service_capacity)return -EINVAL;uint64_t fields[10]={i,worker_index,join->attempted,join->returned,(uint64_t)join->native_result,(uint64_t)join->interpretation_result,join->joined_result!=NULL,join->joined_result==&join->worker_original->observation,0,0};result=final_service_state(r,emit,count,extent,join,fields,5,i);if(result)return result;}
  sl_linux_family_worker *worker=&o->family;sl_linux_family_events *f=&worker->observation;if(!worker->attempted)continue;
  unsigned long root_index=final_service_index(c,worker->root_terminal_original,0),join_index=final_service_index(c,worker->joined_by,3);
  if(root_index==ULONG_MAX-1||join_index==ULONG_MAX-1||f->read_count>22)return -EINVAL;
  if(worker->create_returned&&!worker->create_result&&(!atomic_load_explicit(&worker->body_returned,memory_order_acquire)||join_index>=c->service_capacity||!worker->joined_by->returned||worker->joined_by->native_result||worker->joined_by->worker_original!=worker||worker->joined_by->joined_result!=f))return -EINVAL;
  sl_linux_call *native[6]={&f->open,&f->filesystem_query,&f->identity_query,&f->close,&f->lift.duplicate,&f->lift.original_close};
  for(unsigned j=0;j<6;j++)if(native[j]->source&&!atomic_load_explicit(&native[j]->returned,memory_order_acquire))return -EINVAL;
  uint64_t family[3][10]={{i,worker->attempted,worker->create_entered,worker->create_returned,(uint64_t)worker->create_result,worker->create_entered?atomic_load(&worker->body_entered):0,worker->create_entered?atomic_load(&worker->body_returned):0,root_index,join_index,0},
  {f->attempted,f->returned,f->eof,f->excess,f->framing_complete,f->populated,f->frozen,f->read_count,f->extent,(uint64_t)f->result},
  {(uint64_t)f->raw_fd,(uint64_t)f->fd,(uint64_t)f->lift.held_fd,(uint64_t)f->primary_result,(uint64_t)f->close_result,f->close_attempted,f->close_returned,f->actual_target!=NULL,0,0}};
  for(unsigned j=0;j<3;j++){result=final_service_state(r,emit,count,extent,worker,family[j],6+j,i);if(result)return result;}
  for(unsigned j=0;j<6;j++)if(native[j]->source){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,native[j],UINT64_MAX-19,i,2+j);if(result)return result;}
  const void *bodies[3]={&f->how,&f->filesystem,&f->identity};unsigned long sizes[3]={sizeof(f->how),sizeof(f->filesystem),sizeof(f->identity)};
  for(unsigned j=0;j<3;j++){result=final_service_body(r,emit,count,extent,f,bodies[j],sizes[j],9+j,8,i);if(result)return result;}
  for(unsigned long j=0;j<f->read_count;j++){
   sl_linux_family_read *read=&f->reads[j];if(!read->attempted||!read->returned||!read->call.source||!atomic_load_explicit(&read->call.returned,memory_order_acquire)||read->count!=read->call.result||read->count>1)return -EINVAL;
   if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&read->call,UINT64_MAX-19,i,8+j);if(result)return result;
   result=final_service_body(r,emit,count,extent,read,&read->byte,read->count>0?(unsigned long)read->count:0,12+j,19,i);if(result)return result;
  }
 }
 return 0;
}
static unsigned long final_context_allocation(sl_sepol_memory_owner *,const void *,unsigned long);
static long final_diagnostic_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,unsigned long parser,unsigned kind,uint64_t stream){
 if(!emit)return installation_budget(r,count,extent,80);unsigned char *body=r->actual_records[r->count].observation;
 for(unsigned n=0;n<10;n++)final_u64(body+8*n,fields[n]);return final_add(r,owner,body,80,7,stream,parser,(long)kind);
}
static long final_diagnostic_frame(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,sl_sepol_diagnostic_owner *owner,sl_sepol_memory_owner *memory,sl_sepol_diagnostic *f,unsigned long parser,unsigned long ordinal,uint64_t stream){
 if(!f->entered)return 0;if(owner->actual_memory!=memory)return -EINVAL;
 if(!f->returned||f->argument_count>f->actual_argument_capacity||f->actual_argument_capacity>ULONG_MAX/sizeof(*f->actual_arguments)||f->format_length>owner->actual_format_capacity)return -EINVAL;
 long result=final_source_span(r,(const unsigned char*)f,sizeof(*f));if(result)return result;
 result=final_source_span(r,(const unsigned char*)f->actual_arguments,f->actual_argument_capacity*sizeof(*f->actual_arguments));if(result)return result;
 result=final_source_span(r,(const unsigned char*)f->actual_output,f->actual_output_capacity);if(result)return result;
 result=final_source_span(r,(const unsigned char*)f->format,f->format_length);if(result)return result;
 if(memory->active||memory->attempt_count>memory->actual_attempt_capacity||memory->actual_attempt_capacity>ULONG_MAX/sizeof(*memory->actual_attempts))return -EINVAL;
 result=final_source_span(r,(const unsigned char*)memory->actual_attempts,memory->actual_attempt_capacity*sizeof(*memory->actual_attempts));if(result)return result;
 for(unsigned long j=0;j<f->argument_count;j++){
  sl_sepol_argument *a=&f->actual_arguments[j];if(a->kind!=SL_ARG_STRING&&a->kind!=SL_ARG_WSTRING)continue;
  if(a->referent_copy_extent>a->referent_extent)return -EINVAL;
  if(!a->referent_entered||!a->referent_returned||a->referent_owner!=memory||a->referent_original!=(a->kind==SL_ARG_STRING?(const void*)a->value.string:(const void*)a->value.wide))return -EINVAL;
  if(a->referent_copy_extent){if(final_context_allocation(memory,a->referent_copy,a->referent_copy_extent)==ULONG_MAX)return -EINVAL;result=final_source_span(r,a->referent_copy,a->referent_copy_extent);if(result)return result;}
 }
 uint64_t states[3][10]={{ordinal,f->entered,f->returned,(uint64_t)f->result,(uint64_t)f->level,(uint64_t)f->entry_errno,f->argument_count,f->actual_argument_capacity,f->arguments_complete,f->format_length},
 {ordinal,f->format_entered,f->format_returned,(uint64_t)f->format_result,(uint64_t)f->format_errno,f->actual_output_capacity,(uint64_t)(uintptr_t)f->handle_original,(uint64_t)(uintptr_t)f->channel,(uint64_t)(uintptr_t)f->function,(uint64_t)(uintptr_t)f->format},
 {ordinal,(uint64_t)(uintptr_t)f->actual_output,(uint64_t)(uintptr_t)f->actual_arguments,0,0,0,0,0,0,0}};
 for(unsigned j=0;j<3;j++){result=final_diagnostic_state(r,emit,count,extent,f,states[j],parser,1+j,stream);if(result)return result;}
 const void *bodies[3]={f->format,f->actual_arguments,f->actual_output};unsigned long sizes[3]={f->format_length,f->argument_count*sizeof(*f->actual_arguments),f->format_entered?f->actual_output_capacity:0};
 /* Output is the actual retained finite backing, including partial/unknown
  * formatter writes; capacity bytes are not claimed as produced byte count. */
 for(unsigned j=0;j<3;j++)if(sizes[j]){if(!emit)result=installation_budget(r,count,extent,sizes[j]);else result=final_add(r,f,bodies[j],sizes[j],21+j,stream,parser,(long)ordinal);if(result)return result;}
 for(unsigned long j=0;j<f->argument_count;j++){
  sl_sepol_argument *a=&f->actual_arguments[j];if(a->kind!=SL_ARG_STRING&&a->kind!=SL_ARG_WSTRING)continue;
  unsigned long allocation=a->referent_copy_extent?final_context_allocation(memory,a->referent_copy,a->referent_copy_extent):ULONG_MAX;
  uint64_t state[10]={ordinal,j,a->precision_present,a->referent_limit,a->referent_extent,a->referent_terminated,a->referent_entered,a->referent_returned,(uint64_t)a->referent_result,allocation};
  result=final_diagnostic_state(r,emit,count,extent,a,state,parser,4,stream);if(result)return result;
  uint64_t copied[10]={ordinal,j,a->referent_copy_extent,(uint64_t)(uintptr_t)a->referent_original,(uint64_t)(uintptr_t)a->referent_copy,0,0,0,0,0};result=final_diagnostic_state(r,emit,count,extent,a,copied,parser,5,stream);if(result)return result;
  if(a->referent_copy_extent){if(!emit)result=installation_budget(r,count,extent,a->referent_copy_extent);else result=final_add(r,a,a->referent_copy,a->referent_copy_extent,stream==UINT64_MAX-22?28:29,parser,ordinal,(long)j);if(result)return result;}
 }
 return 0;
}
static long final_parser_diagnostics(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_startup *startup=r->prepared_controller->startup_original;if(!startup->actual_parse_capacity)return 0;
 if(startup->actual_parse_capacity>ULONG_MAX/sizeof(*startup->actual_parses))return -EINVAL;
 long result=final_source_span(r,(const unsigned char*)startup->actual_parses,startup->actual_parse_capacity*sizeof(*startup->actual_parses));if(result)return result;
 for(unsigned long i=0;i<startup->actual_parse_capacity;i++){
  sl_linux_policy_parse *root=&startup->actual_parses[i];if(!root->attempted)continue;
  sl_sepol_diagnostic_owner *o=&root->parser.diagnostics;if(!root->returned||!root->parser.returned||o->active||o->count>o->actual_capacity||o->actual_capacity>ULONG_MAX/sizeof(*o->actual_frames))return -EINVAL;
  result=final_source_span(r,(const unsigned char*)o->actual_frames,o->actual_capacity*sizeof(*o->actual_frames));if(result)return result;
  unsigned long failure=ULONG_MAX;
  if(o->first_failure){if(o->first_failure==&o->exhaustion)failure=ULONG_MAX-1;else for(unsigned long j=0;j<o->count;j++)if(o->first_failure==&o->actual_frames[j])failure=j;if(failure==ULONG_MAX)return -EINVAL;}
  uint64_t state[10]={o->begin_entered,o->begin_returned,(uint64_t)o->begin_result,o->active,o->finished,o->exhausted,o->count,o->actual_capacity,o->actual_format_capacity,failure};
  result=final_diagnostic_state(r,emit,count,extent,o,state,i,0,UINT64_MAX-22);if(result)return result;
  for(unsigned long j=0;j<o->count;j++){result=final_diagnostic_frame(r,emit,count,extent,o,&root->parser.memory,&o->actual_frames[j],i,j,UINT64_MAX-22);if(result)return result;}
  if(o->exhaustion.entered){result=final_diagnostic_frame(r,emit,count,extent,o,&root->parser.memory,&o->exhaustion,i,ULONG_MAX,UINT64_MAX-22);if(result)return result;}
 }
 return 0;
}
static long final_parser_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,unsigned long parser,unsigned kind,uint64_t stream){
 if(!emit)return installation_budget(r,count,extent,80);unsigned char *body=r->actual_records[r->count].observation;
 for(unsigned n=0;n<10;n++)final_u64(body+8*n,fields[n]);return final_add(r,owner,body,80,7,stream,parser,(long)kind);
}
static long final_memory_attempt(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,sl_sepol_memory_owner *memory,sl_sepol_memory_attempt *a,unsigned long parser,unsigned long ordinal,uint64_t stream){
 long result=final_source_span(r,(const unsigned char*)a,sizeof(*a));if(result)return result;
 unsigned long previous=ULONG_MAX;
 if(a->previous_allocation){for(unsigned long k=0;k<memory->attempt_count;k++)if(a->previous_allocation==&memory->actual_attempts[k])previous=k;if(previous==ULONG_MAX)return -EINVAL;}
 unsigned long source_extent=0;
 if(a->source){while(source_extent<r->actual_bytes&&a->source[source_extent])source_extent++;if(source_extent==r->actual_bytes)return -ENOSPC;source_extent++;result=final_source_span(r,(const unsigned char*)a->source,source_extent);if(result)return result;}
 unsigned long copied_extent=0;
 if((a->operation==SL_SEPOL_DUPLICATE||a->operation==SL_SEPOL_BOUNDED_DUPLICATE)&&a->returned&&!a->error&&a->result){
  if(!a->extent||a->offset>memory->used||a->extent>memory->used-a->offset||memory->used>memory->actual_capacity||!memory->actual_bytes||a->offset>UINTPTR_MAX-(uintptr_t)memory->actual_bytes||(uintptr_t)a->result!=(uintptr_t)memory->actual_bytes+a->offset)return -EINVAL;
  copied_extent=a->extent;result=final_source_span(r,a->result,copied_extent);if(result)return result;
 }
 uint64_t fields[3][10]={{ordinal,a->operation,a->line,a->count,a->size,a->extent,a->offset,(uint64_t)(uintptr_t)a->input,(uint64_t)(uintptr_t)a->result,source_extent},
 {ordinal,(uint64_t)(uintptr_t)a->field_original,(uint64_t)(uintptr_t)a->old_pointer,(uint64_t)(uintptr_t)a->new_pointer,a->field_width,a->old_scalar,a->new_scalar,previous,(uint64_t)a->error,a->entered},
 {ordinal,a->returned,a->logically_released,0,0,0,0,0,0,0}};
 for(unsigned j=0;j<3;j++){result=final_parser_state(r,emit,count,extent,a,fields[j],parser,3+j,stream);if(result)return result;}
 if(source_extent){if(!emit)result=installation_budget(r,count,extent,source_extent);else result=final_add(r,a,(const unsigned char*)a->source,source_extent,20,stream,parser,(long)ordinal);if(result)return result;}
 if(copied_extent){if(!emit)return installation_budget(r,count,extent,copied_extent);return final_add(r,a,a->result,copied_extent,24,stream,parser,(long)ordinal);}
 return 0;
}
static unsigned long final_context_allocation(sl_sepol_memory_owner *m,const void *value,unsigned long bytes){
 if(!value||!m->actual_bytes||m->used>m->actual_capacity||m->used>UINTPTR_MAX-(uintptr_t)m->actual_bytes)return ULONG_MAX;
 uintptr_t base=(uintptr_t)m->actual_bytes,address=(uintptr_t)value;
 if(address<base||address>base+m->used||bytes>base+m->used-address)return ULONG_MAX;
 for(unsigned long i=0;i<m->attempt_count;i++){
  sl_sepol_memory_attempt *a=&m->actual_attempts[i];
  if(a->operation!=SL_SEPOL_ALLOCATE&&a->operation!=SL_SEPOL_ZERO_ALLOCATE&&a->operation!=SL_SEPOL_REALLOCATE&&a->operation!=SL_SEPOL_REALLOCATE_ARRAY)continue;
  if(a->entered&&a->returned&&!a->error&&a->result==value&&a->offset==address-base&&a->extent>=bytes&&a->extent<=m->used-a->offset)return i;
 }
 return ULONG_MAX;
}
static long final_context_mls(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,sl_linux_selinux_context *o,unsigned long ordinal){
 if(!o->context_original)return 0;sl_sepol_memory_owner *m=&o->memory;long result;
 unsigned long context_allocation=final_context_allocation(m,o->context_original,sizeof(*o->context_original));if(context_allocation==ULONG_MAX)return -EINVAL;
 for(unsigned level=0;level<2;level++){
  mls_level_t *l=&o->context_original->range.level[level];ebitmap_node_t *node=l->cat.node;unsigned long visited=0;
  uint64_t state[10]={level,l->sens,l->cat.highbit,context_allocation,node!=NULL,0,0,0,0,0};
  result=final_parser_state(r,emit,count,extent,o,state,ordinal,0,UINT64_MAX-26);if(result)return result;
  while(node){
   if(visited>=m->attempt_count)return -ENOSPC;
   unsigned long allocation=final_context_allocation(m,node,sizeof(*node));if(allocation==ULONG_MAX||(uintptr_t)node%_Alignof(ebitmap_node_t))return -EINVAL;
   result=final_source_span(r,(const unsigned char*)node,sizeof(*node));if(result)return result;
   unsigned long next=ULONG_MAX;if(node->next){next=final_context_allocation(m,node->next,sizeof(*node));if(next==ULONG_MAX)return -EINVAL;}
   uint64_t values[10]={level,visited,allocation,next,node->startbit,node->map,(uint64_t)(uintptr_t)node,(uint64_t)(uintptr_t)node->next,0,0};
   result=final_parser_state(r,emit,count,extent,node,values,ordinal,1,UINT64_MAX-26);if(result)return result;
   if(!emit)result=installation_budget(r,count,extent,sizeof(*node));else result=final_add(r,node,(const unsigned char*)node,sizeof(*node),27,UINT64_MAX-26,ordinal,(long)level);if(result)return result;
   node=node->next;visited++;
  }
 }
 return 0;
}
static unsigned long final_policy_allocation(sl_sepol_memory_owner *m,policydb_t *policy,const void *value,unsigned long bytes,unsigned long *offset){
 uintptr_t at=(uintptr_t)value,root=(uintptr_t)policy;if(!value||!bytes||bytes>UINTPTR_MAX-at)return ULONG_MAX;
 if(at>=root&&at-root<=sizeof(*policy)&&bytes<=sizeof(*policy)-(at-root)){*offset=at-root;return ULONG_MAX-1;}
 uintptr_t base=(uintptr_t)m->actual_bytes;if(!base||m->used>m->actual_capacity||m->used>UINTPTR_MAX-base||at<base||at-base>m->used||bytes>m->used-(at-base))return ULONG_MAX;
 for(unsigned long i=0;i<m->attempt_count;i++){
  sl_sepol_memory_attempt *a=&m->actual_attempts[i];
  if(!a->entered||!a->returned||a->error||!a->result||a->offset>m->used||a->extent>m->used-a->offset||(uintptr_t)a->result!=base+a->offset)continue;
  if(a->operation!=SL_SEPOL_ALLOCATE&&a->operation!=SL_SEPOL_ZERO_ALLOCATE&&a->operation!=SL_SEPOL_REALLOCATE&&a->operation!=SL_SEPOL_REALLOCATE_ARRAY&&a->operation!=SL_SEPOL_DUPLICATE&&a->operation!=SL_SEPOL_BOUNDED_DUPLICATE)continue;
  if(at-base<a->offset||at-base-a->offset>a->extent||bytes>a->extent-(at-base-a->offset))continue;
  *offset=at-base-a->offset;return i;
 }
 return ULONG_MAX;
}
static long final_policy_datums(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,sl_selinux_te_union *o,sl_sepol_memory_owner *m,unsigned long ordinal){
 struct datum_span {const void *body;unsigned long bytes,alignment;} spans[5]={
 {o->class_original,sizeof(class_datum_t),_Alignof(class_datum_t)},
 {o->permission_original,sizeof(perm_datum_t),_Alignof(perm_datum_t)},
 {o->common_original,sizeof(common_datum_t),_Alignof(common_datum_t)},
 {o->source_attributes,sizeof(ebitmap_t),_Alignof(ebitmap_t)},
 {o->target_attributes,sizeof(ebitmap_t),_Alignof(ebitmap_t)}};
 for(unsigned field=0;field<5;field++)if(spans[field].body){
  unsigned long offset=0,allocation=final_policy_allocation(m,o->actual_policy,spans[field].body,spans[field].bytes,&offset);
  if(allocation==ULONG_MAX||(uintptr_t)spans[field].body%spans[field].alignment)return -EINVAL;
  long result=final_source_span(r,spans[field].body,spans[field].bytes);if(result)return result;
  uint64_t values[10]={ordinal,field,allocation,offset,spans[field].bytes,(uint64_t)(uintptr_t)spans[field].body,0,0,0,0};
  result=final_parser_state(r,emit,count,extent,o,values,ordinal,field,UINT64_MAX-31);if(result)return result;
  if(!emit)result=installation_budget(r,count,extent,spans[field].bytes);else result=final_add(r,o,spans[field].body,spans[field].bytes,35,ordinal,field,0);if(result)return result;
 }
 return 0;
}
static long final_policy_helper(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,sl_selinux_te_union *o,sl_sepol_memory_owner *m,sl_selinux_helper_visit *v,const char **slots[3],unsigned long ordinal,unsigned long helper){
 const sl_selinux_lookup_source *static_source=NULL;unsigned source_slot=UINT_MAX;unsigned long static_index=ULONG_MAX;
 if(v->kind==SL_SELINUX_HASH_BUCKET||v->kind==SL_SELINUX_HASH_NODE){
  unsigned selected=0;for(unsigned i=0;i<3;i++)if(slots[i]&&v->name_slot_original==slots[i]){selected=1;source_slot=i;}
  if(!selected||*v->name_slot_original!=v->name_original)return -EINVAL;
  static_source=sl_selinux_lookup_source_for(v->name_original);
  if(v->name_source_original!=static_source)return -EINVAL;
  if(static_source){if(v->name_read_extent>static_source->extent)return -EINVAL;static_index=(unsigned long)(static_source-sl_selinux_lookup_sources);}
 }else if(v->name_slot_original||v->name_source_original)return -EINVAL;
 struct helper_span {const void *body;unsigned long bytes,alignment;} spans[9]={
 {v->avtab_original,sizeof(avtab_t),_Alignof(avtab_t)},
 {v->table_original,sizeof(hashtab_val_t),_Alignof(hashtab_val_t)},
 {v->hash_original,sizeof(hashtab_node_t),_Alignof(hashtab_node_t)},
 {v->bitmap_original,sizeof(ebitmap_t),_Alignof(ebitmap_t)},
 {v->bitmap_node_original,sizeof(ebitmap_node_t),_Alignof(ebitmap_node_t)},
 {v->filename_key_original,sizeof(filename_trans_key_t),_Alignof(filename_trans_key_t)},
 {v->filename_datum_original,sizeof(filename_trans_datum_t),_Alignof(filename_trans_datum_t)},
 {v->genfs_original,sizeof(genfs_t),_Alignof(genfs_t)},
 {v->ocontext_original,sizeof(ocontext_t),_Alignof(ocontext_t)}};
 long result;
 if(source_slot!=UINT_MAX){uint64_t source_values[10]={ordinal,helper,source_slot,static_index,static_source?static_source->extent:0,(uint64_t)(uintptr_t)v->name_slot_original,(uint64_t)(uintptr_t)v->name_original,0,0,0};result=final_parser_state(r,emit,count,extent,v,source_values,ordinal,11,UINT64_MAX-30);if(result)return result;}
 for(unsigned field=0;field<9;field++)if(spans[field].body){
  unsigned long offset=0,allocation=final_policy_allocation(m,o->actual_policy,spans[field].body,spans[field].bytes,&offset);
  if(allocation==ULONG_MAX||(uintptr_t)spans[field].body%spans[field].alignment)return -EINVAL;
  result=final_source_span(r,spans[field].body,spans[field].bytes);if(result)return result;
  uint64_t values[10]={ordinal,helper,field,allocation,offset,spans[field].bytes,(uint64_t)(uintptr_t)spans[field].body,0,0,0};
  result=final_parser_state(r,emit,count,extent,v,values,ordinal,field,UINT64_MAX-30);if(result)return result;
  if(!emit)result=installation_budget(r,count,extent,spans[field].bytes);else result=final_add(r,v,spans[field].body,spans[field].bytes,32,ordinal,helper,(long)field);if(result)return result;
 }
 /* Header ownership precedes bucket geometry. Retain the actual consumed
  * pointer slot once per bucket visit, not the entire table for every node. */
 if(v->kind==SL_SELINUX_HASH_BUCKET||v->kind==SL_SELINUX_AVTAB_BUCKET){
  unsigned field=v->kind==SL_SELINUX_HASH_BUCKET?0:1;
  const void *body=field?(v->avtab_original?v->avtab_original->htable:NULL):(v->table_original?v->table_original->htable:NULL);
  unsigned long entries=field?(v->avtab_original?v->avtab_original->nslot:0):(v->table_original?v->table_original->size:0);
  unsigned long width=field?sizeof(avtab_ptr_t):sizeof(hashtab_ptr_t),offset=0;
  if(!body||!entries||entries>ULONG_MAX/width||v->bucket>=entries)return -EINVAL;
  unsigned long allocation=final_policy_allocation(m,o->actual_policy,body,entries*width,&offset);
  if(allocation>=ULONG_MAX-1||(uintptr_t)body%_Alignof(void*))return -EINVAL;
  const unsigned char *slot=(const unsigned char*)body+v->bucket*width;
  result=final_source_span(r,slot,width);if(result)return result;
  uint64_t values[10]={ordinal,helper,field,allocation,offset+v->bucket*width,width,entries,v->bucket,(uint64_t)(uintptr_t)slot,0};
  result=final_parser_state(r,emit,count,extent,v,values,ordinal,12+field,UINT64_MAX-30);if(result)return result;
  if(!emit)result=installation_budget(r,count,extent,width);else result=final_add(r,v,slot,width,34,ordinal,helper,(long)field);if(result)return result;
 }
 if(v->kind==SL_SELINUX_HASH_NODE&&(!v->hash_original||v->hash_original->key!=v->key_original))return -EINVAL;
 if(v->kind==SL_SELINUX_FILENAME_NODE&&(!v->filename_key_original||v->filename_key_original->name!=v->key_original))return -EINVAL;
 if(v->kind==SL_SELINUX_GENFS_NODE&&(!v->genfs_original||v->genfs_original->fstype!=v->name_original))return -EINVAL;
 if(v->kind==SL_SELINUX_OCONTEXT_NODE&&(v->name_original||v->key_original)&&(!v->ocontext_original||v->ocontext_original->u.name!=(v->name_original?v->name_original:v->key_original)))return -EINVAL;
 const char *names[2]={v->name_original,v->key_original};unsigned long sizes[2]={v->name_read_extent,v->key_read_extent};unsigned terminated[2]={v->name_terminated,v->key_terminated};
 for(unsigned field=0;field<2;field++){
  if(!sizes[field]){if(terminated[field])return -EINVAL;continue;}
  if(!names[field]||sizes[field]>o->actual_string_capacity||terminated[field]>1)return -EINVAL;
  unsigned long offset=0,allocation;
  /* HASH lookup names are static original arguments of the closed native
   * callers. Keys and GENFS/filename strings belong to the parsed policy. */
  if(!field&&static_source)allocation=ULONG_MAX-2;
  else{allocation=final_policy_allocation(m,o->actual_policy,names[field],sizes[field],&offset);if(allocation>=ULONG_MAX-1)return -EINVAL;}
  result=final_source_span(r,(const unsigned char*)names[field],sizes[field]);if(result)return result;
  if(terminated[field]&&names[field][sizes[field]-1])return -EINVAL;
  uint64_t values[10]={ordinal,helper,field,allocation,offset,sizes[field],terminated[field],v->compared,(uint64_t)(uintptr_t)names[field],0};
  result=final_parser_state(r,emit,count,extent,v,values,ordinal,9+field,UINT64_MAX-30);if(result)return result;
  if(!emit)result=installation_budget(r,count,extent,sizes[field]);else result=final_add(r,v,(const unsigned char*)names[field],sizes[field],33,ordinal,helper,(long)field);if(result)return result;
 }
 return 0;
}
/* Closed existing traversal roots; helper ABI is occurrence metadata, not
 * pointer-only proof of its still separately required typed referents. */
static long final_policy_traversals(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_startup *s=r->prepared_controller->startup_original;
 struct traversal_roster {void *base;unsigned long capacity,stride,offset;} rows[14]={
 {s->actual_control_access,s->actual_control_access_capacity,sizeof(*s->actual_control_access),offsetof(sl_linux_control_access,inventory.custody)},
 {s->actual_reachability,s->actual_reachability_capacity,sizeof(*s->actual_reachability),offsetof(sl_linux_context_reachability,graph.matrix.custody)},
 {s->actual_candidate_payloads,s->actual_candidate_payload_capacity,sizeof(*s->actual_candidate_payloads),offsetof(sl_linux_candidate_payload_context,reachable.graph.matrix.custody)},
 {s->actual_candidate_socket_creates,s->actual_candidate_socket_create_capacity,sizeof(*s->actual_candidate_socket_creates),offsetof(sl_linux_candidate_socket_create,inventory.custody)},
 {s->candidate_socket_uses,s->candidate_socket_use_capacity,sizeof(*s->candidate_socket_uses),offsetof(sl_linux_candidate_socket_create,inventory.custody)},
 {s->actual_candidate_ptrace,s->actual_candidate_ptrace_capacity,sizeof(*s->actual_candidate_ptrace),offsetof(sl_linux_candidate_ptrace,inventory.custody)},
 {s->actual_candidate_owner_entry,s->actual_candidate_owner_entry_capacity,sizeof(*s->actual_candidate_owner_entry),offsetof(sl_linux_candidate_ptrace,inventory.custody)},
 {s->actual_candidate_file_entry,s->actual_candidate_file_entry_capacity,sizeof(*s->actual_candidate_file_entry),offsetof(sl_linux_candidate_file_entry,matrix.custody)},
 {s->actual_candidate_file_entry,s->actual_candidate_file_entry_capacity,sizeof(*s->actual_candidate_file_entry),offsetof(sl_linux_candidate_file_entry,exec_type.custody)},
 {s->archive_accesses,s->archive_access_capacity,sizeof(*s->archive_accesses),offsetof(sl_linux_archive_access,inventory.custody)},
 {s->archive_creations,s->archive_creation_capacity,sizeof(*s->archive_creations),offsetof(sl_linux_archive_creation,types.custody)},
 {s->migration_labels,s->migration_label_capacity,sizeof(*s->migration_labels),offsetof(sl_linux_migration_labels,labels.targets.custody)},
 {s->migration_labels,s->migration_label_capacity,sizeof(*s->migration_labels),offsetof(sl_linux_migration_labels,labels.filesystem.custody)},
 {s->migration_access,s->migration_access_capacity,sizeof(*s->migration_access),offsetof(sl_linux_migration_access,inventory.custody)}};
 unsigned long total=0;long result;
 for(unsigned category=0;category<14;category++){
  if(rows[category].capacity>ULONG_MAX/rows[category].stride||rows[category].capacity>ULONG_MAX-total)return -EINVAL;
  result=final_source_span(r,rows[category].base,rows[category].capacity*rows[category].stride);if(result)return result;total+=rows[category].capacity;
 }
 if(s->actual_parse_capacity>ULONG_MAX/sizeof(*s->actual_parses))return -EINVAL;
 result=final_source_span(r,(const unsigned char*)s->actual_parses,s->actual_parse_capacity*sizeof(*s->actual_parses));if(result)return result;
 unsigned long ordinal=0;
 for(unsigned category=0;category<14;category++)for(unsigned long index=0;index<rows[category].capacity;index++,ordinal++){
  sl_selinux_te_union *o=(sl_selinux_te_union*)((unsigned char*)rows[category].base+index*rows[category].stride+rows[category].offset);
  if(!o->attempted)continue;
  if(!o->returned||o->count>o->actual_capacity||o->helper_count>o->actual_helper_capacity||o->actual_capacity>ULONG_MAX/sizeof(*o->actual_visits)||o->actual_helper_capacity>ULONG_MAX/sizeof(*o->actual_helpers))return -EINVAL;
  unsigned long parser=ULONG_MAX;for(unsigned long j=0;j<s->actual_parse_capacity;j++)if(o->actual_policy==&s->actual_parses[j].parser.policy)parser=j;
  if(parser==ULONG_MAX)return -EINVAL;
  if(category==12){
   /* GENFS derives the filesystem subset inside the SAME parent occurrence;
    * it sets actual_policy but does not invent a second policy_original. */
   sl_linux_migration_labels *parent=&s->migration_labels[index];
   if(!parent->labels.returned||!parent->labels.targets.custody.returned||o->actual_policy!=parent->labels.targets.custody.actual_policy||(o->policy_original&&o->policy_original!=o->actual_policy))return -EINVAL;
  }else if(o->policy_original!=o->actual_policy)return -EINVAL;
  sl_linux_policy_parse *p=&s->actual_parses[parser];sl_sepol_memory_owner *m=&p->parser.memory;
  if(!p->returned||!p->parser.returned||m->active||m->used>m->actual_capacity||m->attempt_count>m->actual_attempt_capacity||m->actual_attempt_capacity>ULONG_MAX/sizeof(*m->actual_attempts))return -EINVAL;
  result=final_source_span(r,(const unsigned char*)m->actual_attempts,m->actual_attempt_capacity*sizeof(*m->actual_attempts));if(result)return result;
  result=final_source_span(r,(const unsigned char*)o->actual_visits,o->actual_capacity*sizeof(*o->actual_visits));if(result)return result;
  result=final_source_span(r,(const unsigned char*)o->actual_helpers,o->actual_helper_capacity*sizeof(*o->actual_helpers));if(result)return result;
  uint64_t states[3][10]={{category,index,ordinal,parser,o->attempted,o->returned,(uint64_t)o->result,o->count,o->actual_capacity,o->helper_count},
   {o->actual_helper_capacity,o->actual_string_capacity,o->actual_source,o->actual_target,o->actual_class,o->actual_permission,o->unconditional,o->conditional,o->possible,0},
   {(uint64_t)(uintptr_t)o->class_original,(uint64_t)(uintptr_t)o->permission_original,(uint64_t)(uintptr_t)o->common_original,(uint64_t)(uintptr_t)o->source_attributes,(uint64_t)(uintptr_t)o->target_attributes,0,0,0,0,0}};
  for(unsigned j=0;j<3;j++){result=final_parser_state(r,emit,count,extent,o,states[j],ordinal,j,UINT64_MAX-27);if(result)return result;}
  result=final_policy_datums(r,emit,count,extent,o,m,ordinal);if(result)return result;
  for(unsigned long j=0;j<o->count;j++){
   sl_selinux_rule_visit *v=&o->actual_visits[j];unsigned table;
   if(v->table_original==&p->parser.policy.te_avtab)table=0;else if(v->table_original==&p->parser.policy.te_cond_avtab)table=1;else return -EINVAL;
   unsigned long allocation=final_context_allocation(m,v->node_original,sizeof(*v->node_original));
   if(allocation==ULONG_MAX||(uintptr_t)v->node_original%_Alignof(struct avtab_node)||v->conditional!=table)return -EINVAL;
   result=final_source_span(r,(const unsigned char*)v->node_original,sizeof(*v->node_original));if(result)return result;
   uint64_t values[10]={category,index,ordinal,parser,j,table,v->bucket,v->conditional,v->matched,allocation};
   result=final_parser_state(r,emit,count,extent,v,values,ordinal,3,UINT64_MAX-28);if(result)return result;
   if(!emit)result=installation_budget(r,count,extent,sizeof(*v->node_original));else result=final_add(r,v,(const unsigned char*)v->node_original,sizeof(*v->node_original),30,UINT64_MAX-28,ordinal,(long)j);if(result)return result;
  }
  const char **name_slots[3]={&o->actual_class_name,&o->actual_permission_name,NULL};
  if(category==0||category==3||category==4||category==5||category==6||category==9||category==13){
   sl_selinux_access_sources *a=(sl_selinux_access_sources*)o;name_slots[1]=&a->permission_names[0];name_slots[2]=&a->permission_names[1];
  }else if(category==1||category==2||category==7){
   sl_selinux_transition_matrix *a=(sl_selinux_transition_matrix*)o;name_slots[1]=&a->permission_names[0];name_slots[2]=&a->permission_names[1];
  }else if(category==11||category==12){name_slots[0]=&s->migration_labels[index].labels.class_names[0];name_slots[1]=&s->migration_labels[index].labels.class_names[1];name_slots[2]=NULL;}
  for(unsigned long j=0;j<o->helper_count;j++){
   sl_selinux_helper_visit *v=&o->actual_helpers[j];
   if(v->kind<SL_SELINUX_HASH_BUCKET||v->kind>SL_SELINUX_OCONTEXT_NODE)return -EINVAL;
   result=final_policy_helper(r,emit,count,extent,o,m,v,name_slots,ordinal,j);if(result)return result;
   if(!emit)result=installation_budget(r,count,extent,sizeof(*v));else result=final_add(r,v,(const unsigned char*)v,sizeof(*v),31,UINT64_MAX-29,ordinal,(long)j);if(result)return result;
  }
 }
 return 0;
}
/* Closed source-owned categories; the whole containing rosters are checked
 * before any context metadata or allocation referent is consumed. */
static long final_contexts(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_startup *s=r->prepared_controller->startup_original;
 struct context_roster {void *base;unsigned long count,stride,offset;} rows[8]={
 {s->actual_contexts,s->actual_context_capacity,sizeof(*s->actual_contexts),0},
 {s->actual_exec_contexts,s->actual_exec_context_capacity,sizeof(*s->actual_exec_contexts),0},
 {s->actual_control_contexts,s->actual_control_context_capacity,sizeof(*s->actual_control_contexts),0},
 {s->actual_archive_contexts,s->actual_archive_context_capacity,sizeof(*s->actual_archive_contexts),0},
 {s->archive_fscreate_contexts,s->archive_fscreate_context_capacity,sizeof(*s->archive_fscreate_contexts),0},
 {s->actual_candidate_payloads,s->actual_candidate_payload_capacity,sizeof(*s->actual_candidate_payloads),offsetof(sl_linux_candidate_payload_context,context)},
 {s->actual_candidate_socket_contexts,s->actual_candidate_socket_context_capacity,sizeof(*s->actual_candidate_socket_contexts),offsetof(sl_linux_candidate_socket_context,context)},
 {s->actual_candidate_preexec,s->actual_candidate_preexec_capacity,sizeof(*s->actual_candidate_preexec),offsetof(sl_linux_candidate_preexec,child_context)}};
 unsigned long total=0;long result;
 for(unsigned category=0;category<8;category++){
  if(rows[category].count>ULONG_MAX/rows[category].stride||rows[category].count>ULONG_MAX-total)return -EINVAL;
  result=final_source_span(r,rows[category].base,rows[category].count*rows[category].stride);if(result)return result;total+=rows[category].count;
 }
 if(s->actual_parse_capacity>ULONG_MAX/sizeof(*s->actual_parses))return -EINVAL;
 result=final_source_span(r,(const unsigned char*)s->actual_parses,s->actual_parse_capacity*sizeof(*s->actual_parses));if(result)return result;
 unsigned long ordinal=0;
 for(unsigned category=0;category<8;category++)for(unsigned long index=0;index<rows[category].count;index++,ordinal++){
  sl_linux_selinux_context *o=(sl_linux_selinux_context*)((unsigned char*)rows[category].base+index*rows[category].stride+rows[category].offset);
  if(!o->attempted)continue;sl_sepol_memory_owner *m=&o->memory;
  if(!o->returned||m->active||o->diagnostics.active||m->used>m->actual_capacity||m->attempt_count>m->actual_attempt_capacity||m->actual_attempt_capacity>ULONG_MAX/sizeof(*m->actual_attempts))return -EINVAL;
  unsigned long parser=ULONG_MAX,failure=ULONG_MAX;
  for(unsigned long j=0;j<s->actual_parse_capacity;j++)if(o->actual_parse==&s->actual_parses[j])parser=j;
  if(parser==ULONG_MAX||o->parse_original!=o->actual_parse||!o->actual_parse->returned||o->body!=o->actual_body||o->extent!=o->actual_extent||o->examined>o->actual_extent)return -EINVAL;
  result=final_source_span(r,o->actual_body,o->actual_extent);if(result)return result;
  result=final_source_span(r,(const unsigned char*)m->actual_attempts,m->actual_attempt_capacity*sizeof(*m->actual_attempts));if(result)return result;
  if(m->first_failure){if(m->first_failure==&m->exhaustion)failure=ULONG_MAX-1;else for(unsigned long j=0;j<m->attempt_count;j++)if(m->first_failure==&m->actual_attempts[j])failure=j;if(failure==ULONG_MAX)return -EINVAL;}
  if(o->context_original){
   uintptr_t start=(uintptr_t)m->actual_bytes,value=(uintptr_t)o->context_original;
   if(!start||m->used>UINTPTR_MAX-start||value<start||value>start+m->used||sizeof(*o->context_original)>start+m->used-value||value%_Alignof(context_struct_t))return -EINVAL;
   result=final_source_span(r,(const unsigned char*)o->context_original,sizeof(*o->context_original));if(result)return result;
  }
  if(o->context_original&&final_context_allocation(m,o->context_original,sizeof(*o->context_original))==ULONG_MAX)return -EINVAL;
  sl_linux_selinux_status_sample *status=&o->after_context;
  if(status->attempted&&(!o->actual_parse->actual_snapshot||status->status_original!=&o->actual_parse->actual_snapshot->status||status->actual_status!=status->status_original||!status->returned))return -EINVAL;
  uint64_t states[6][10]={{category,index,ordinal,parser,o->attempted,o->returned,(uint64_t)o->result,o->actual_extent,o->examined,o->context_original!=NULL},
  {o->call_entered,o->call_returned,(uint64_t)o->call_result,(uint64_t)o->call_errno,(uint64_t)o->status_result,o->user,o->role,o->type,(uint64_t)o->memory_begin,(uint64_t)o->diagnostic_begin},
  {(uint64_t)o->memory_finish,(uint64_t)o->diagnostic_finish,m->prepared,m->active,m->finished,m->failed,m->used,m->actual_capacity,m->attempt_count,failure},
  {o->self_attempted,o->self_returned,(uint64_t)o->self_result,o->exec_attempted,o->exec_returned,(uint64_t)o->exec_result,o->control_attempted,o->control_returned,(uint64_t)o->control_result,o->archive_attempted},
  {o->archive_returned,(uint64_t)o->archive_result,m->begin_entered,m->begin_returned,(uint64_t)m->begin_result,m->actual_attempt_capacity,m->exhaustion.entered,m->exhaustion.returned,0,0},
  {status->attempted,status->fields_read,status->returned,(uint64_t)status->result,status->sequence_before,status->sequence_after,status->version,status->enforcing,status->policyload,status->deny_unknown}};
  for(unsigned j=0;j<6;j++){result=final_parser_state(r,emit,count,extent,o,states[j],ordinal,j,UINT64_MAX-23);if(result)return result;}
  if(o->actual_extent){if(!emit)result=installation_budget(r,count,extent,o->actual_extent);else result=final_add(r,o,o->actual_body,o->actual_extent,25,UINT64_MAX-23,ordinal,0);if(result)return result;}
  if(o->context_original){if(!emit)result=installation_budget(r,count,extent,sizeof(*o->context_original));else result=final_add(r,o,(const unsigned char*)o->context_original,sizeof(*o->context_original),26,UINT64_MAX-23,ordinal,0);if(result)return result;}
  uint64_t coordinate[10]={category,index,ordinal,parser,0,0,0,0,0,0};result=final_parser_state(r,emit,count,extent,o,coordinate,ordinal,0,UINT64_MAX-24);if(result)return result;
  for(unsigned long j=0;j<m->attempt_count;j++){result=final_memory_attempt(r,emit,count,extent,m,&m->actual_attempts[j],ordinal,j,UINT64_MAX-24);if(result)return result;}
  if(m->exhaustion.entered){result=final_memory_attempt(r,emit,count,extent,m,&m->exhaustion,ordinal,ULONG_MAX,UINT64_MAX-24);if(result)return result;}
  result=final_context_mls(r,emit,count,extent,o,ordinal);if(result)return result;
  sl_sepol_diagnostic_owner *diagnostics=&o->diagnostics;
  if(diagnostics->count>diagnostics->actual_capacity||diagnostics->actual_capacity>ULONG_MAX/sizeof(*diagnostics->actual_frames))return -EINVAL;
  result=final_source_span(r,(const unsigned char*)diagnostics->actual_frames,diagnostics->actual_capacity*sizeof(*diagnostics->actual_frames));if(result)return result;
  unsigned long diagnostic_failure=ULONG_MAX;
  if(diagnostics->first_failure){if(diagnostics->first_failure==&diagnostics->exhaustion)diagnostic_failure=ULONG_MAX-1;else for(unsigned long j=0;j<diagnostics->count;j++)if(diagnostics->first_failure==&diagnostics->actual_frames[j])diagnostic_failure=j;if(diagnostic_failure==ULONG_MAX)return -EINVAL;}
  uint64_t diagnostic_state[10]={diagnostics->begin_entered,diagnostics->begin_returned,(uint64_t)diagnostics->begin_result,diagnostics->active,diagnostics->finished,diagnostics->exhausted,diagnostics->count,diagnostics->actual_capacity,diagnostics->actual_format_capacity,diagnostic_failure};
  result=final_diagnostic_state(r,emit,count,extent,diagnostics,diagnostic_state,ordinal,0,UINT64_MAX-25);if(result)return result;
  for(unsigned long j=0;j<diagnostics->count;j++){result=final_diagnostic_frame(r,emit,count,extent,diagnostics,m,&diagnostics->actual_frames[j],ordinal,j,UINT64_MAX-25);if(result)return result;}
  if(diagnostics->exhaustion.entered){result=final_diagnostic_frame(r,emit,count,extent,diagnostics,m,&diagnostics->exhaustion,ordinal,ULONG_MAX,UINT64_MAX-25);if(result)return result;}
 }
 return 0;
}
static long final_parsers(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_startup *startup=r->prepared_controller->startup_original;if(!startup->actual_parse_capacity)return 0;
 if(startup->actual_parse_capacity>ULONG_MAX/sizeof(*startup->actual_parses)||startup->actual_policy_capacity>ULONG_MAX/sizeof(*startup->actual_policies))return -EINVAL;
 long result=final_source_span(r,(const unsigned char*)startup->actual_parses,startup->actual_parse_capacity*sizeof(*startup->actual_parses));if(result)return result;
 result=final_source_span(r,(const unsigned char*)startup->actual_policies,startup->actual_policy_capacity*sizeof(*startup->actual_policies));if(result)return result;
 for(unsigned long i=0;i<startup->actual_parse_capacity;i++){
  sl_linux_policy_parse *root=&startup->actual_parses[i];if(!root->attempted)continue;
  sl_sepol_parse_owner *p=&root->parser;sl_sepol_memory_owner *memory=&p->memory;
  if(!root->returned||!p->returned||memory->active||memory->attempt_count>memory->actual_attempt_capacity||memory->actual_attempt_capacity>ULONG_MAX/sizeof(*memory->actual_attempts)||memory->used>memory->actual_capacity)return -EINVAL;
  unsigned long policy=ULONG_MAX;
  for(unsigned long j=0;j<startup->actual_policy_capacity;j++)if(root->actual_snapshot==&startup->actual_policies[j])policy=j;
  if(policy==ULONG_MAX||root->snapshot_original!=root->actual_snapshot||p->actual_body!=root->actual_snapshot->actual_body||p->actual_extent>root->actual_snapshot->actual_body_capacity||p->actual_extent!=root->actual_snapshot->length||p->body!=p->actual_body||p->extent!=p->actual_extent)return -EINVAL;
  result=final_source_span(r,(const unsigned char*)memory->actual_attempts,memory->actual_attempt_capacity*sizeof(*memory->actual_attempts));if(result)return result;
  result=final_source_span(r,p->actual_body,p->actual_extent);if(result)return result;
  unsigned long cursor=ULONG_MAX;
  if(p->read_cursor){uintptr_t begin=(uintptr_t)p->actual_body,value=(uintptr_t)p->read_cursor;if(p->actual_extent>UINTPTR_MAX-begin||value<begin||value>begin+p->actual_extent)return -EINVAL;cursor=value-begin;}
  unsigned long failure=ULONG_MAX;
  if(memory->first_failure){if(memory->first_failure==&memory->exhaustion)failure=ULONG_MAX-1;else for(unsigned long j=0;j<memory->attempt_count;j++)if(memory->first_failure==&memory->actual_attempts[j])failure=j;if(failure==ULONG_MAX)return -EINVAL;}
  uint64_t states[3][10]={{policy,root->attempted,root->returned,(uint64_t)root->result,(uint64_t)root->status_result,p->attempted,p->returned,(uint64_t)p->result,p->actual_extent,cursor},
 {p->init_entered,p->init_returned,(uint64_t)p->init_result,(uint64_t)p->init_errno,p->file_initialized,p->read_entered,p->read_returned,(uint64_t)p->read_result,(uint64_t)p->read_errno,p->read_remaining},
 {(uint64_t)p->memory_begin,(uint64_t)p->diagnostic_begin,(uint64_t)p->memory_finish,(uint64_t)p->diagnostic_finish,memory->begin_entered,memory->begin_returned,(uint64_t)memory->begin_result,memory->attempt_count,memory->used,failure}};
  for(unsigned j=0;j<3;j++){result=final_parser_state(r,emit,count,extent,j==0?(void*)root:(void*)p,states[j],i,j,UINT64_MAX-21);if(result)return result;}
  uint64_t owner_state[10]={memory->prepared,memory->active,memory->finished,memory->failed,memory->actual_capacity,memory->actual_attempt_capacity,memory->exhaustion.entered,memory->exhaustion.returned,0,0};
  result=final_parser_state(r,emit,count,extent,memory,owner_state,i,6,UINT64_MAX-21);if(result)return result;
  sl_linux_selinux_status_sample *status=&root->after_parse;
  if(status->attempted&&(status->status_original!=&root->actual_snapshot->status||status->actual_status!=status->status_original||!status->returned))return -EINVAL;
  uint64_t status_state[2][10]={{status->attempted,status->fields_read,status->returned,(uint64_t)status->result,status->sequence_before,status->sequence_after,status->version,status->enforcing,status->policyload,status->deny_unknown},
  {policy,status->status_original!=NULL,status->actual_status!=NULL,0,0,0,0,0,0,0}};
  for(unsigned j=0;j<2;j++){result=final_parser_state(r,emit,count,extent,status,status_state[j],i,7+j,UINT64_MAX-21);if(result)return result;}
  for(unsigned long j=0;j<memory->attempt_count;j++){result=final_memory_attempt(r,emit,count,extent,memory,&memory->actual_attempts[j],i,j,UINT64_MAX-21);if(result)return result;}
  if(memory->exhaustion.entered){result=final_memory_attempt(r,emit,count,extent,memory,&memory->exhaustion,i,ULONG_MAX,UINT64_MAX-21);if(result)return result;}
 }
 return 0;
}
static long final_service_attempt_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,unsigned long service,unsigned kind){
 if(!emit)return installation_budget(r,count,extent,80);unsigned char *body=r->actual_records[r->count].observation;
 for(unsigned n=0;n<10;n++)final_u64(body+8*n,fields[n]);
 return final_add(r,owner,body,80,7,UINT64_MAX-20,service,(long)kind);
}
static long final_service_attempts(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;if(!c->service_capacity)return 0;
 if(c->service_capacity>ULONG_MAX/sizeof(*c->service_roster)||c->reader_count>ULONG_MAX/sizeof(*c->readers)||c->reader_count>ULONG_MAX/sizeof(*c->forwarders))return -EINVAL;
 long result=final_entry_span(r,c->service_roster,c->service_capacity*sizeof(*c->service_roster));if(result)return result;
 result=final_entry_span(r,c->readers,c->reader_count*sizeof(*c->readers));if(result)return result;
 result=final_entry_span(r,c->forwarders,c->reader_count*sizeof(*c->forwarders));if(result)return result;
 /* Validate ALL potential reference rosters before following any release's
  * pointer or mutating an output header, including inactive service slots. */
 for(unsigned long i=0;i<c->service_capacity;i++){
  sl_linux_service_observation *o=&c->service_roster[i];if(!o->prepared&&!o->attempted)continue;
  if(o->capacity!=c->reader_count||o->capacity>ULONG_MAX/sizeof(*o->raw_joins)||o->capacity>ULONG_MAX/sizeof(*o->forward_joins)||o->capacity>ULONG_MAX/sizeof(*o->forward_releases))return -EINVAL;
  result=final_entry_span(r,o->raw_joins,o->capacity*sizeof(*o->raw_joins));if(result)return result;
  result=final_entry_span(r,o->forward_joins,o->capacity*sizeof(*o->forward_joins));if(result)return result;
  result=final_entry_span(r,o->forward_releases,o->capacity*sizeof(*o->forward_releases));if(result)return result;
 }
 for(unsigned long i=0;i<c->service_capacity;i++){
  sl_linux_service_observation *o=&c->service_roster[i];if(!o->attempted)continue;
  if(!o->prepared||!o->returned||o->controller_original!=c)return -EINVAL;
  for(unsigned long j=0;j<o->capacity;j++){
   sl_linux_reader_join *raw=&o->raw_joins[j];sl_linux_forward_join *forward=&o->forward_joins[j];sl_linux_forward_release *release=&o->forward_releases[j];
   if(raw->attempted){
    if(raw->worker_original!=&c->readers[j]||!raw->interpretation_returned)return -EINVAL;
    uint64_t fields[10]={j,raw->attempted,raw->returned,(uint64_t)raw->result,raw->interpretation_returned,(uint64_t)raw->interpretation_result,raw->joined_result!=NULL,raw->joined_result==raw->worker_original,raw==raw->worker_original->joined_by,0};
    result=final_service_attempt_state(r,emit,count,extent,raw,fields,i,0);if(result)return result;
   }
   if(forward->attempted){
    if(forward->worker_original!=&c->forwarders[j]||!forward->interpretation_returned)return -EINVAL;
    uint64_t fields[10]={j,forward->attempted,forward->returned,(uint64_t)forward->result,forward->interpretation_returned,(uint64_t)forward->interpretation_result,forward->joined_result!=NULL,forward->joined_result==forward->worker_original,forward==forward->worker_original->joined_by,0};
    result=final_service_attempt_state(r,emit,count,extent,forward,fields,i,1);if(result)return result;
   }
   if(release->attempted){
    if(release->worker_original!=&c->forwarders[j]||!release->returned)return -EINVAL;
    unsigned long source_service=ULONG_MAX,source_worker=ULONG_MAX;
    if(release->raw_join_original)for(unsigned long n=0;n<c->service_capacity;n++){
     sl_linux_service_observation *prior=&c->service_roster[n];if(!prior->prepared&&!prior->attempted)continue;
     for(unsigned long k=0;k<prior->capacity;k++)if(release->raw_join_original==&prior->raw_joins[k]){source_service=n;source_worker=k;}
    }
    if(release->raw_join_original&&source_service==ULONG_MAX)return -EINVAL;
    uint64_t fields[10]={j,release->attempted,release->returned,(uint64_t)release->result,source_service,source_worker,release->notification.attempted,release->notification.lock_returned,release->notification.signal_returned,release->notification.unlock_returned};
    result=final_service_attempt_state(r,emit,count,extent,release,fields,i,2);if(result)return result;
    uint64_t notification[10]={j,(uint64_t)release->notification.lock_result,(uint64_t)release->notification.signal_result,(uint64_t)release->notification.unlock_result,0,0,0,0,0,0};
    result=final_service_attempt_state(r,emit,count,extent,&release->notification,notification,i,3);if(result)return result;
   }
  }
 }
 return 0;
}
static long final_collector_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,uint64_t frame){
 if(!emit)return installation_budget(r,count,extent,80);unsigned char *body=r->actual_records[r->count].observation;
 for(unsigned i=0;i<10;i++)final_u64(body+8*i,fields[i]);return final_add(r,owner,body,80,7,UINT64_MAX-18,frame,0);
}
static long final_collector(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;
 if(!c->collector_create_entered&&!c->collector_create_returned&&!c->collector_join_capacity)return 0;
 if(c->collector_join_capacity>ULONG_MAX/sizeof(*c->collector_join_roster)||c->collector_join_index>c->collector_join_capacity)return -EINVAL;
 long result=final_entry_span(r,c->collector_join_roster,c->collector_join_capacity*sizeof(*c->collector_join_roster));if(result)return result;
 unsigned long joined_index=ULONG_MAX;
 for(unsigned long i=0;i<c->collector_join_capacity;i++)if(c->collector_joined_by==&c->collector_join_roster[i])joined_index=i;
 if(c->collector_joined_by&&joined_index==ULONG_MAX)return -EINVAL;
 uint64_t states[4][10]={{c->attempted,c->construction_returned,c->construction_body_completed,c->construction_body_returned,(uint64_t)c->construction_result,(uint64_t)c->construction_return_result,(uint64_t)c->birth_result,0,0,0},
 {c->collector_create_entered,c->collector_create_returned,(uint64_t)c->collector_create_result,atomic_load_explicit(&c->collector_entered,memory_order_acquire),atomic_load_explicit(&c->collector_returned,memory_order_acquire),c->collector_result==&c->collection,joined_index,c->collector_join_capacity,c->collector_join_index,0},
 {c->failure_continuation_attempted,c->failure_continuation_returned,c->failure_continuation_exhausted,(uint64_t)c->failure_continuation_result,c->drain_attempted,c->drain_returned,c->drain_exhausted,(uint64_t)c->drain_result,c->drained_by!=NULL,0},
 {c->normal_capture_attempted,c->normal_capture_returned,(uint64_t)c->normal_capture_drain_result,(uint64_t)c->normal_capture_result,c->capture_cut_original==c->drained_by,c->capture_original==r,0,0,0,0}};
 for(unsigned j=0;j<4;j++){result=final_collector_state(r,emit,count,extent,c,states[j],j);if(result)return result;}
 /* The fixed actual roster preserves earlier native EBUSY and rejected
  * interpretation attempts; returned index is not their custody frontier. */
 for(unsigned long i=0;i<c->collector_join_capacity;i++){
  sl_linux_collector_join *join=&c->collector_join_roster[i];if(!join->attempted)continue;
  if(join->controller_original!=c||!join->interpretation_returned)return -EINVAL;
  if(join->channel_original&&join->channel_original!=&c->channel)return -EINVAL;
  if(join->collection_original&&join->collection_original!=&c->collection)return -EINVAL;
  uint64_t fields[2][10]={{i,join->attempted,join->returned,(uint64_t)join->result,(uint64_t)join->interpretation_result,join->joined_result!=NULL,join->joined_result==&c->collection,join==c->collector_joined_by,join->interpretation_returned,0},
  {(uint64_t)join->channel_result,(uint64_t)join->collection_result,join->channel_original!=NULL,join->collection_original!=NULL,0,0,0,0,0,0}};
  for(unsigned j=0;j<2;j++){result=final_collector_state(r,emit,count,extent,join,fields[j],4+j);if(result)return result;}
 }
 return 0;
}
static long final_schedule_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,uint64_t frame){
 if(!emit)return installation_budget(r,count,extent,80);unsigned char *body=r->actual_records[r->count].observation;
 for(unsigned i=0;i<10;i++)final_u64(body+8*i,fields[i]);return final_add(r,owner,body,80,7,UINT64_MAX-17,frame,0);
}
static long final_schedule_source(sl_linux_public_controller *c,void *source,uint64_t *kind,uint64_t *first,uint64_t *second){
 unsigned matches=0;*kind=*first=*second=0;
 if(source==&c->channel){*kind=1;matches++;}if(source==&c->final_reader_task){*kind=2;matches++;}
 for(unsigned long i=0;i<c->reader_count;i++){
  if(source==&c->readers[i]){*kind=3;*first=i;matches++;}if(source==&c->forwarders[i]){*kind=4;*first=i;matches++;}
  for(unsigned long j=0;j<c->readers[i].prepared_capacity;j++)if(source==&c->readers[i].prepared_frames[j]){*kind=5;*first=i;*second=j;matches++;}
 }
 for(unsigned long i=0;i<c->service_capacity;i++)if(source==&c->service_roster[i].family){*kind=6;*first=i;matches++;}
 for(unsigned long i=0;i<c->channel.prepared_capacity;i++)if(source==&c->channel.prepared_requests[i].input){*kind=7;*first=i;matches++;}
 return matches==1?0:-EINVAL;
}
static long final_schedule(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;sl_linux_schedule *s=&c->schedule;if(!s->prepared)return 0;
 if(s->controller_original!=c||s->actual_wake_capacity>UINT32_MAX||s->actual_wake_capacity>ULONG_MAX/sizeof(*s->actual_wakes)||s->actual_wait_capacity>ULONG_MAX/sizeof(*s->actual_waits)||c->reader_count>ULONG_MAX/sizeof(*c->readers)||c->reader_count>ULONG_MAX/sizeof(*c->forwarders)||c->service_capacity>ULONG_MAX/sizeof(*c->service_roster)||c->channel.prepared_capacity>ULONG_MAX/sizeof(*c->channel.prepared_requests))return -EINVAL;
 long result=final_entry_span(r,s,sizeof(*s));if(result)return result;
 const void *roots[6]={s->actual_wakes,s->actual_waits,c->readers,c->forwarders,c->service_roster,c->channel.prepared_requests};unsigned long sizes[6]={s->actual_wake_capacity*sizeof(*s->actual_wakes),s->actual_wait_capacity*sizeof(*s->actual_waits),c->reader_count*sizeof(*c->readers),c->reader_count*sizeof(*c->forwarders),c->service_capacity*sizeof(*c->service_roster),c->channel.prepared_capacity*sizeof(*c->channel.prepared_requests)};
 for(unsigned j=0;j<6;j++){result=final_entry_span(r,roots[j],sizes[j]);if(result)return result;}
 for(unsigned long i=0;i<c->reader_count;i++){sl_linux_capture_worker *reader=&c->readers[i];if(reader->prepared_capacity>ULONG_MAX/sizeof(*reader->prepared_frames))return -EINVAL;result=final_entry_span(r,reader->prepared_frames,reader->prepared_capacity*sizeof(*reader->prepared_frames));if(result)return result;}
 uint64_t fields[10]={s->prepare_attempted,s->prepared,atomic_load_explicit(&s->generation,memory_order_acquire),s->actual_wake_capacity,s->actual_wait_capacity,s->actual_interval_ns,0,0,0,0};
 result=final_schedule_state(r,emit,count,extent,s,fields,0);if(result)return result;
 for(unsigned long i=0;i<s->actual_wake_capacity;i++){
  sl_linux_schedule_wake *w=&s->actual_wakes[i];uint64_t kind,first,second;
  if(w->schedule_original!=s||final_schedule_source(c,w->source_original,&kind,&first,&second))return -EINVAL;
  unsigned claimed=atomic_load_explicit(&w->claimed,memory_order_acquire),returned=atomic_load_explicit(&w->returned,memory_order_acquire);
  if(claimed&&(!returned||!w->call.source||!atomic_load_explicit(&w->call.returned,memory_order_acquire)))return -EINVAL;
  if(!claimed&&(returned||w->call.source))return -EINVAL;
  uint64_t fields[10]={i,kind,first,second,claimed,returned,w->generation_before,w->generation_after,(uint64_t)w->result,0};result=final_schedule_state(r,emit,count,extent,w,fields,1);if(result)return result;
  if(w->call.source){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&w->call,UINT64_MAX-17,i,0);if(result)return result;}
 }
 for(unsigned long i=0;i<s->actual_wait_capacity;i++){
  sl_linux_schedule_wait *w=&s->actual_waits[i];if(w->schedule_original!=s)return -EINVAL;
  unsigned claimed=atomic_load_explicit(&w->claimed,memory_order_acquire);if(!claimed){if(w->observed||w->returned||w->call.source)return -EINVAL;continue;}
  if(!w->observed||(w->call.source&&(!w->returned||!atomic_load_explicit(&w->call.returned,memory_order_acquire))))return -EINVAL;
  uint64_t fields[10]={i,claimed,w->observed,w->returned,w->generation_before,w->generation_after,(uint64_t)w->result,0,0,0};result=final_schedule_state(r,emit,count,extent,w,fields,2);if(result)return result;
  if(w->call.source){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&w->call,UINT64_MAX-17,i,1);if(result)return result;}
 }
 if(!emit)return installation_budget(r,count,extent,sizeof(s->interval));
 return final_add(r,s,(const unsigned char*)&s->interval,sizeof(s->interval),8,UINT64_MAX-17,3,0);
}
static long final_forward_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,uint64_t frame){
 if(!emit)return installation_budget(r,count,extent,80);
 unsigned char *body=r->actual_records[r->count].observation;for(unsigned i=0;i<10;i++)final_u64(body+8*i,fields[i]);
 return final_add(r,owner,body,80,7,UINT64_MAX-16,frame,0);
}
static long final_forward_notification(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,sl_linux_notification *n,uint64_t frame){
 uint64_t fields[10]={n->attempted,n->lock_returned,n->signal_returned,n->unlock_returned,(uint64_t)n->lock_result,(uint64_t)n->signal_result,(uint64_t)n->unlock_result,0,0,0};
 return final_forward_state(r,emit,count,extent,n,fields,frame);
}
static long final_forward(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;sl_linux_source_cut *cut=c->drained_by;
 if(!cut||!cut->reader_count)return 0;
 if(cut->readers_original!=c->readers||cut->forwarders_original!=c->forwarders||cut->reader_count!=c->reader_count||!c->readers||!c->forwarders||cut->reader_count>ULONG_MAX/sizeof(*c->readers)||cut->reader_count>ULONG_MAX/sizeof(*c->forwarders))return -EINVAL;
 long result=final_entry_span(r,c->readers,cut->reader_count*sizeof(*c->readers));if(result)return result;
 result=final_entry_span(r,c->forwarders,cut->reader_count*sizeof(*c->forwarders));if(result)return result;
 for(unsigned long i=0;i<cut->reader_count;i++){
  sl_linux_capture_worker *raw=&c->readers[i];sl_linux_forward_worker *w=&c->forwarders[i];sl_linux_reader_join *joined=raw->joined_by;sl_linux_forward_join *forward_join=w->joined_by;sl_linux_forward_release *release=w->release_original;
  if(w->reader_original!=raw||raw->forward_original!=w||!joined||!forward_join||!release||joined->worker_original!=raw||forward_join->worker_original!=w||release->worker_original!=w||release->raw_join_original!=joined||w->prepared_frames!=raw->prepared_frames||w->prepared_frame_capacity!=raw->prepared_capacity)return -EINVAL;
  if(!joined->returned||joined->result||!joined->interpretation_returned||joined->interpretation_result||joined->joined_result!=raw||!forward_join->returned||forward_join->result||!forward_join->interpretation_returned||forward_join->interpretation_result||forward_join->joined_result!=w)return -EINVAL;
  result=final_entry_span(r,joined,sizeof(*joined));if(result)return result;result=final_entry_span(r,forward_join,sizeof(*forward_join));if(result)return result;result=final_entry_span(r,release,sizeof(*release));if(result)return result;
  if(w->wait_count>w->prepared_wait_capacity||w->iteration_count>w->prepared_iteration_capacity||w->next_frame>w->prepared_frame_capacity||w->prepared_wait_capacity>ULONG_MAX/sizeof(*w->prepared_waits)||w->prepared_iteration_capacity>ULONG_MAX/sizeof(*w->prepared_iterations)||w->prepared_frame_capacity>ULONG_MAX/sizeof(*w->prepared_publications))return -EINVAL;
  result=final_entry_span(r,w->prepared_waits,w->prepared_wait_capacity*sizeof(*w->prepared_waits));if(result)return result;result=final_entry_span(r,w->prepared_iterations,w->prepared_iteration_capacity*sizeof(*w->prepared_iterations));if(result)return result;result=final_entry_span(r,w->prepared_publications,w->prepared_frame_capacity*sizeof(*w->prepared_publications));if(result)return result;
  sl_linux_call *calls[7]={&w->signal.thread,&w->signal.disposition,&w->signal.block,&w->signal.pending_before,&w->signal.pending_after,&w->signal.restore,&w->held_wait};
  for(unsigned j=0;j<7;j++)if(calls[j]->source&&!atomic_load_explicit(&calls[j]->returned,memory_order_acquire))return -EINVAL;
  uint64_t states[7][10]={{i,raw->prepared,raw->start_attempted,raw->create_returned,(uint64_t)raw->create_result,atomic_load(&raw->entered),atomic_load(&raw->returned),atomic_load(&raw->completed_count),raw->read_count,raw->outcome},
  {joined->attempted,joined->returned,(uint64_t)joined->result,joined->interpretation_returned,(uint64_t)joined->interpretation_result,joined->joined_result==raw,0,0,0,0},
  {w->prepare_attempted,w->prepared,w->mutex_init_entered,w->mutex_init_returned,(uint64_t)w->mutex_result,w->condition_init_entered,w->condition_init_returned,(uint64_t)w->condition_result,w->create_entered,w->create_returned},
  {(uint64_t)w->create_result,atomic_load(&w->entered),atomic_load(&w->forwarding_finished),atomic_load(&w->returned),w->next_frame,w->wait_count,w->iteration_count,(uint64_t)w->result,(uint64_t)w->forwarding_result,w->worker_result==w},
  {forward_join->attempted,forward_join->returned,(uint64_t)forward_join->result,forward_join->interpretation_returned,(uint64_t)forward_join->interpretation_result,forward_join->joined_result==w,0,0,0,0},
  {release->attempted,release->returned,(uint64_t)release->result,atomic_load(&w->release_attempt_original)==release,w->prepared_wait_capacity,w->prepared_iteration_capacity,w->prepared_frame_capacity,0,0,0},
  {w->signal.attempted,w->signal.blocked,w->signal.finish_attempted,w->signal.restored,w->signal.block_set,w->signal.original_mask,w->signal.pending_before_bits,w->signal.pending_after_bits,w->hold_word,0}};
  void *owners[7]={raw,joined,w,w,forward_join,release,&w->signal};
  for(unsigned j=0;j<7;j++){result=final_forward_state(r,emit,count,extent,owners[j],states[j],j);if(result)return result;}
  for(unsigned j=0;j<7;j++)if(calls[j]->source){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[j],UINT64_MAX-16,i,j);if(result)return result;}
  result=final_forward_notification(r,emit,count,extent,&w->terminal_notification,7);if(result)return result;result=final_forward_notification(r,emit,count,extent,&release->notification,8);if(result)return result;
  for(unsigned long j=0;j<w->prepared_frame_capacity;j++)if(w->prepared_publications[j].attempted){result=final_forward_notification(r,emit,count,extent,&w->prepared_publications[j],9+j);if(result)return result;}
  for(unsigned long j=0;j<w->wait_count;j++){sl_linux_forward_wait *wait=&w->prepared_waits[j];if(!wait->attempted||!wait->returned)return -EINVAL;uint64_t fields[10]={i,j,wait->attempted,wait->returned,(uint64_t)wait->result,0,0,0,0,0};result=final_forward_state(r,emit,count,extent,wait,fields,9);if(result)return result;}
  for(unsigned long j=0;j<=w->iteration_count;j++){
   sl_linux_forward_iteration *iteration=j==w->iteration_count?&w->terminal_lock:&w->prepared_iterations[j];
   uint64_t fields[10]={i,j,iteration->entered,iteration->lock_returned,iteration->unlock_returned,(uint64_t)iteration->lock_result,(uint64_t)iteration->unlock_result,j==w->iteration_count,0,0};result=final_forward_state(r,emit,count,extent,iteration,fields,10);if(result)return result;
  }
  result=final_entry_span(r,&w->signal.original_disposition,sizeof(w->signal.original_disposition));if(result)return result;
  if(!emit)result=installation_budget(r,count,extent,sizeof(w->signal.original_disposition));else result=final_add(r,&w->signal,(const unsigned char*)&w->signal.original_disposition,sizeof(w->signal.original_disposition),8,UINT64_MAX-16,11,0);if(result)return result;
 }
 return 0;
}
static long final_input_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,uint64_t frame){
 if(!emit)return installation_budget(r,count,extent,80);
 unsigned char *body=r->actual_records[r->count].observation;for(unsigned i=0;i<10;i++)final_u64(body+8*i,fields[i]);
 return final_add(r,owner,body,80,7,UINT64_MAX-15,frame,0);
}
static long final_input_body(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const void *body,unsigned long length,uint64_t frame,unsigned kind){
 long result=final_entry_span(r,body,length);if(result)return result;
 if(!emit)return installation_budget(r,count,extent,length);
 return final_add(r,owner,body,length,kind,UINT64_MAX-15,frame,0);
}
static long final_request_input(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;sl_linux_owner_channel *channel=&c->channel;
 if(!channel->attempted)return 0;
 if(!channel->prepared_requests||channel->prepared_capacity>ULONG_MAX/sizeof(*channel->prepared_requests))return -EINVAL;
 long result=final_entry_span(r,channel->prepared_requests,channel->prepared_capacity*sizeof(*channel->prepared_requests));if(result)return result;
 for(unsigned long i=0;i<channel->prepared_capacity;i++){
  sl_linux_terminal_request *q=&channel->prepared_requests[i];sl_linux_input_worker *w=&q->input;
  if(!q->prefix.attempted)continue;
  sl_linux_call *calls[11]={&q->window.query,&q->window.copy,&q->mode.query,&q->mode.change,&q->restoration.restore,&q->input_stop.send,&w->control_pair,&w->control_lifts[0].duplicate,&w->control_lifts[0].original_close,&w->control_lifts[1].duplicate,&w->control_lifts[1].original_close};
  for(unsigned j=0;j<11;j++)if(calls[j]->source&&!atomic_load_explicit(&calls[j]->returned,memory_order_acquire))return -EINVAL;
  unsigned long stop_index=ULONG_MAX,restore_index=ULONG_MAX;
  for(unsigned long j=0;j<channel->prepared_capacity;j++){
   if(w->stop_original==&channel->prepared_requests[j].input_stop)stop_index=j;
   if(q->mode.restore_original==&channel->prepared_requests[j].restoration)restore_index=j;
  }
  if(w->stop_original&&(stop_index==ULONG_MAX||w->stop_original->worker_original!=w))return -EINVAL;
  if(q->mode.restore_original&&(restore_index==ULONG_MAX||q->mode.restore_original->mode_original!=&q->mode))return -EINVAL;
  if(w->start_attempted){
   if(!w->start_returned||w->owner_original!=q||w->mode_original!=&q->mode||w->count>w->prepared_capacity||!w->prepared_frames||w->prepared_capacity>ULONG_MAX/sizeof(*w->prepared_frames))return -EINVAL;
   if(w->create_returned&&!w->create_result&&(!atomic_load_explicit(&w->returned,memory_order_acquire)||stop_index==ULONG_MAX||!w->stop_original->join_returned||w->stop_original->join_result||w->stop_original->joined_result!=w||!w->stop_original->interpretation_returned||w->stop_original->interpretation_result))return -EINVAL;
   result=final_entry_span(r,w->prepared_frames,w->prepared_capacity*sizeof(*w->prepared_frames));if(result)return result;
   for(unsigned long f=0;f<w->prepared_capacity;f++){
    sl_linux_input_frame *frame=&w->prepared_frames[f];
    if(frame->count>frame->prepared_capacity||frame->prepared_capacity>ULONG_MAX/sizeof(*frame->prepared_steps)||(!frame->prepared_steps&&frame->prepared_capacity))return -EINVAL;
    result=final_entry_span(r,frame->prepared_steps,frame->prepared_capacity*sizeof(*frame->prepared_steps));if(result)return result;
   }
  }
  uint64_t states[5][10]={{i,q->window.attempted,(uint64_t)q->window.pty_fd,q->mode.attempted,(uint64_t)q->mode.input_fd,q->restoration.attempted,restore_index,stop_index,0,0},
  {w->prepared,w->start_attempted,w->start_returned,(uint64_t)w->start_result,w->create_entered,w->create_returned,(uint64_t)w->create_result,atomic_load(&w->entered),atomic_load(&w->returned),w->outcome},
  {w->prepared_capacity,w->count,(uint64_t)w->master_fd,(uint64_t)w->input_fd,(uint64_t)w->raw_control[0],(uint64_t)w->raw_control[1],(uint64_t)w->control[0],(uint64_t)w->control[1],(uint64_t)w->control_lifts[0].held_fd,(uint64_t)w->control_lifts[1].held_fd},
  {q->input_stop.attempted,q->input_stop.send_completed,q->input_stop.join_entered,q->input_stop.join_returned,(uint64_t)q->input_stop.join_result,q->input_stop.interpretation_returned,(uint64_t)q->input_stop.interpretation_result,q->input_stop.worker_original!=NULL,q->input_stop.joined_result==q->input_stop.worker_original,0},
  {q->mode.input_original==w,q->mode.input_original!=NULL,w->worker_result==w,0,0,0,0,0,0,0}};
  for(unsigned j=0;j<5;j++){result=final_input_state(r,emit,count,extent,q,states[j],j);if(result)return result;}
  for(unsigned j=0;j<11;j++)if(calls[j]->source){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[j],UINT64_MAX-15,i,j);if(result)return result;}
  const void *bodies[4]={&q->window.window,&q->mode.original_state,&q->mode.raw_state,&q->input_stop.byte};unsigned long sizes[4]={sizeof(q->window.window),sizeof(q->mode.original_state),sizeof(q->mode.raw_state),1};
  for(unsigned j=0;j<4;j++){result=final_input_body(r,emit,count,extent,q,bodies[j],sizes[j],5+j,8);if(result)return result;}
  if(!w->start_attempted)continue;
  for(unsigned long f=0;f<w->count;f++){
   sl_linux_input_frame *frame=&w->prepared_frames[f];unsigned long length=frame->raw_count>0?(unsigned long)frame->raw_count:0;
   if(length>sizeof(frame->bytes)||frame->written>length)return -EINVAL;
   uint64_t fields[10]={i,f,(uint64_t)frame->raw_count,frame->written,frame->count,frame->prepared_capacity,0,0,0,0};
   result=final_input_state(r,emit,count,extent,frame,fields,9);if(result)return result;
   result=final_input_body(r,emit,count,extent,frame,frame->bytes,length,10,18);if(result)return result;
   for(unsigned long j=0;j<frame->count;j++){
    sl_linux_input_step *step=&frame->prepared_steps[j];
    if(step->kind<1||step->kind>4||!step->call.source||!atomic_load_explicit(&step->call.returned,memory_order_acquire)||(step->kind==2&&step->call.result!=frame->raw_count))return -EINVAL;
    uint64_t fields[10]={i,f,j,step->kind,0,0,0,0,0,0};result=final_input_state(r,emit,count,extent,step,fields,11);if(result)return result;
    if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&step->call,UINT64_MAX-15,f,j);if(result)return result;
    result=final_input_body(r,emit,count,extent,step,step->readiness,sizeof(step->readiness),12,8);if(result)return result;
   }
  }
 }
 return 0;
}
static long final_channel_state(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const uint64_t *fields,uint64_t frame){
 if(!emit)return installation_budget(r,count,extent,80);
 unsigned char *body=r->actual_records[r->count].observation;
 for(unsigned i=0;i<10;i++)final_u64(body+8*i,fields[i]);
 return final_add(r,owner,body,80,7,UINT64_MAX-14,frame,0);
}
static long final_channel_body(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,void *owner,const void *body,unsigned long length,uint64_t frame){
 long result=final_entry_span(r,body,length);if(result)return result;
 if(!emit)return installation_budget(r,count,extent,length);
 return final_add(r,owner,body,length,17,UINT64_MAX-14,frame,0);
}
static long final_channel_io(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent,sl_linux_custody_io *io,uint64_t frame){
 if(io->count>io->capacity||io->capacity>ULONG_MAX/sizeof(*io->attempts)||(!io->attempts&&io->capacity))return -EINVAL;
 long result=final_entry_span(r,io->attempts,io->capacity*sizeof(*io->attempts));if(result)return result;
 for(unsigned long i=0;i<io->count;i++)if(!io->attempts[i].source||!atomic_load_explicit(&io->attempts[i].returned,memory_order_acquire))return -EINVAL;
 uint64_t fields[10]={io->capacity,io->count,io->completed,0,0,0,0,0,0,0};
 result=final_channel_state(r,emit,count,extent,io,fields,frame);if(result)return result;
 for(unsigned long i=0;i<io->count;i++){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&io->attempts[i],UINT64_MAX-14,frame,i);if(result)return result;}
 return 0;
}
static long final_channel(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;sl_linux_owner_channel *o=&c->channel;sl_linux_local_collection *local=&c->collection;
 if(!o->attempted)return 0;
 if(!o->prepared||!o->returned||o->prepared_collection!=local||o->prepared_terminal!=&c->terminal||o->count>o->prepared_capacity||!o->prepared_requests||o->prepared_capacity>ULONG_MAX/sizeof(*o->prepared_requests))return -EINVAL;
 long result=final_entry_span(r,o,sizeof(*o));if(result)return result;
 result=final_entry_span(r,local,sizeof(*local));if(result)return result;
 result=final_entry_span(r,o->prepared_requests,o->prepared_capacity*sizeof(*o->prepared_requests));if(result)return result;
 unsigned long local_index=ULONG_MAX;
 for(unsigned long i=0;i<o->prepared_capacity;i++)if(o->local_prefix_original==&o->prepared_requests[i].prefix)local_index=i;
 if(o->local_prefix_original&&(local_index==ULONG_MAX||local->header_original!=o->local_prefix_original))return -EINVAL;
 if(local->body_length>local->prepared_capacity)return -EINVAL;
 result=final_entry_span(r,local->prepared_body,local->body_length);if(result)return result;
 result=final_entry_span(r,local->prepared_readback,local->body_length);if(result)return result;
 uint64_t fields[10]={o->prepared,o->attempted,o->returned,o->exhausted,o->prepared_capacity,o->count,(uint64_t)o->result,local_index,(uint64_t)o->prepared_endpoint,(uint64_t)o->prepared_pty_fd};
 result=final_channel_state(r,emit,count,extent,o,fields,0);if(result)return result;
 sl_linux_custody_io *ios[5]={&local->prepared_receive_header,&local->prepared_receive_body,&local->prepared_write_body,&local->prepared_read_body,&local->prepared_send_reply};
 for(unsigned i=0;i<5;i++){result=final_channel_io(r,emit,count,extent,ios[i],1+i);if(result)return result;}
 sl_linux_call *calls[3]={&local->flush,&local->directory_flush,&local->readback_eof};
 for(unsigned i=0;i<3;i++)if(calls[i]->source){if(!atomic_load_explicit(&calls[i]->returned,memory_order_acquire))return -EINVAL;if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[i],UINT64_MAX-14,6,i);if(result)return result;}
 uint64_t local_fields[10]={local->prepared,local->attempted,local->returned,local->readback_equal,local->prepared_capacity,local->body_length,(uint64_t)local->result,(uint64_t)local->prepared_storage,(uint64_t)local->prepared_directory,(uint64_t)local->prepared_endpoint};
 result=final_channel_state(r,emit,count,extent,local,local_fields,7);if(result)return result;
 const void *bodies[4]={local->header,local->reply,&local->eof_byte,local->prepared_readback};unsigned long sizes[4]={8,8,1,local->body_length};
 for(unsigned i=0;i<4;i++){result=final_channel_body(r,emit,count,extent,local,bodies[i],sizes[i],8+i);if(result)return result;}
 for(unsigned long i=0;i<o->prepared_capacity;i++){
  sl_linux_terminal_request *q=&o->prepared_requests[i];if(!q->prefix.attempted)continue;
  if(!q->prefix.prepared||!q->prefix.returned||q->prefix.prepared_endpoint!=o->prepared_endpoint||q->prefix.prepared_receive.completed>8||q->reply.prepared_length>sizeof(q->response)||q->reply.prepared_send.completed>q->reply.prepared_length)return -EINVAL;
  if(q->reply.attempted&&(!q->reply.returned||q->reply.prepared_endpoint!=o->prepared_endpoint||q->reply.prepared_bytes!=q->response))return -EINVAL;
  uint64_t state[10]={i,q->discriminant,q->routed,(uint64_t)q->operation_result,q->prefix.attempted,q->prefix.returned,(uint64_t)q->prefix.result,q->reply.attempted,q->reply.returned,(uint64_t)q->reply.result};
  result=final_channel_state(r,emit,count,extent,q,state,12);if(result)return result;
  result=final_channel_io(r,emit,count,extent,&q->prefix.prepared_receive,13);if(result)return result;
  result=final_channel_io(r,emit,count,extent,&q->reply.prepared_send,14);if(result)return result;
  result=final_channel_body(r,emit,count,extent,q,q->prefix.bytes,q->prefix.prepared_receive.completed,15);if(result)return result;
  /* Entire retained constructed response is distinct from actual sent extent. */
  result=final_channel_body(r,emit,count,extent,q,q->response,q->routed?sizeof(q->response):0,16);if(result)return result;
 }
 return 0;
}
static long final_terminal(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;sl_linux_terminal_views *t=&c->terminal;
 unsigned present=t->attempted||c->pty_open.source!=NULL;
 for(unsigned i=0;i<3;i++)present|=c->routes[i].attempted;
 if(!present)return 0;
 if(t->active_mode)return -EINVAL;
 if(t->attempted&&(t->input_original!=&c->entry.public_streams[0]||t->output_original!=&c->entry.public_streams[1]))return -EINVAL;
 long result=final_entry_span(r,t,sizeof(*t));if(result)return result;
 result=final_entry_span(r,c->routes,sizeof(c->routes));if(result)return result;
 sl_linux_call *calls[80]={&t->input_query,&t->output_query,&t->fallback_open,&t->fallback_query,&t->fallback_lift.duplicate,&t->fallback_lift.original_close,
 &c->private_query,&c->private_set,&c->parent_slave_close,&c->parent_custody_child_close,&c->parent_gate_child_close,
 &c->pty_open,&c->pty_unlock,&c->pty_peer,&c->master_flags,&c->master_nonblocking,&c->master_lift.duplicate,&c->master_lift.original_close,&c->slave_lift.duplicate,&c->slave_lift.original_close};unsigned used=20;
 for(unsigned i=0;i<3;i++){
  sl_linux_stream_route *route=&c->routes[i];
  if(route->attempted&&(route->public_original!=&c->entry.public_streams[i]||route->stream_index!=(int)i))return -EINVAL;
  sl_linux_call *group[12]={&route->query,&route->pipe,&route->pty_open,&route->pty_unlock,&route->pty_peer,&route->private_query,&route->private_set,&route->close_child,&route->lifts[0].duplicate,&route->lifts[0].original_close,&route->lifts[1].duplicate,&route->lifts[1].original_close};
  for(unsigned j=0;j<12;j++)calls[used++]=group[j];
 }
 for(unsigned i=0;i<used;i++){result=final_entry_span(r,calls[i],sizeof(*calls[i]));if(result)return result;if(calls[i]->source&&!atomic_load_explicit(&calls[i]->returned,memory_order_acquire))return -EINVAL;}
 const void *bodies[14]={&t->input_state,&t->output_state,&t->fallback_state,&c->private_state,&c->private_output_state};
 unsigned long sizes[14]={sizeof(t->input_state),sizeof(t->output_state),sizeof(t->fallback_state),sizeof(c->private_state),sizeof(c->private_output_state)};unsigned body_count=5;
 for(unsigned i=0;i<3;i++){sl_linux_stream_route *route=&c->routes[i];bodies[body_count]=&route->public_state;sizes[body_count++]=sizeof(route->public_state);bodies[body_count]=&route->private_state;sizes[body_count++]=sizeof(route->private_state);bodies[body_count]=&route->private_output_state;sizes[body_count++]=sizeof(route->private_output_state);}
 for(unsigned i=0;i<body_count;i++){result=final_entry_span(r,bodies[i],sizes[i]);if(result)return result;}
 uint64_t states[8][10]={{t->attempted,t->fallback_attempted,t->input_is_terminal,t->output_is_terminal,(uint64_t)t->selected_input_fd,(uint64_t)t->fallback_raw_fd,(uint64_t)t->fallback_lift.held_fd,0,0,0},
 {(uint64_t)c->pty_master,(uint64_t)c->pty_slave,(uint64_t)c->unlock_value,c->parent_views_attempted,c->parent_slave_close_attempted,c->parent_custody_child_close_attempted,c->parent_gate_child_close_attempted,(uint64_t)c->master_lift.held_fd,(uint64_t)c->slave_lift.held_fd,0}};
 for(unsigned i=0;i<3;i++){sl_linux_stream_route *route=&c->routes[i];uint64_t *a=states[2+2*i],*b=states[3+2*i];a[0]=route->attempted;a[1]=route->is_terminal;a[2]=route->shares_controlling;a[3]=route->observed_absent;a[4]=route->close_attempted;a[5]=(uint64_t)route->stream_index;a[6]=(uint64_t)route->owner_fd;a[7]=(uint64_t)route->child_fd;a[8]=(uint64_t)route->raw_endpoints[0];a[9]=(uint64_t)route->raw_endpoints[1];b[0]=(uint64_t)route->unlock_value;b[1]=(uint64_t)route->lifts[0].held_fd;b[2]=(uint64_t)route->lifts[1].held_fd;}
 for(unsigned i=0;i<8;i++){if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,states[i][j]);result=final_add(r,t,body,80,7,UINT64_MAX-13,i,0);}if(result)return result;}
 for(unsigned i=0;i<used;i++)if(calls[i]->source){if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[i],UINT64_MAX-13,8,i);if(result)return result;}
 for(unsigned i=0;i<body_count;i++){if(!emit)result=installation_budget(r,count,extent,sizes[i]);else result=final_add(r,t,bodies[i],sizes[i],8,UINT64_MAX-13,9+i,0);if(result)return result;}
 return 0;
}
static long final_namespace(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_controller *c=r->prepared_controller;sl_linux_parent_namespace *o=&c->namespace_observation;if(!o->attempted)return 0;
 sl_linux_pidfd_proc *info=&o->pidfd_info;
 if(!o->returned||o->result||o->birth_original!=&c->entry.birth||!info->returned||info->result||!info->eof||!info->pid_seen||info->birth_original!=o->birth_original||info->procfs_original!=&o->views[0]||info->length>info->actual_capacity||info->read_count>info->actual_read_capacity||info->read_count>ULONG_MAX/sizeof(*info->actual_reads))return -EINVAL;
 long result=final_entry_span(r,o,sizeof(*o));if(result)return result;
 result=final_entry_span(r,&c->namespace_target,sizeof(c->namespace_target));if(result)return result;
 result=final_entry_span(r,info->actual_reads,info->read_count*sizeof(*info->actual_reads));if(result)return result;
 result=final_entry_span(r,info->actual_bytes,info->length);if(result)return result;
 uint64_t states[9][10]={{o->attempted,o->returned,(uint64_t)o->result,o->closes_attempted,o->closes_returned,(uint64_t)o->close_result,0,0,0,0},
 {info->attempted,info->returned,(uint64_t)info->result,info->eof,info->pid_seen,(uint64_t)info->raw_pid,(uint64_t)info->actual_pidfd,(uint64_t)info->fd,info->length,info->read_count},
 {c->namespace_target.attempted,c->namespace_target.returned,(uint64_t)c->namespace_target.result,c->namespace_target.nsdelegate_observed,c->namespace_target.options_extent,c->namespace_target.checked_options,0,0,0,0}};
 for(unsigned i=0;i<6;i++){sl_linux_namespace_view *v=&o->views[i];uint64_t fields[10]={(uint64_t)v->fd,(uint64_t)v->raw_fd,(uint64_t)v->type,v->lift.attempted,(uint64_t)v->lift.original_fd,(uint64_t)v->lift.held_fd,v->close_held_attempted,v->close_raw_attempted,0,0};for(unsigned j=0;j<10;j++)states[3+i][j]=fields[j];}
 for(unsigned i=0;i<9;i++){if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,states[i][j]);result=final_add(r,i<3?(void*)o:(void*)&o->views[i-3],body,80,7,UINT64_MAX-12,i,0);}if(result)return result;}
 sl_linux_call *calls[55]={&o->root_before.wait,&o->root_after.wait,&info->open,&info->filesystem_query,&info->close,&info->lift.duplicate,&info->lift.close_original};unsigned used=7;
 for(unsigned i=0;i<6;i++){sl_linux_namespace_view *v=&o->views[i];calls[used++]=&v->acquire;calls[used++]=&v->filesystem_query;calls[used++]=&v->identity_query;calls[used++]=&v->type_query;calls[used++]=&v->close_held;calls[used++]=&v->close_raw;calls[used++]=&v->lift.duplicate;calls[used++]=&v->lift.close_original;}
 for(unsigned i=0;i<used;i++)if(calls[i]->source){if(!atomic_load_explicit(&calls[i]->returned,memory_order_acquire))return -EINVAL;if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[i],UINT64_MAX-12,9,i);if(result)return result;}
 for(unsigned long i=0;i<info->read_count;i++){sl_linux_call *call=&info->actual_reads[i];if(!call->source||!atomic_load_explicit(&call->returned,memory_order_acquire))return -EINVAL;if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,call,UINT64_MAX-12,10,i);if(result)return result;}
 const void *bodies[6]={&o->root_before.information,&o->root_before.usage,&o->root_after.information,&o->root_after.usage,&info->filesystem,info->actual_bytes};unsigned long lengths[6]={sizeof(o->root_before.information),sizeof(o->root_before.usage),sizeof(o->root_after.information),sizeof(o->root_after.usage),sizeof(info->filesystem),info->length};
 for(unsigned i=0;i<6;i++){if(!emit)result=installation_budget(r,count,extent,lengths[i]);else result=final_add(r,o,bodies[i],lengths[i],i==5?16:8,UINT64_MAX-12,11+i,0);if(result)return result;}
 for(unsigned i=0;i<6;i++){sl_linux_namespace_view *v=&o->views[i];const void *data[2]={&v->filesystem,&v->identity};unsigned long sizes[2]={sizeof(v->filesystem),sizeof(v->identity)};for(unsigned j=0;j<2;j++){if(!emit)result=installation_budget(r,count,extent,sizes[j]);else result=final_add(r,v,data[j],sizes[j],8,UINT64_MAX-12,17+2*i+j,0);if(result)return result;}}
 return 0;
}
static long final_fd_scan(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_fd_scan *scan=&r->prepared_controller->scan;sl_linux_fd_map *map=&r->prepared_controller->child_map;if(!scan->attempted)return 0;
 if(!scan->eof||!map->attempted||map->scan_original!=scan||map->views!=scan->views||map->count!=scan->view_count||scan->frame_count>scan->frame_capacity||scan->view_count>scan->view_capacity||map->action_count>map->action_capacity||scan->frame_count>ULONG_MAX/sizeof(*scan->frames)||scan->view_count>ULONG_MAX/sizeof(*scan->views)||map->action_count>ULONG_MAX/sizeof(*map->actions))return -EINVAL;
 const void *roots[5]={scan,map,scan->frames,scan->views,map->actions};unsigned long sizes[5]={sizeof(*scan),sizeof(*map),scan->frame_count*sizeof(*scan->frames),scan->view_count*sizeof(*scan->views),map->action_count*sizeof(*map->actions)};
 for(unsigned i=0;i<5;i++){long result=final_entry_span(r,roots[i],sizes[i]);if(result)return result;}
 if(!scan->frame_count)return -EINVAL;
 for(unsigned long i=0;i<scan->frame_count;i++){sl_linux_fd_scan_frame *frame=&scan->frames[i];if((i+1==scan->frame_count?frame->raw_count!=0:frame->raw_count<=0)||frame->scan_original!=scan||frame->raw_count<0||(unsigned long)frame->raw_count>frame->capacity||!frame->read.source||!atomic_load_explicit(&frame->read.returned,memory_order_acquire)||frame->read.result!=frame->raw_count)return -EINVAL;long result=final_entry_span(r,frame->bytes,(unsigned long)frame->raw_count);if(result)return result;}
 uint64_t fields[2][10]={{scan->attempted,scan->eof,(uint64_t)scan->directory_fd,scan->frame_count,scan->view_count,scan->frame_capacity,scan->view_capacity,0,0,0},{map->attempted,map->count,map->action_count,map->action_capacity,(uint64_t)map->payload_fd,(uint64_t)map->custody_fd,(uint64_t)map->pty_slave_fd,(uint64_t)map->standard_fds[0],(uint64_t)map->standard_fds[1],(uint64_t)map->standard_fds[2]}};
 long result;
 for(unsigned i=0;i<2;i++){if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,fields[i][j]);result=final_add(r,i?(void*)map:(void*)scan,body,80,7,UINT64_MAX-11,i,0);}if(result)return result;}
 sl_linux_call *calls[5]={&scan->open_directory,&scan->filesystem_query,&map->standard_state[0],&map->standard_state[1],&map->standard_state[2]};
 for(unsigned i=0;i<5;i++){if(!calls[i]->source||!atomic_load_explicit(&calls[i]->returned,memory_order_acquire))return -EINVAL;if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[i],UINT64_MAX-11,2,i);if(result)return result;}
 if(!emit)result=installation_budget(r,count,extent,sizeof(scan->filesystem));else result=final_add(r,scan,(const unsigned char*)&scan->filesystem,sizeof(scan->filesystem),8,UINT64_MAX-11,3,0);if(result)return result;
 for(unsigned long i=0;i<scan->frame_count;i++){
  sl_linux_fd_scan_frame *frame=&scan->frames[i];if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&frame->read,UINT64_MAX-11,4,i);if(result)return result;
  if(!emit)result=installation_budget(r,count,extent,(unsigned long)frame->raw_count);else result=final_add(r,frame,frame->bytes,(unsigned long)frame->raw_count,15,UINT64_MAX-11,i,frame->raw_count);if(result)return result;
 }
 for(unsigned long i=0;i<scan->view_count;i++){
  sl_linux_fd_view *view=&scan->views[i];unsigned long original_index=scan->frame_count;
  for(unsigned long j=0;j<scan->frame_count;j++)if(view->original==&scan->frames[j]){original_index=j;break;}
  if(original_index==scan->frame_count||!view->inspect_flags.source||!atomic_load_explicit(&view->inspect_flags.returned,memory_order_acquire))return -EINVAL;
  uint64_t state[10]={(uint64_t)view->fd,original_index,0,0,0,0,0,0,0,0};
  if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,state[j]);result=final_add(r,view,body,80,7,UINT64_MAX-11,5,i);}if(result)return result;
  if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&view->inspect_flags,UINT64_MAX-11,5,i);if(result)return result;
 }
 for(unsigned long i=0;i<map->action_count;i++){
  sl_linux_fd_action *action=&map->actions[i];unsigned long original_index=scan->view_count;
  for(unsigned long j=0;j<scan->view_count;j++)if(action->outer_original==&scan->views[j]){original_index=j;break;}
  if(original_index==scan->view_count)return -EINVAL;
  uint64_t state[10]={action->kind,(uint64_t)action->source_fd,(uint64_t)action->target_fd,original_index,0,0,0,0,0,0};
  if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,state[j]);result=final_add(r,action,body,80,7,UINT64_MAX-11,6,i);}if(result)return result;
 }
 return 0;
}
static long final_entry(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_entry *e=&r->prepared_controller->entry;if(!e->attempted)return 0;
 long result=final_entry_span(r,e,sizeof(*e));if(result)return result;
 result=final_shared_range(r,emit,count,extent,&e->bootstrap_range,UINT64_MAX-9,20);if(result)return result;
 if(e->fd_actions_attempted){result=final_shared_range(r,emit,count,extent,&e->fd_action_range,UINT64_MAX-9,21);if(result)return result;}
 for(unsigned i=0;i<2;i++){
  sl_linux_socket_peer *p=&e->custody_peers[i];sl_linux_socket_state *state=&e->custody_states[i];
  if(!p->returned||p->result||!p->framing_complete||p->extent>p->actual_capacity||(!p->actual_bytes&&p->extent)||!state->returned||state->result||!state->framing_complete)return -EINVAL;
  result=final_entry_span(r,p->actual_bytes,p->extent);if(result)return result;
 }
 uint64_t states[12][10]={{e->attempted,e->public_streams_attempted,e->public_streams_prepared,e->fd_actions_attempted,e->fd_action_input_count,e->fd_action_mapping_bytes,(uint64_t)e->fd_action_mapping_result,(uint64_t)e->mapping_result,0,0}};
 for(unsigned i=0;i<3;i++){sl_linux_public_stream *p=&e->public_streams[i];states[1+i][0]=(uint64_t)p->original_fd;states[1+i][1]=(uint64_t)p->held_fd;states[1+i][2]=p->observed_absent;}
 sl_linux_fd_lift *lifts[4]={&e->custody_lifts[0],&e->custody_lifts[1],&e->entry_lifts[0],&e->entry_lifts[1]};
 for(unsigned i=0;i<4;i++){states[4+i][0]=lifts[i]->attempted;states[4+i][1]=(uint64_t)lifts[i]->original_fd;states[4+i][2]=(uint64_t)lifts[i]->held_fd;}
 for(unsigned i=0;i<2;i++){
  sl_linux_socket_peer *p=&e->custody_peers[i];sl_linux_socket_state *q=&e->custody_states[i];
  uint64_t peer[10]={p->attempted,p->returned,p->framing_complete,p->endpoint_index,(uint64_t)p->descriptor,(uint64_t)p->domain,(uint64_t)p->type,(uint64_t)p->protocol,p->extent,(uint64_t)p->result};
  uint64_t state[10]={q->attempted,q->returned,q->framing_complete,q->endpoint_index,(uint64_t)q->descriptor,(uint64_t)q->accepting,q->local_length,q->peer_length,q->accept_length,(uint64_t)q->result};
  for(unsigned j=0;j<10;j++){states[8+i][j]=peer[j];states[10+i][j]=state[j];}
 }
 for(unsigned i=0;i<12;i++){
  if(!emit)result=installation_budget(r,count,extent,80);else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned j=0;j<10;j++)final_u64(body+8*j,states[i][j]);result=final_add(r,e,body,80,7,UINT64_MAX-9,i,0);}if(result)return result;
 }
 sl_linux_call *calls[32]={&e->shared_mapping,&e->custody_pair,&e->entry_pipe,&e->fd_action_mapping};unsigned used=4;
 for(unsigned i=0;i<3;i++){calls[used++]=&e->public_streams[i].original_flags;calls[used++]=&e->public_streams[i].duplicate;}
 for(unsigned i=0;i<4;i++){calls[used++]=&lifts[i]->duplicate;calls[used++]=&lifts[i]->close_original;}
 for(unsigned i=0;i<2;i++){sl_linux_socket_peer *p=&e->custody_peers[i];calls[used++]=&p->domain_call;calls[used++]=&p->type_call;calls[used++]=&p->protocol_call;calls[used++]=&p->security_call;sl_linux_socket_state *q=&e->custody_states[i];calls[used++]=&q->local_query;calls[used++]=&q->peer_query;calls[used++]=&q->accept_query;}
 for(unsigned i=0;i<used;i++)if(calls[i]->source){if(!atomic_load_explicit(&calls[i]->returned,memory_order_acquire))return -EINVAL;if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[i],UINT64_MAX-9,12,i);if(result)return result;}
 for(unsigned i=0;i<2;i++){
  sl_linux_socket_peer *p=&e->custody_peers[i];sl_linux_socket_state *q=&e->custody_states[i];
  const unsigned char *bodies[3]={p->actual_bytes,(const unsigned char*)&q->local_address,(const unsigned char*)&q->peer_address};unsigned long sizes[3]={p->extent,sizeof(q->local_address),sizeof(q->peer_address)};
  for(unsigned j=0;j<3;j++){if(!emit)result=installation_budget(r,count,extent,sizes[j]);else result=final_add(r,j?(void*)q:(void*)p,bodies[j],sizes[j],j?8:13,UINT64_MAX-9,13+3*i+j,0);if(result)return result;}
 }
 return 0;
}
/* Gate lives in the distinct static public owner, not in capture output. */
static long final_gate(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_gate_delivery *g=r->gate_original;if(!g)return 0;
 if(!g->attempted||!g->returned||g->result||g->controller_original!=r->prepared_controller||
    g->birth_original!=&r->prepared_controller->entry.birth||!g->bootstrap_original||
    g->bootstrap_original!=g->birth_original->bootstrap||g->bytes_original!=g->bootstrap_original->gate_expected||
    g->offset!=8||!g->count||g->count>8||!g->close_attempted||g->close_result||g->signal_result||g->restore_result)return -EINVAL;
 long result=final_source_span(r,(const unsigned char*)g,sizeof(*g));if(result)return result;
 result=final_source_span(r,g->bytes_original,8);if(result)return result;
 uint64_t states[3][10]={{g->attempted,g->returned,g->count,g->offset,(uint64_t)g->fd,(uint64_t)g->signal_result,(uint64_t)g->write_result,(uint64_t)g->close_result,(uint64_t)g->restore_result,(uint64_t)g->result},
 {g->signal.attempted,g->signal.blocked,g->signal.finish_attempted,g->signal.restored,g->signal.block_set,g->signal.original_mask,g->signal.pending_before_bits,g->signal.pending_after_bits,0,0},
 {g->close_attempted,g->write_failure_original!=NULL,0,0,0,0,0,0,0,0}};
 for(unsigned i=0;i<3;i++){
  if(!emit)result=installation_budget(r,count,extent,80);
  else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned n=0;n<10;n++)final_u64(body+8*n,states[i][n]);result=final_add(r,g,body,80,7,UINT64_MAX-8,i,0);}
  if(result)return result;
 }
 sl_linux_call *calls[15];unsigned used=0;
 for(unsigned i=0;i<g->count;i++)calls[used++]=&g->writes[i];calls[used++]=&g->close;
 calls[used++]=&g->signal.thread;calls[used++]=&g->signal.disposition;calls[used++]=&g->signal.block;
 calls[used++]=&g->signal.pending_before;calls[used++]=&g->signal.pending_after;calls[used++]=&g->signal.restore;
 for(unsigned i=0;i<used;i++)if(calls[i]->source){
  if(!atomic_load_explicit(&calls[i]->returned,memory_order_acquire))return -EINVAL;
  if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[i],UINT64_MAX-8,3,i);
  if(result)return result;
 }
 const unsigned char *bodies[2]={g->bytes_original,(const unsigned char*)&g->signal.original_disposition};
 unsigned long sizes[2]={8,sizeof(g->signal.original_disposition)};
 for(unsigned i=0;i<2;i++){
  if(!emit)result=installation_budget(r,count,extent,sizes[i]);else result=final_add(r,g,bodies[i],sizes[i],8,UINT64_MAX-8,4+i,0);
  if(result)return result;
 }
 return 0;
}
/* Two traversals of the same settled startup originals: complete preflight
 * first, representation construction only after ALL capture sources pass. */
static long final_installation(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_local_installation *i=&r->prepared_controller->startup_original->installation;
 sl_linux_installed_payload *p=&i->payload;long result;
 if(!i->attempted)return 0;
 if(!i->returned||(p->attempted&&!p->returned))return -EINVAL;
 if(!emit){result=final_source_span(r,(const unsigned char*)i,sizeof(*i));if(result)return result;}
 uint64_t states[7][10]={{i->attempted,i->returned,(uint64_t)i->result,(uint64_t)i->proc_fd,(uint64_t)i->executable_fd,(uint64_t)i->public_fd,(uint64_t)i->raw_link_count,i->directory_length,i->prepared_executable_capacity,i->prepared_directory_capacity},
 {p->attempted,p->returned,(uint64_t)p->result,(uint64_t)p->directory_fd,(uint64_t)p->payload_fd,p->directory_length,p->directory_capacity,0,0,0}};
 sl_linux_fd_lift *lifts[5]={&i->proc_lift,&i->executable_lift,&i->public_lift,&p->directory_lift,&p->payload_lift};
 for(unsigned n=0;n<5;n++){states[n+2][0]=lifts[n]->attempted;states[n+2][1]=(uint64_t)lifts[n]->original_fd;states[n+2][2]=(uint64_t)lifts[n]->held_fd;}
 for(unsigned n=0;n<7;n++){
  if(!emit)result=installation_budget(r,count,extent,80);
  else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned k=0;k<10;k++)final_u64(body+8*k,states[n][k]);result=final_add(r,n<2?(n?(void*)p:(void*)i):(void*)lifts[n-2],body,80,7,UINT64_MAX-3,n,0);}
  if(result)return result;
 }
 sl_linux_call *calls[21]={&i->open_proc,&i->proc_filesystem,&i->open_executable,&i->executable_identity,&i->readlink,&i->open_public,&i->public_identity,
 &p->open_directory,&p->open_payload,&p->directory_identity,&p->payload_identity};
 unsigned call_count=11;
 for(unsigned n=0;n<5;n++)if(lifts[n]->attempted&&lifts[n]->original_fd>=0&&lifts[n]->original_fd<3){calls[call_count++]=&lifts[n]->duplicate;calls[call_count++]=&lifts[n]->close_original;}
 for(unsigned n=0;n<call_count;n++){
  /* These exact fixed cells are initialized by the corresponding constructor
   * after it binds its prepared path; direct-adopted lifts have no call cells. */
  if((n<7&&!i->prepared_executable_path)||(n>=7&&n<11&&!p->prepared_locator))continue;
  if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[n],UINT64_MAX-3,n,0);
  if(result)return result;
 }
 const unsigned char *abi[7]={(const unsigned char*)&i->proc_fs,(const unsigned char*)&i->executable_state,(const unsigned char*)&i->public_state,
 (const unsigned char*)&p->directory_how,(const unsigned char*)&p->payload_how,(const unsigned char*)&p->directory_state,(const unsigned char*)&p->payload_state};
 unsigned long sizes[7]={sizeof(i->proc_fs),sizeof(i->executable_state),sizeof(i->public_state),sizeof(p->directory_how),sizeof(p->payload_how),sizeof(p->directory_state),sizeof(p->payload_state)};
 for(unsigned n=0;n<7;n++){
  if((n<3&&!i->prepared_executable_path)||(n>=3&&!p->prepared_locator))continue;
  if(!emit)result=installation_budget(r,count,extent,sizes[n]);else result=final_add(r,i,abi[n],sizes[n],8,UINT64_MAX-3,n,0);
  if(result)return result;
 }
 const unsigned char *paths[3]={(const unsigned char*)i->prepared_executable_path,(const unsigned char*)i->prepared_directory_path,(const unsigned char*)p->prepared_locator};
 unsigned long lengths[3]={i->raw_link_count>0?(unsigned long)i->raw_link_count:0,i->directory_length,p->directory_length};
 unsigned long capacities[3]={i->prepared_executable_capacity,i->prepared_directory_capacity,p->directory_capacity};
 for(unsigned n=0;n<3;n++){
  if(!paths[n]){if(lengths[n])return -EINVAL;continue;}
  if(lengths[n]>capacities[n])return -EINVAL;
  if(!emit){result=final_source_span(r,paths[n],lengths[n]);if(result)return result;result=installation_budget(r,count,extent,lengths[n]);}
  else result=final_add(r,n==2?(void*)p:(void*)i,paths[n],lengths[n],9,UINT64_MAX-3,n,(long)lengths[n]);
  if(result)return result;
 }
 return 0;
}
static long final_policies(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_public_startup *s=r->prepared_controller->startup_original;long result;
 if(s->actual_policy_capacity>ULONG_MAX/sizeof(*s->actual_policies)||(!s->actual_policies&&s->actual_policy_capacity))return -EINVAL;
 if(!emit){result=final_source_span(r,(const unsigned char*)s->actual_policies,s->actual_policy_capacity*sizeof(*s->actual_policies));if(result)return result;}
 for(unsigned long index=0;index<s->actual_policy_capacity;index++){
  sl_linux_selinux_snapshot *p=&s->actual_policies[index];sl_linux_selinux_status *status=&p->status;
  if(!p->attempted)continue;
  if(!p->returned||p->frame_count>p->actual_frame_capacity||p->frame_count>ULONG_MAX/sizeof(*p->actual_frames)||(!p->actual_frames&&p->frame_count)||p->length>p->actual_body_capacity||p->path_length>p->actual_path_capacity)return -EINVAL;
  if(!emit){
   result=final_source_span(r,(const unsigned char*)p->actual_frames,p->frame_count*sizeof(*p->actual_frames));if(result)return result;
   result=final_source_span(r,p->actual_body,p->length);if(result)return result;
   result=final_source_span(r,(const unsigned char*)p->actual_path,p->path_length);if(result)return result;
  }
  uint64_t states[7][10]={{p->attempted,p->returned,(uint64_t)p->result,(uint64_t)p->root_fd,(uint64_t)p->policy_fd,p->eof,p->frame_count,p->length,p->path_length,p->row_index},
   {p->actual_frame_capacity,p->actual_body_capacity,p->actual_path_capacity,0,0,0,0,0,0,0},
   {p->root_lift.attempted,(uint64_t)p->root_lift.original_fd,(uint64_t)p->root_lift.held_fd,0,0,0,0,0,0,0},
   {p->policy_lift.attempted,(uint64_t)p->policy_lift.original_fd,(uint64_t)p->policy_lift.held_fd,0,0,0,0,0,0,0},
   {status->attempted,status->returned,(uint64_t)status->result,(uint64_t)status->raw_fd,(uint64_t)status->fd,(uint64_t)status->raw_mapping,status->lift.attempted,(uint64_t)status->lift.original_fd,(uint64_t)status->lift.held_fd,0},
   {p->before_policy.attempted,p->before_policy.returned,(uint64_t)p->before_policy.result,p->before_policy.fields_read,p->before_policy.sequence_before,p->before_policy.sequence_after,p->before_policy.version,p->before_policy.enforcing,p->before_policy.policyload,p->before_policy.deny_unknown},
   {p->after_policy.attempted,p->after_policy.returned,(uint64_t)p->after_policy.result,p->after_policy.fields_read,p->after_policy.sequence_before,p->after_policy.sequence_after,p->after_policy.version,p->after_policy.enforcing,p->after_policy.policyload,p->after_policy.deny_unknown}};
  for(unsigned n=0;n<7;n++){
   if(!emit)result=installation_budget(r,count,extent,80);
   else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned k=0;k<10;k++)final_u64(body+8*k,states[n][k]);result=final_add(r,p,body,80,7,UINT64_MAX-7,index,n);}
   if(result)return result;
  }
  sl_linux_call *calls[18]={&p->root_open,&p->root_filesystem,&p->root_identity,&p->root_close,&p->policy_open,&p->policy_filesystem,&p->policy_identity,&p->policy_close,
   &p->root_lift.duplicate,&p->root_lift.close_original,&p->policy_lift.duplicate,&p->policy_lift.close_original,&status->open,&status->filesystem,&status->identity,&status->mapping,&status->lift.duplicate,&status->lift.close_original};
  for(unsigned n=0;n<18;n++){
   if(!calls[n]->source)continue;
   if(!atomic_load_explicit(&calls[n]->entered,memory_order_acquire)||!atomic_load_explicit(&calls[n]->returned,memory_order_acquire))return -EINVAL;
   if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[n],UINT64_MAX-7,index,n);
   if(result)return result;
  }
  const unsigned char *bodies[9]={(const unsigned char*)&p->root_fs,(const unsigned char*)&p->policy_fs,(const unsigned char*)&p->root_state,(const unsigned char*)&p->policy_state,(const unsigned char*)&p->root_how,(const unsigned char*)&p->policy_how,(const unsigned char*)&status->fs,(const unsigned char*)&status->state,(const unsigned char*)&status->how};
  unsigned long sizes[9]={sizeof(p->root_fs),sizeof(p->policy_fs),sizeof(p->root_state),sizeof(p->policy_state),sizeof(p->root_how),sizeof(p->policy_how),sizeof(status->fs),sizeof(status->state),sizeof(status->how)};
  sl_linux_call *owners[9]={&p->root_filesystem,&p->policy_filesystem,&p->root_identity,&p->policy_identity,&p->root_open,&p->policy_open,&status->filesystem,&status->identity,&status->open};
  for(unsigned n=0;n<9;n++){
   if(!owners[n]->source)continue;
   if(!emit)result=installation_budget(r,count,extent,sizes[n]);else result=final_add(r,p,bodies[n],sizes[n],8,UINT64_MAX-7,index,n);
   if(result)return result;
  }
  if(!emit)result=installation_budget(r,count,extent,p->path_length);else result=final_add(r,p,(const unsigned char*)p->actual_path,p->path_length,9,UINT64_MAX-7,index,(long)p->path_length);
  if(result)return result;
  unsigned long progress=0;
  for(unsigned long n=0;n<p->frame_count;n++){
   sl_linux_policy_frame *f=&p->actual_frames[n];
   if(!f->attempted||!f->returned||f->length>f->actual_capacity||!f->read.source||!atomic_load_explicit(&f->read.entered,memory_order_acquire)||!atomic_load_explicit(&f->read.returned,memory_order_acquire)||
      f->read.number!=SYS_read||f->read.arguments[0]!=p->policy_fd||f->read.arguments[1]!=(long)f->actual_bytes||f->read.arguments[2]!=(long)f->actual_capacity)return -EINVAL;
   if(f->read.result>=0&&(unsigned long)f->read.result<=f->actual_capacity&&f->length!=(unsigned long)f->read.result)return -EINVAL;
   if(!emit){result=final_source_span(r,f->actual_bytes,f->length);if(result)return result;}
   if(progress>p->length)return -EINVAL;
   unsigned long copied=f->length;if(copied>p->length-progress)copied=p->length-progress;
   if(!p->result&&copied!=f->length)return -EINVAL;
   for(unsigned long j=0;j<copied;j++)if(f->actual_bytes[j]!=p->actual_body[progress+j])return -EINVAL;
   progress+=copied;
   if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,&f->read,UINT64_MAX-7,index,18+n);
   if(result)return result;
   if(!emit)result=installation_budget(r,count,extent,f->length);else result=final_add(r,f,f->actual_bytes,f->length,12,index,n,f->read.result);
   if(result)return result;
  }
  if(progress!=p->length||(!p->result&&(!p->eof||!p->frame_count||p->actual_frames[p->frame_count-1].read.result!=0)))return -EINVAL;
 }
 return 0;
}
static long final_target(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_candidate_target *t=&r->prepared_controller->startup_original->candidate_target;long result;
 if(!t->attempted)return 0;if(!t->returned)return -EINVAL;
 if(!emit){result=final_source_span(r,(const unsigned char*)t,sizeof(*t));if(result)return result;}
 uint64_t states[4][10]={{t->attempted,t->returned,(uint64_t)t->primary_result,(uint64_t)t->status_result,(uint64_t)t->result,t->bound,(uint64_t)t->raw_target_fd,(uint64_t)t->target_fd,(uint64_t)t->raw_procs_fd,(uint64_t)t->procs_fd},
 {t->tuple[0],t->tuple[1],t->tuple[2],t->tuple[3],t->evidence_index,t->statement_offset,t->statement_extent,t->declarations,t->children_examined,t->views_examined},
 {t->target_lift.attempted,(uint64_t)t->target_lift.original_fd,(uint64_t)t->target_lift.held_fd,t->control_episodes,t->status_count,t->actual_status_capacity,0,0,0,0},
 {t->procs_lift.attempted,(uint64_t)t->procs_lift.original_fd,(uint64_t)t->procs_lift.held_fd,t->closes_attempted,t->closes_returned,(uint64_t)t->close_result,0,0,0,0}};
 for(unsigned n=0;n<4;n++){
  if(!emit)result=installation_budget(r,count,extent,80);
  else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned k=0;k<10;k++)final_u64(body+8*k,states[n][k]);result=final_add(r,n<2?(void*)t:n==2?(void*)&t->target_lift:(void*)&t->procs_lift,body,80,7,UINT64_MAX-6,n,0);}
  if(result)return result;
 }
 sl_linux_call *calls[10]={&t->open_target,&t->open_procs,&t->target_filesystem,&t->procs_filesystem,&t->target_identity,&t->procs_identity,&t->target_lift.duplicate,&t->target_lift.close_original,&t->procs_lift.duplicate,&t->procs_lift.close_original};
 for(unsigned n=0;n<10;n++){
  if(!calls[n]->source)continue;
  if(!atomic_load_explicit(&calls[n]->entered,memory_order_acquire)||!atomic_load_explicit(&calls[n]->returned,memory_order_acquire))return -EINVAL;
  if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[n],UINT64_MAX-6,n,0);
  if(result)return result;
 }
 const unsigned char *bodies[6]={(const unsigned char*)&t->target_fs,(const unsigned char*)&t->procs_fs,(const unsigned char*)&t->target_state,(const unsigned char*)&t->procs_state,(const unsigned char*)&t->target_how,(const unsigned char*)&t->procs_how};
 unsigned long sizes[6]={sizeof(t->target_fs),sizeof(t->procs_fs),sizeof(t->target_state),sizeof(t->procs_state),sizeof(t->target_how),sizeof(t->procs_how)};
 sl_linux_call *owners[6]={&t->target_filesystem,&t->procs_filesystem,&t->target_identity,&t->procs_identity,&t->open_target,&t->open_procs};
 for(unsigned n=0;n<6;n++){
  if(!owners[n]->source)continue;
  if(!emit)result=installation_budget(r,count,extent,sizes[n]);else result=final_add(r,t,bodies[n],sizes[n],8,UINT64_MAX-6,n,owners[n]->result);
  if(result)return result;
 }
 return 0;
}
static long final_mounts(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_local_mounts *m=&r->prepared_controller->startup_original->mounts;long result;
 if(!m->attempted)return 0;
 if(!m->returned||m->length>m->actual_capacity||m->read_count>m->actual_read_capacity||
    m->read_count>ULONG_MAX/sizeof(*m->actual_reads)||(!m->actual_reads&&m->read_count))return -EINVAL;
 if(m->eof&&(!m->read_count||m->actual_reads[m->read_count-1].result!=0))return -EINVAL;
 if(!emit){
  result=final_source_span(r,(const unsigned char*)m,sizeof(*m));if(result)return result;
  result=final_source_span(r,(const unsigned char*)m->actual_reads,m->read_count*sizeof(*m->actual_reads));if(result)return result;
  result=final_source_span(r,m->actual_bytes,m->length);if(result)return result;
 }
 uint64_t state[2][10]={{m->attempted,m->returned,(uint64_t)m->result,(uint64_t)m->raw_fd,(uint64_t)m->fd,m->eof,m->length,m->read_count,m->row_count,m->selinux_count},
  {m->lift.attempted,(uint64_t)m->lift.original_fd,(uint64_t)m->lift.held_fd,m->actual_capacity,m->actual_read_capacity,m->actual_row_capacity,m->actual_selinux_capacity,0,0,0}};
 for(unsigned n=0;n<2;n++){
  if(!emit)result=installation_budget(r,count,extent,80);
  else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned k=0;k<10;k++)final_u64(body+8*k,state[n][k]);result=final_add(r,n?(void*)&m->lift:(void*)m,body,80,7,UINT64_MAX-5,n,0);}
  if(result)return result;
 }
 sl_linux_call *calls[5]={&m->open,&m->filesystem_query,&m->close,&m->lift.duplicate,&m->lift.close_original};
 for(unsigned n=0;n<5;n++){
  /* Only a genuinely entered cell has a published source. Failed validation
   * before native initialization must not load an uninitialized atomic. */
  if(!calls[n]->source)continue;
  if(!atomic_load_explicit(&calls[n]->entered,memory_order_acquire)||!atomic_load_explicit(&calls[n]->returned,memory_order_acquire))return -EINVAL;
  if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[n],UINT64_MAX-5,n,0);
  if(result)return result;
 }
 unsigned long progress=0;
 for(unsigned long n=0;n<m->read_count;n++){
  sl_linux_call *call=&m->actual_reads[n];
  if(!atomic_load_explicit(&call->entered,memory_order_acquire)||!atomic_load_explicit(&call->returned,memory_order_acquire)||call->number!=SYS_read||
     call->arguments[0]!=m->fd||progress>m->actual_capacity||call->arguments[1]!=(long)(m->actual_bytes+progress)||call->arguments[2]!=(long)(m->actual_capacity-progress))return -EINVAL;
  if(call->result>0){if((unsigned long)call->result>m->actual_capacity-progress)return -EINVAL;progress+=(unsigned long)call->result;}
  else if(n+1!=m->read_count)return -EINVAL;
  if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,call,UINT64_MAX-5,5+n,0);
  if(result)return result;
 }
 if(progress!=m->length)return -EINVAL;
 if(m->filesystem_query.source){
  if(!emit)result=installation_budget(r,count,extent,sizeof(m->filesystem));else result=final_add(r,m,(const unsigned char*)&m->filesystem,sizeof(m->filesystem),8,UINT64_MAX-5,0,0);
  if(result)return result;
 }
 if(!emit)result=installation_budget(r,count,extent,m->length);else result=final_add(r,m,m->actual_bytes,m->length,11,UINT64_MAX-5,0,(long)m->length);
 return result;
}
static long final_membership(sl_linux_final_capture *r,unsigned emit,unsigned long *count,unsigned long *extent){
 sl_linux_local_membership *m=&r->prepared_controller->startup_original->membership;long result;
 if(!m->attempted)return 0;
 if(!m->returned||m->count>m->prepared_capacity||m->count>ULONG_MAX/sizeof(*m->prepared_frames)||(!m->prepared_frames&&m->count)||m->path_length>m->prepared_path_capacity)return -EINVAL;
 if(!emit){
  result=final_source_span(r,(const unsigned char*)m,sizeof(*m));if(result)return result;
  result=final_source_span(r,(const unsigned char*)m->prepared_frames,m->count*sizeof(*m->prepared_frames));if(result)return result;
  result=final_source_span(r,(const unsigned char*)m->prepared_path,m->path_length);if(result)return result;
 }
 uint64_t states[2][10]={{m->attempted,m->returned,(uint64_t)m->result,(uint64_t)m->fd,m->eof,m->count,m->prepared_capacity,m->path_length,m->prepared_path_capacity,0},
 {m->lift.attempted,(uint64_t)m->lift.original_fd,(uint64_t)m->lift.held_fd,0,0,0,0,0,0,0}};
 for(unsigned n=0;n<2;n++){
  if(!emit)result=installation_budget(r,count,extent,80);
  else{unsigned char *body=r->actual_records[r->count].observation;for(unsigned k=0;k<10;k++)final_u64(body+8*k,states[n][k]);result=final_add(r,n?(void*)&m->lift:(void*)m,body,80,7,UINT64_MAX-4,n,0);}
  if(result)return result;
 }
 sl_linux_call *calls[4]={&m->open,&m->filesystem_query,&m->lift.duplicate,&m->lift.close_original};
 unsigned native_count=m->prepared_frames?2:0;
 if(m->lift.attempted&&m->lift.original_fd>=0&&m->lift.original_fd<3)native_count=4;
 for(unsigned n=0;n<native_count;n++){
  if(!emit)result=installation_budget(r,count,extent,80);else result=final_native_call(r,calls[n],UINT64_MAX-4,n,0);
  if(result)return result;
 }
 if(m->prepared_frames){
  if(!emit)result=installation_budget(r,count,extent,sizeof(m->filesystem));else result=final_add(r,m,(const unsigned char*)&m->filesystem,sizeof(m->filesystem),8,UINT64_MAX-4,0,0);
  if(result)return result;
 }
 if(m->prepared_path){
  if(!emit)result=installation_budget(r,count,extent,m->path_length);else result=final_add(r,m,(const unsigned char*)m->prepared_path,m->path_length,9,UINT64_MAX-4,0,(long)m->path_length);
  if(result)return result;
 }else if(m->path_length)return -EINVAL;
 for(unsigned long n=0;n<m->count;n++){
  sl_linux_membership_frame *frame=&m->prepared_frames[n];
  if(!atomic_load_explicit(&frame->read.entered,memory_order_acquire)||!atomic_load_explicit(&frame->read.returned,memory_order_acquire)||frame->raw_count!=frame->read.result||
     frame->read.number!=SYS_read||frame->read.arguments[0]!=m->fd||frame->read.arguments[1]!=(long)frame->bytes||frame->read.arguments[2]!=(long)frame->capacity||
     (frame->raw_count>0&&(unsigned long)frame->raw_count>frame->capacity))return -EINVAL;
  unsigned long bytes=frame->raw_count>0?(unsigned long)frame->raw_count:0;
  if(!emit){result=final_source_span(r,frame->bytes,bytes);if(result)return result;result=installation_budget(r,count,extent,80);}
  else result=final_native_call(r,&frame->read,UINT64_MAX-4,n,4);
  if(result)return result;
  if(!emit)result=installation_budget(r,count,extent,bytes);else result=final_add(r,frame,frame->bytes,bytes,10,UINT64_MAX-4,n,frame->raw_count);
  if(result)return result;
 }
 return 0;
}
long sl_linux_persist_final_capture(sl_linux_final_capture *r){
 long result=-EINVAL;
 if(!r||!r->prepared||r->attempted)return -EINVAL;r->attempted=1;
 sl_linux_public_controller *c=r->prepared_controller;sl_linux_source_cut *cut=c->drained_by;r->cut_original=cut;
 if(!cut||!cut->returned||cut->result||cut->controller_original!=c||!cut->service_original||cut!=&cut->service_original->source_cut||!c->drain_returned||c->drain_result||
    cut->checked_readers!=cut->reader_count||cut->checked_requests!=cut->request_capacity)goto finished;
 sl_linux_local_collection *local=&c->collection;
 if(!local->returned||local->result!=8||!local->readback_equal||local->body_length>local->prepared_capacity||local->prepared_storage_original==r->file_original||local->prepared_storage==r->file_fd||local->prepared_directory_original!=r->directory_original||local->prepared_directory!=r->directory_fd)goto finished;
 /* Complete source preflight precedes even record0/header mutation. A later
  * raw source may alias an earlier record's header. Never corrupt it merely
  * to discover the overlap when that later record would be constructed. */
 result=final_source_span(r,local->prepared_body,local->body_length);if(result)goto finished;
 result=final_source_span(r,(const unsigned char*)r->candidate_original,sizeof(*r->candidate_original));if(result)goto finished;
 result=final_source_span(r,r->candidate_body.bytes,r->candidate_body.extent);if(result)goto finished;
 result=final_source_span(r,(const unsigned char*)cut,sizeof(*cut));if(result)goto finished;
 sl_linux_root_observation *root=cut->root_original;
 sl_linux_family_join *family_join=cut->family_original;
 if(!root||!family_join||!family_join->worker_original){result=-EINVAL;goto finished;}
 result=final_source_span(r,(const unsigned char*)root,sizeof(*root));if(result)goto finished;
 result=final_source_span(r,(const unsigned char*)family_join,sizeof(*family_join));if(result)goto finished;
 sl_linux_family_worker *family=family_join->worker_original;
 result=final_source_span(r,(const unsigned char*)family,sizeof(*family));if(result)goto finished;
 if(family->observation.read_count>22){result=-EINVAL;goto finished;}
 if(!cut->readers_original||cut->reader_count>ULONG_MAX/sizeof(*cut->readers_original)){result=-EINVAL;goto finished;}
 result=final_source_span(r,(const unsigned char*)cut->readers_original,cut->reader_count*sizeof(*cut->readers_original));if(result)goto finished;
 unsigned long required_count=5,required_extent=32+112+32+sizeof(root->information)+32+sizeof(root->usage);
 if(required_extent>r->actual_bytes||32>r->actual_bytes-required_extent||r->candidate_body.extent>r->actual_bytes-required_extent-32){result=-ENOSPC;goto finished;}required_extent+=32+r->candidate_body.extent;
 if(local->body_length>r->actual_bytes||required_extent>r->actual_bytes-local->body_length){result=-ENOSPC;goto finished;}required_extent+=local->body_length;
 for(unsigned long i=0;i<family->observation.read_count;i++){
  sl_linux_family_read *read=&family->observation.reads[i];
  if(!read->returned||read->count<0||read->count>1){result=-EINVAL;goto finished;}
  unsigned long size=144+(unsigned long)read->count;
  if(required_extent>r->actual_bytes||size>r->actual_bytes-required_extent){result=-ENOSPC;goto finished;}
  required_extent+=size;required_count+=2;
 }
 for(unsigned long stream=0;stream<cut->reader_count;stream++){
  sl_linux_capture_worker *reader=&cut->readers_original[stream];
  if(reader->read_count>reader->prepared_capacity||reader->read_count>ULONG_MAX/sizeof(*reader->prepared_frames)||(!reader->prepared_frames&&reader->read_count)){result=-EINVAL;goto finished;}
  result=final_source_span(r,(const unsigned char*)reader->prepared_frames,reader->read_count*sizeof(*reader->prepared_frames));if(result)goto finished;
  for(unsigned long frame=0;frame<reader->read_count;frame++){
   sl_linux_capture_frame *original=&reader->prepared_frames[frame];long raw=original->raw_count;
   if(!original->read_returned||raw>(long)sizeof(original->bytes)||(raw<0&&raw!=-EAGAIN)){result=-EINVAL;goto finished;}
   result=final_source_span(r,original->bytes,raw>0?(unsigned long)raw:0);if(result)goto finished;
   unsigned long length=raw>0?(unsigned long)raw:0;
   if(original->public_write_count>original->public_write_capacity||original->public_write_count>ULONG_MAX/sizeof(sl_linux_call)||(!original->public_writes&&original->public_write_count)){result=-EINVAL;goto finished;}
   result=final_source_span(r,(const unsigned char*)original->public_writes,original->public_write_count*sizeof(sl_linux_call));if(result)goto finished;
   if(original->public_write_count>ULONG_MAX-6||required_count>ULONG_MAX-6-original->public_write_count){result=-ENOSPC;goto finished;}
   unsigned long calls=5+original->public_write_count;
   if(calls>ULONG_MAX/112||required_extent>r->actual_bytes||32>r->actual_bytes-required_extent||length>r->actual_bytes-required_extent-32){result=-ENOSPC;goto finished;}
   required_count+=6+original->public_write_count;required_extent+=32+length;
   if(calls*112>r->actual_bytes-required_extent){result=-ENOSPC;goto finished;}required_extent+=calls*112;
  }
 }
 result=final_services(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_service_attempts(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_parsers(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_parser_diagnostics(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_contexts(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_policy_traversals(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_collector(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_schedule(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_forward(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_request_input(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_channel(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_terminal(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_namespace(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_fd_scan(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_bootstrap(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_entry(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_gate(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_installation(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_membership(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_mounts(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_target(r,0,&required_count,&required_extent);if(result)goto finished;
 result=final_policies(r,0,&required_count,&required_extent);if(result)goto finished;
 if(required_count>r->actual_capacity){result=-ENOSPC;goto finished;}
 result=final_add(r,local,local->prepared_body,local->body_length,1,UINT64_MAX,0,(long)local->body_length);if(result)goto finished;
 result=final_native_call(r,&root->wait,UINT64_MAX-1,0,0);if(result)goto finished;
 result=final_add(r,root,(const unsigned char*)&root->information,sizeof(root->information),4,UINT64_MAX-1,0,0);if(result)goto finished;
 result=final_add(r,root,(const unsigned char*)&root->usage,sizeof(root->usage),4,UINT64_MAX-1,1,0);if(result)goto finished;
 for(unsigned long i=0;i<family->observation.read_count;i++){
  sl_linux_family_read *read=&family->observation.reads[i];
  result=final_native_call(r,&read->call,UINT64_MAX-2,i,0);if(result)goto finished;
  result=final_add(r,read,&read->byte,(unsigned long)read->count,5,UINT64_MAX-2,i,read->count);if(result)goto finished;
 }
 for(unsigned long stream=0;stream<cut->reader_count;stream++){
  sl_linux_capture_worker *reader=&cut->readers_original[stream];
  if(reader->read_count>reader->prepared_capacity){result=-EINVAL;goto finished;}
  for(unsigned long frame=0;frame<reader->read_count;frame++){
   sl_linux_capture_frame *original=&reader->prepared_frames[frame];long raw=original->raw_count;
   if(!original->read_returned||raw>(long)sizeof(original->bytes)||(raw<0&&raw!=-EAGAIN)){result=-EINVAL;goto finished;}
   result=final_add(r,original,original->bytes,raw>0?(unsigned long)raw:0,2,stream,frame,raw);if(result)goto finished;
   sl_linux_call *calls[5]={&original->ready,&original->read,&original->public_thread,&original->public_pending_before,&original->public_pending_after};
   for(unsigned i=0;i<5;i++){result=final_native_call(r,calls[i],stream,frame,i);if(result)goto finished;}
   for(unsigned long i=0;i<original->public_write_count;i++){result=final_native_call(r,&original->public_writes[i],stream,frame,5+i);if(result)goto finished;}
  }
 }
 result=final_add(r,r->candidate_original,r->candidate_body.bytes,r->candidate_body.extent,6,UINT64_MAX,0,(long)r->candidate_body.extent);if(result)goto finished;
 result=final_services(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_service_attempts(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_parsers(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_parser_diagnostics(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_contexts(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_policy_traversals(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_collector(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_schedule(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_forward(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_request_input(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_channel(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_terminal(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_namespace(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_fd_scan(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_bootstrap(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_entry(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_gate(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_installation(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_membership(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_mounts(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_target(r,1,&required_count,&required_extent);if(result)goto finished;
 result=final_policies(r,1,&required_count,&required_extent);if(result)goto finished;
 /* All actual record extents are retained before the first file effect. */
 for(unsigned long i=0;i<r->count;i++){
  sl_linux_final_record *record=&r->actual_records[i];
  result=final_exact(r,&r->actual_writes,record->header,32,record->offset,0);if(result)goto finished;
  result=final_exact(r,&r->actual_writes,record->body,record->length,record->offset+32,0);if(result)goto finished;
 }
 result=sl_linux_call6(&r->flush,"archive.final.capture.flush",SYS_fsync,r->file_fd,0,0,0,0,0);if(result)goto finished;
 result=sl_linux_call6(&r->directory_flush,"archive.final.capture.directory-flush",SYS_fsync,r->directory_fd,0,0,0,0,0);if(result)goto finished;
 result=final_exact(r,&r->actual_reads,r->actual_readback,r->extent,0,1);if(result)goto finished;
 result=sl_linux_call6(&r->eof,"archive.final.capture.readback-eof",SYS_pread64,r->file_fd,(long)&r->eof_byte,1,r->extent,0,0);if(result){if(result>0)result=-EIO;goto finished;}
 for(unsigned long i=0;i<r->count;i++){
  sl_linux_final_record *record=&r->actual_records[i];
  for(unsigned long j=0;j<32;j++)if(record->header[j]!=r->actual_readback[record->offset+j]){result=-EIO;goto finished;}
  for(unsigned long j=0;j<record->length;j++)if(record->body[j]!=r->actual_readback[record->offset+32+j]){result=-EIO;goto finished;}
 }
 r->readback_equal=1;
finished:
 r->result=result;r->returned=1;return result;
 /* No seal/independent ACK: this is not the complete native-attempt prefix.
  * Partial files and every original remain owned; never unlink or retry. */
}
#endif
