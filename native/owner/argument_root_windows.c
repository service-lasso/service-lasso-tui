#ifdef _WIN32
#include "argument_root_windows.h"
#include <string.h>
static int root_source_zero(const void *p,SIZE_T n){const unsigned char *b=p;for(SIZE_T i=0;i<n;i++)if(b[i])return 0;return 1;}
static int root_source_disjoint(const void *a,SIZE_T an,const void *b,SIZE_T bn){ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;return x&&y&&an&&bn&&x<=~(ULONG_PTR)0-an&&y<=~(ULONG_PTR)0-bn&&!(x<y+bn&&y<x+an);}
int sl_win_prepare_argument_root_source(sl_win_argument_root_source *r){
 const sl_win_public_startup *o;const sl_win_birth *b;const sl_win_argument_sender *s;const sl_win_argument_send_sequence *q;
 if(!r||r->attempted||r->returned||r->actual_startup||r->result||!root_source_zero(&r->root,sizeof(r->root)))return 0;
 o=r->startup_original;
 if(!o||!root_source_disjoint(r,sizeof(*r),o,sizeof(*o))||!sl_win_public_startup_factory_disjoint(o,r,sizeof(*r)))return 0;
 s=&o->argument_sender;q=&o->argument_sequence;b=&o->controller.entry.birth;
 if(!q->actual_turns||!q->actual_turn_capacity||q->actual_turn_capacity>~(SIZE_T)0/sizeof(*q->actual_turns)||!root_source_disjoint(r,sizeof(*r),q->actual_turns,q->actual_turn_capacity*sizeof(*q->actual_turns)))return 0;
 if(!s->returned||!s->result||!s->span_count||s->span_count>4)return 0;
 for(SIZE_T i=0;i<s->span_count;i++)if(!root_source_disjoint(r,sizeof(*r),s->spans[i].bytes,s->spans[i].extent))return 0;
 r->attempted=1;r->actual_startup=o;
 if(!o->returned||!o->result||!o->controller_result||!o->sender_result||!o->sequence_result||s->actual_arguments!=&o->arguments||s->actual_duplex!=&o->duplex||!q->returned||!q->prepared||q->actual_sender!=s||q->span_count!=s->span_count||q->span_count>4||q->actual_turns!=q->turns||q->actual_turn_capacity!=q->turn_capacity||q->turn_index||q->turn_exhausted||q->active_span||q->active_settled||q->cancelled||q->failed||q->delivered)goto root_source_done;
 for(SIZE_T i=0;i<q->span_count;i++)if(q->workers[i].attempted||q->writes[i].attempted)goto root_source_done;
 if(!b->create_process.returned||!b->create_process.result||!b->process.hProcess||b->process.hProcess==INVALID_HANDLE_VALUE||!b->process.hThread||b->process.hThread==INVALID_HANDLE_VALUE||!(b->creation_flags&CREATE_SUSPENDED)||!(b->creation_flags&DEBUG_ONLY_THIS_PROCESS))goto root_source_done;
 if(!o->arguments.returned||o->arguments.result||!o->arguments.transport_prepared||b->inherited_child_endpoint!=o->arguments.endpoint_original||b->inherited_child_endpoint!=o->duplex.inherited_client||b->inherited_entry_gate!=o->channels.inherited_gate||!b->inherited_child_endpoint||b->inherited_child_endpoint==INVALID_HANDLE_VALUE||!b->inherited_entry_gate||b->inherited_entry_gate==INVALID_HANDLE_VALUE)goto root_source_done;
 memcpy(r->root.layout_tag,"SLWROOT1",8);r->root.layout_version=1;r->root.source_extent=sizeof(r->root);
 r->root.endpoint=(uint64_t)(ULONG_PTR)b->inherited_child_endpoint;r->root.entry_gate=(uint64_t)(ULONG_PTR)b->inherited_entry_gate;
 r->root.public_count=o->arguments.public_count;r->root.security_label_bytes=o->arguments.security_label_bytes;r->root.terminal_calls=o->arguments.terminal_calls;r->root.representation_bytes=o->arguments.representation_bytes;
 memcpy(r->root.argument_header,o->arguments.transport_header,sizeof(r->root.argument_header));r->root.phase=1;
 r->result=1;
root_source_done:r->returned=1;return r->result;
}
#endif
