#ifdef _WIN32
#include "candidate_deadline_windows.h"
#include "debug_root_windows.h"
#include "argument_root_windows.h"
#include "argument_reader_cancel_windows.h"
#include <stdint.h>
static int away(const void *a,SIZE_T n,const void *b,SIZE_T m){ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;return x&&y&&n&&m&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&!(x<y+m&&y<x+n);}
static int fresh(const void *p,SIZE_T n){const unsigned char *b=p;for(SIZE_T i=0;i<n;i++)if(b[i])return 0;return 1;}
static int owner_layout(unsigned k,unsigned *role,SIZE_T *unit,SIZE_T *alignment){
 #define OWNER(kind,item,type) case kind:*role=item;*unit=sizeof(type);*alignment=_Alignof(type);return 1
 switch(k){
 OWNER(SL_WIN_DEADLINE_STARTUP,SL_WIN_LAYOUT_STARTUP,sl_win_public_startup);
 OWNER(SL_WIN_DEADLINE_DEBUG_EVENT,SL_WIN_LAYOUT_DEBUG_EVENTS,sl_win_debug_event_observation);
 OWNER(SL_WIN_DEADLINE_MAPPED_ROOT,SL_WIN_LAYOUT_MAPPED_ROOTS,sl_win_mapped_root_observation);
 OWNER(SL_WIN_DEADLINE_ROOT_WRITE,SL_WIN_LAYOUT_ROOT_WRITES,sl_win_root_write);
 OWNER(SL_WIN_DEADLINE_DEBUG_RELEASE,SL_WIN_LAYOUT_DEBUG_RELEASES,sl_win_debug_release);
 OWNER(SL_WIN_DEADLINE_READER_CANCEL,SL_WIN_LAYOUT_READER_CANCELS,sl_win_argument_reader_cancel);
 OWNER(SL_WIN_DEADLINE_DUPLEX_COMPLETION,SL_WIN_LAYOUT_CONNECTIONS,sl_win_duplex_completion);
 OWNER(SL_WIN_DEADLINE_DUPLEX_SETTLEMENT,SL_WIN_LAYOUT_SETTLEMENTS,sl_win_duplex_settlement);
 OWNER(SL_WIN_DEADLINE_MODULE_QUERY,SL_WIN_LAYOUT_MODULE_QUERIES,sl_win_module_query);
 OWNER(SL_WIN_DEADLINE_SERVICE,SL_WIN_LAYOUT_SERVICES,sl_win_service_observation);
 OWNER(SL_WIN_DEADLINE_SEND_TURN,SL_WIN_LAYOUT_TURNS,sl_win_argument_send_turn);
 default:return 0;
 }
 #undef OWNER
}
static const sl_win_call *member(const sl_win_candidate_deadline_row *r){
 #define ONE(id,path) case id:return r->slot?NULL:&o->path
 #define ARRAY(id,path,n,field) case id:return r->slot<(n)?&o->path[r->slot].field:NULL
 #define ELEMENT(id,path,n) case id:return r->slot<(n)?&o->path[r->slot]:NULL
 switch(r->kind){
 case SL_WIN_DEADLINE_STARTUP:{const sl_win_public_startup *o=r->owner_original;switch(r->member){
 ONE(SL_WIN_DCALL_ENTRY_PROCESS,controller.entry.current_process_query);
 ARRAY(SL_WIN_DCALL_STREAM_GET,controller.entry.public_streams,3,get_handle);
 ARRAY(SL_WIN_DCALL_STREAM_DUPLICATE,controller.entry.public_streams,3,duplicate);
 ONE(SL_WIN_DCALL_CHANNEL_PROCESS,channels.process_query);ELEMENT(SL_WIN_DCALL_CHANNEL_CREATE,channels.create,3);
 ONE(SL_WIN_DCALL_CHANNEL_GATE,channels.gate_duplicate);
 ONE(SL_WIN_DCALL_DUPLEX_PROCESS,duplex.process_query);ONE(SL_WIN_DCALL_DUPLEX_TOKEN,duplex.token_open);
 ONE(SL_WIN_DCALL_DUPLEX_USER,duplex.user_query);ONE(SL_WIN_DCALL_DUPLEX_SID_VALID,duplex.sid_valid);
 ONE(SL_WIN_DCALL_DUPLEX_SID_LENGTH,duplex.sid_length);ONE(SL_WIN_DCALL_DUPLEX_ACL_INIT,duplex.acl_initialize);
 ONE(SL_WIN_DCALL_DUPLEX_ACL_ADD,duplex.acl_add);ONE(SL_WIN_DCALL_DUPLEX_DESCRIPTOR_INIT,duplex.descriptor_initialize);
 ONE(SL_WIN_DCALL_DUPLEX_DESCRIPTOR_DACL,duplex.descriptor_dacl);ONE(SL_WIN_DCALL_DUPLEX_DESCRIPTOR_CONTROL,duplex.descriptor_control);
 ONE(SL_WIN_DCALL_DUPLEX_PID,duplex.pid_query);ONE(SL_WIN_DCALL_DUPLEX_TICK,duplex.tick_query);
 ONE(SL_WIN_DCALL_DUPLEX_SERVER,duplex.server_create);ONE(SL_WIN_DCALL_DUPLEX_EVENT,duplex.event_create);
 ONE(SL_WIN_DCALL_DUPLEX_CONNECT,duplex.connect_begin);ONE(SL_WIN_DCALL_DUPLEX_CLIENT,duplex.client_open);
 ONE(SL_WIN_DCALL_DUPLEX_DUPLICATE,duplex.client_duplicate);ONE(SL_WIN_DCALL_DUPLEX_CANCEL,duplex.cancel_request);
 ONE(SL_WIN_DCALL_ARGUMENT_COMMAND,arguments.command_line_query);ONE(SL_WIN_DCALL_ENDPOINT_SERVER,endpoint.server_process_query);
 ONE(SL_WIN_DCALL_ENDPOINT_INFO,endpoint.information_query);ONE(SL_WIN_DCALL_ENDPOINT_SECURITY,endpoint.security.query);
 ONE(SL_WIN_DCALL_BIRTH_OWNER,controller.entry.birth.owner_process_query);
 ARRAY(SL_WIN_DCALL_BIRTH_SECURITY,controller.entry.birth.security,6,query);
 ARRAY(SL_WIN_DCALL_BIRTH_TOKEN_OPEN,controller.entry.birth.tokens,2,open);
 ARRAY(SL_WIN_DCALL_BIRTH_TOKEN_BEFORE,controller.entry.birth.tokens,2,before_query);
 ARRAY(SL_WIN_DCALL_BIRTH_TOKEN_INFO,controller.entry.birth.tokens,2,information_query);
 ARRAY(SL_WIN_DCALL_BIRTH_TOKEN_AFTER,controller.entry.birth.tokens,2,after_query);
 ONE(SL_WIN_DCALL_BIRTH_JOB,controller.entry.birth.create_job);ONE(SL_WIN_DCALL_BIRTH_LIMITS,controller.entry.birth.set_limits);
 ONE(SL_WIN_DCALL_BIRTH_ATTRIBUTE_QUERY,controller.entry.birth.attribute_query);
 ONE(SL_WIN_DCALL_BIRTH_ATTRIBUTE_INIT,controller.entry.birth.attribute_initialize);
 ONE(SL_WIN_DCALL_BIRTH_ATTRIBUTE_JOB,controller.entry.birth.attribute_job);
 ONE(SL_WIN_DCALL_BIRTH_ATTRIBUTE_HANDLES,controller.entry.birth.attribute_handles);
 ONE(SL_WIN_DCALL_BIRTH_CREATOR_QUERY,controller.entry.birth.creator_thread_query);
 ONE(SL_WIN_DCALL_BIRTH_CREATOR_PSEUDO,controller.entry.birth.creator_thread_pseudo_query);
 ONE(SL_WIN_DCALL_BIRTH_CREATOR_DUPLICATE,controller.entry.birth.creator_thread_duplicate);
 ONE(SL_WIN_DCALL_BIRTH_PROCESS,controller.entry.birth.create_process);
 ARRAY(SL_WIN_DCALL_WRITE_THREAD,argument_sequence.writes,4,thread_query);
 ARRAY(SL_WIN_DCALL_WRITE_EVENT,argument_sequence.writes,4,event_create);
 ARRAY(SL_WIN_DCALL_WRITE,argument_sequence.writes,4,write);
 ARRAY(SL_WIN_DCALL_WRITE_WORKER_QUERY,argument_sequence.writes,4,worker_query);
 ARRAY(SL_WIN_DCALL_WRITE_SUSPEND,argument_sequence.writes,4,suspension);
 ARRAY(SL_WIN_DCALL_WRITE_CANCEL_CHECK,argument_sequence.writes,4,cancellation_check);
 ARRAY(SL_WIN_DCALL_WRITE_CANCEL,argument_sequence.writes,4,worker_cancel);
 ARRAY(SL_WIN_DCALL_WORKER_CREATE,argument_sequence.workers,4,create);
 ARRAY(SL_WIN_DCALL_WORKER_RESUME,argument_sequence.workers,4,resume);
 default:return NULL;}}
 case SL_WIN_DEADLINE_DEBUG_EVENT:{const sl_win_debug_event_observation *o=r->owner_original;switch(r->member){
 ONE(SL_WIN_DCALL_THREAD_QUERY,thread_query);ONE(SL_WIN_DCALL_HELD_THREAD_QUERY,held_thread_query);
 ONE(SL_WIN_DCALL_HELD_THREAD_WAIT,held_thread_wait);ONE(SL_WIN_DCALL_WAIT,wait);default:return NULL;}}
 case SL_WIN_DEADLINE_MAPPED_ROOT:{const sl_win_mapped_root_observation *o=r->owner_original;switch(r->member){
 ONE(SL_WIN_DCALL_REGION,region_query);ONE(SL_WIN_DCALL_READ,read);default:return NULL;}}
 case SL_WIN_DEADLINE_ROOT_WRITE:{const sl_win_root_write *o=r->owner_original;switch(r->member){
 ONE(SL_WIN_DCALL_ROOT_WRITE,write);ONE(SL_WIN_DCALL_READBACK_REGION,readback.region_query);ONE(SL_WIN_DCALL_READBACK_READ,readback.read);default:return NULL;}}
 case SL_WIN_DEADLINE_DEBUG_RELEASE:{const sl_win_debug_release *o=r->owner_original;switch(r->member){
 ONE(SL_WIN_DCALL_CURRENT_THREAD,current_thread_query);ONE(SL_WIN_DCALL_HELD_THREAD_QUERY,held_thread_query);
 ONE(SL_WIN_DCALL_HELD_THREAD_WAIT,held_thread_wait);ONE(SL_WIN_DCALL_CONTINUE,continue_event);
 ONE(SL_WIN_DCALL_STOP,stop_debug);ONE(SL_WIN_DCALL_RESUME_PRIMARY,resume_primary);default:return NULL;}}
 case SL_WIN_DEADLINE_READER_CANCEL:{const sl_win_argument_reader_cancel *o=r->owner_original;switch(r->member){
 ONE(SL_WIN_DCALL_DISCONNECT,disconnect);ONE(SL_WIN_DCALL_THREAD_QUERY,thread_query);ONE(SL_WIN_DCALL_WAIT,thread_wait);
 ONE(SL_WIN_DCALL_CANCEL,cancel);ONE(SL_WIN_DCALL_WRITER_PUBLICATION,writer_cancellation.cancellation.publication);
 ONE(SL_WIN_DCALL_WRITER_REQUEST,writer_cancellation.cancellation.request);default:return NULL;}}
 case SL_WIN_DEADLINE_DUPLEX_COMPLETION:{const sl_win_duplex_completion *o=r->owner_original;switch(r->member){ONE(SL_WIN_DCALL_QUERY,query);default:return NULL;}}
 case SL_WIN_DEADLINE_DUPLEX_SETTLEMENT:{const sl_win_duplex_settlement *o=r->owner_original;switch(r->member){ONE(SL_WIN_DCALL_QUERY,query);default:return NULL;}}
 case SL_WIN_DEADLINE_MODULE_QUERY:{const sl_win_module_query *o=r->owner_original;switch(r->member){ONE(SL_WIN_DCALL_QUERY,query);default:return NULL;}}
 case SL_WIN_DEADLINE_SERVICE:{const sl_win_service_observation *o=r->owner_original;switch(r->member){
 ONE(SL_WIN_DCALL_WAIT,root.wait);ONE(SL_WIN_DCALL_EXIT,root.exit_code);ONE(SL_WIN_DCALL_QUERY,family.query);default:return NULL;}}
 case SL_WIN_DEADLINE_SEND_TURN:{const sl_win_argument_send_turn *o=r->owner_original;switch(r->member){
 ONE(SL_WIN_DCALL_JOIN_WAIT,join.wait);ONE(SL_WIN_DCALL_JOIN_EXIT,join.exit_query);ONE(SL_WIN_DCALL_QUERY,completion.query);default:return NULL;}}
 default:return NULL;
 }
 #undef ONE
 #undef ARRAY
 #undef ELEMENT
}
static int row(const sl_win_candidate_deadline_row *r,SIZE_T ordinal,const sl_win_candidate_preparation *p){
 unsigned role;SIZE_T unit,alignment;
 if(r->ordinal!=ordinal||!owner_layout(r->kind,&role,&unit,&alignment)||r->owner_extent!=unit)return 0;
 const sl_win_candidate_layout_item *v=&p->plan_original->copies_original[role];
 if(v->element_extent!=unit||v->alignment<alignment||v->offset%alignment||r->occurrence>=v->count)return 0;
 const void *actual=(unsigned char *)p->backing_original+v->offset+r->occurrence*unit;
 return r->owner_original==actual&&r->native_row_original&&r->native_row_original==member(r);
}
static int row_equal(const sl_win_candidate_deadline_row *a,const sl_win_candidate_deadline_row *b){
 return a->native_row_original==b->native_row_original&&a->owner_original==b->owner_original&&a->owner_extent==b->owner_extent&&
 a->kind==b->kind&&a->member==b->member&&a->occurrence==b->occurrence&&a->slot==b->slot&&a->ordinal==b->ordinal;
}
static int source(const sl_win_candidate_deadline_source *s,sl_win_candidate_deadline_step **steps){
 if(!s||!s->preparation_original||!s->caller_original||!s->rows_original||!s->row_count||s->row_count>SIZE_MAX/sizeof(*s->rows_original))return 0;
 const sl_win_candidate_preparation *p=s->preparation_original;
 if(!p->attempted||!p->returned||!p->result||!sl_win_candidate_layout_matches_original(p->plan_original)||
    !p->backing_original||p->backing_extent!=p->plan_original->bytes||p->backing_extent>UINTPTR_MAX-(ULONG_PTR)p->backing_original||
    (ULONG_PTR)p->backing_original%p->plan_original->alignment||
    p->startup_original!=(void *)((unsigned char *)p->backing_original+p->plan_original->copies_original[SL_WIN_LAYOUT_STARTUP].offset)||
    p->factory_original!=(void *)((unsigned char *)p->backing_original+p->plan_original->copies_original[SL_WIN_LAYOUT_FACTORY_CONTROL].offset))return 0;
 const sl_win_candidate_layout_item *v=&p->plan_original->copies_original[SL_WIN_LAYOUT_DEADLINE_CUSTODY];
 if(v->count!=s->row_count||v->element_extent!=sizeof(**steps)||v->alignment<_Alignof(sl_win_candidate_deadline_step)||
    v->offset%_Alignof(sl_win_candidate_deadline_step))return 0;
 *steps=(void *)((unsigned char *)p->backing_original+v->offset);
 return (ULONG_PTR)*steps%_Alignof(sl_win_candidate_deadline_step)==0;
}
static int caller(const struct slcli_product_windows_operation_caller *c){
 return c&&c->initial_source_original&&c->arena_original&&c->memory_original&&c->arena_original->owner==c->memory_original&&
 c->deadline_original&&slcli_source_deadline_original_matches(c->arena_original,c->deadline_original,c)&&
 slcli_native_clock_windows_matches_original(c->deadline_original->origin);
}
static int live(const sl_win_candidate_deadline_source *s){
 struct slcli_arena *a=s->caller_original->arena_original;
 const sl_win_candidate_preparation *p=s->preparation_original;
 if(!slcli_arena_live_block(a,p->backing_original,p->backing_extent))return 0;
 for(SIZE_T i=0;i<s->row_count;i++)if(!slcli_arena_live_range(a,s->rows_original[i].owner_original,s->rows_original[i].owner_extent))return 0;
 return 1;
}
static int excludes(const sl_win_candidate_deadline_source *s,const void *out,SIZE_T n){
 const struct slcli_product_windows_operation_caller *c=s->caller_original;
 if(!away(out,n,s,sizeof(*s))||!away(out,n,s->rows_original,s->row_count*sizeof(*s->rows_original))||
    !away(out,n,s->preparation_original,sizeof(*s->preparation_original))||
    !sl_win_candidate_layout_excludes_output(s->preparation_original->plan_original,out,n)||
    !away(out,n,c,sizeof(*c))||!slcli_product_windows_initial_source_excludes_output(c->initial_source_original,out,n)||
    !away(out,n,c->arena_original,sizeof(*c->arena_original))||!away(out,n,c->memory_original,sizeof(*c->memory_original))||
    !slcli_source_deadline_away(c->deadline_original,out,n))return 0;
 for(SIZE_T i=0;i<s->row_count;i++){
  if(!row(&s->rows_original[i],i,s->preparation_original)||!away(out,n,s->rows_original[i].owner_original,s->rows_original[i].owner_extent))return 0;
 }return 1;
}
int sl_win_bind_prepared_candidate_deadline(sl_win_candidate_deadline_sequence *q){
 sl_win_candidate_deadline_step *steps;
 if(!q||q->attempted||q->returned||!q->source_original||!source(q->source_original,&steps)||!caller(q->source_original->caller_original)||!live(q->source_original))return 0;
 const sl_win_candidate_deadline_source *s=q->source_original;const struct slcli_product_windows_operation_caller *c=s->caller_original;
 if(s->row_count>SIZE_MAX/sizeof(*steps))return 0;
 SIZE_T n=s->row_count*sizeof(*steps);
 if(!excludes(s,q,sizeof(*q))||!excludes(s,steps,n)||!away(q,sizeof(*q),steps,n)||
    !away(q,sizeof(*q),s->preparation_original->backing_original,s->preparation_original->backing_extent)||
    !slcli_arena_live_range(c->arena_original,q,sizeof(*q))||
    !slcli_arena_live_range(c->arena_original,c,sizeof(*c))||!slcli_arena_live_range(c->arena_original,steps,n)||
    !fresh(steps,n)||q->actual_source||!fresh(&q->source_snapshot,sizeof(q->source_snapshot))||q->arena_original||q->memory_original||q->initial_source_original||
    q->deadline_original||q->origin_original||q->origin_tick||q->expires||q->steps_original||q->capacity||q->result||
    !fresh(q->origin_raw,sizeof(q->origin_raw))||!fresh(q->origin_status,sizeof(q->origin_status))||!fresh(q->origin_args,sizeof(q->origin_args)))return 0;
 for(SIZE_T i=0;i<s->row_count;i++){
  if(!fresh(s->rows_original[i].native_row_original,sizeof(sl_win_call)))return 0;
  for(SIZE_T j=0;j<i;j++)if(s->rows_original[j].native_row_original==s->rows_original[i].native_row_original||
    (s->rows_original[j].kind==s->rows_original[i].kind&&s->rows_original[j].occurrence==s->rows_original[i].occurrence&&
     s->rows_original[j].member==s->rows_original[i].member&&s->rows_original[j].slot==s->rows_original[i].slot))return 0;
 }
 q->attempted=1;q->actual_source=s;q->source_snapshot=*s;q->arena_original=c->arena_original;q->memory_original=c->memory_original;
 q->initial_source_original=c->initial_source_original;
 q->deadline_original=c->deadline_original;q->origin_original=c->deadline_original->origin;q->origin_tick=q->origin_original->tick;q->expires=c->deadline_original->expires;
 for(unsigned i=0;i<2;i++){q->origin_raw[i]=q->origin_original->raw[i];q->origin_status[i]=q->origin_original->status[i];q->origin_args[i]=q->origin_original->args[i];}
 q->steps_original=steps;q->capacity=s->row_count;
 for(SIZE_T i=0;i<q->capacity;i++){
  steps[i].sequence_original=q;steps[i].source_original=&s->rows_original[i];steps[i].source_snapshot=s->rows_original[i];
  steps[i].deadline_original=q->deadline_original;steps[i].caller_original=c;
 }
 q->returned=1;q->result=1;return 1;
}
int sl_win_candidate_deadline_matches_original(const sl_win_candidate_deadline_sequence *q){
 sl_win_candidate_deadline_step *steps;
 if(!q||!q->attempted||!q->returned||!q->result||q->source_original!=q->actual_source||!q->source_original)return 0;
 const sl_win_candidate_deadline_source *s=q->source_original;
 if(s->preparation_original!=q->source_snapshot.preparation_original||s->caller_original!=q->source_snapshot.caller_original||
    s->rows_original!=q->source_snapshot.rows_original||s->row_count!=q->source_snapshot.row_count||!source(s,&steps)||!caller(s->caller_original)||!live(s))return 0;
 const struct slcli_product_windows_operation_caller *c=s->caller_original;
 if(c->arena_original!=q->arena_original||c->memory_original!=q->memory_original||c->initial_source_original!=q->initial_source_original||c->deadline_original!=q->deadline_original||
    q->deadline_original->origin!=q->origin_original||q->deadline_original->expires!=q->expires||q->origin_original->tick!=q->origin_tick||
    steps!=q->steps_original||s->row_count!=q->capacity||!excludes(s,q,sizeof(*q))||!excludes(s,steps,q->capacity*sizeof(*steps))||
    !away(q,sizeof(*q),s->preparation_original->backing_original,s->preparation_original->backing_extent)||
    !slcli_arena_live_range(c->arena_original,q,sizeof(*q))||!slcli_arena_live_range(c->arena_original,c,sizeof(*c))||
    !slcli_arena_live_range(c->arena_original,steps,q->capacity*sizeof(*steps)))return 0;
 for(unsigned i=0;i<2;i++)if(q->origin_raw[i]!=q->origin_original->raw[i]||q->origin_status[i]!=q->origin_original->status[i]||q->origin_args[i]!=q->origin_original->args[i])return 0;
 for(SIZE_T i=0;i<q->capacity;i++){
  if(steps[i].sequence_original!=q||steps[i].source_original!=&s->rows_original[i]||!row_equal(&steps[i].source_snapshot,&s->rows_original[i])||
     steps[i].deadline_original!=q->deadline_original||steps[i].caller_original!=c)return 0;
 }
 return 1; /* Not clock validity of later cuts, dispatch permission, or closure. */
}
#endif
