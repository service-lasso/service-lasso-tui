#ifdef _WIN32
#include "../public_startup_windows.h"
#include <assert.h>
#include <string.h>
static unsigned mode,step,completion_calls;
int sl_win_prepare_public_streams(sl_win_entry *e){assert(++step==1);e->public_streams_attempted=1;if(mode==1)return 0;e->public_streams_prepared=1;return 1;}
int sl_win_construct_channels(sl_win_channels *c){assert(++step==2);c->attempted=c->returned=1;c->read[1]=(HANDLE)(ULONG_PTR)70;if(mode==7)return 0;c->result=1;c->inherited_gate=(HANDLE)(ULONG_PTR)71;c->write[2]=(HANDLE)(ULONG_PTR)72;return 1;}
int sl_win_construct_duplex(sl_win_duplex *d){assert(++step==3);d->attempted=d->returned=1;d->pending=1;d->actual_settlements=d->settlements;d->actual_settlement_capacity=d->settlement_capacity;d->server=(HANDLE)(ULONG_PTR)20;if(mode==2)return 0;d->client=(HANDLE)(ULONG_PTR)49;d->pid=123;d->inherited_client=(HANDLE)(ULONG_PTR)50;d->client_duplicate.returned=1;d->client_duplicate.result=TRUE;return 2;}
int sl_win_observe_duplex_completion(sl_win_duplex *d){assert(step==3||step==4);step++;completion_calls++;if(mode==3){d->completion_failed=1;return 0;}if(completion_calls==1)return 2;d->pending=0;d->connected=1;return 1;}
/* UNRUN source fixture, not genuine native endpoint/provider evidence. */
int sl_win_observe_entry_endpoint(sl_win_endpoint_observation *o){assert(++step==6);assert(o->endpoint_original==(HANDLE)(ULONG_PTR)49);o->attempted=o->returned=1;if(mode==8)return 0;o->server_process_id=mode==9?999:123;o->maximum_instances=mode==10?2:1;o->result=1;return 1;}
int sl_win_construct_private_arguments(sl_win_private_arguments *a){assert(++step==7);assert(a->endpoint_original==(HANDLE)(ULONG_PTR)50);a->attempted=a->returned=1;if(mode==4){a->result=-1;return -1;}a->actual_output=a->output;a->output[0]=L'p';a->output[1]=0;a->output_units=2;a->transport_prepared=1;a->public_count=2;a->security_label_bytes=256;a->terminal_calls=3;a->representation_bytes=4;a->transport_header[0]=42;return 0;}
int sl_win_prepare_argument_sender(sl_win_argument_sender *s){assert(++step==8);assert(s->arguments_original&&s->arguments_original->returned&&s->duplex_original&&s->duplex_original->connected);s->attempted=s->returned=1;if(mode==12)return 0;s->actual_arguments=s->arguments_original;s->actual_duplex=s->duplex_original;s->span_count=1;s->spans[0].bytes=s->arguments_original->transport_header;s->spans[0].extent=48;s->result=1;return 1;}
int sl_win_prepare_argument_send_sequence(sl_win_argument_send_sequence *s){assert(++step==9);assert(s->sender_original&&s->sender_original->result&&s->stack_reservation);s->attempted=s->returned=1;if(mode==13)return 0;s->actual_sender=s->sender_original;s->span_count=s->sender_original->span_count;s->actual_turns=s->turns;s->actual_turn_capacity=s->turn_capacity;s->prepared=1;return 1;}
int sl_win_construct_public_controller_from_prepared_streams(sl_win_public_controller *c){assert(++step==10);assert(c->entry.public_streams_prepared);assert(c->entry.birth.inherited_child_endpoint==(HANDLE)(ULONG_PTR)50);assert(c->entry.birth.command_line_original[0]==L'p'&&c->entry.birth.command_line_units==2&&c->entry.birth.command_line!=c->entry.birth.command_line_original);c->attempted=c->returned=1;c->entry.birth.attempted=1;c->entry.birth.process.hProcess=(HANDLE)(ULONG_PTR)80;c->entry.birth.process.hThread=(HANDLE)(ULONG_PTR)81;c->entry.birth.create_process.entered=c->entry.birth.create_process.returned=1;c->entry.birth.create_process.result=TRUE;c->entry.birth.creation_flags=CREATE_SUSPENDED|DEBUG_ONLY_THIS_PROCESS;return mode==5?0:1;}
int sl_win_request_duplex_cancel(sl_win_duplex *d){assert(d->pending);d->cancel_returned=1;return 0;}
int sl_win_observe_duplex_settlement(sl_win_duplex *d){assert(d->cancel_returned&&d->pending);d->settlement_count++;return 2;}
#include "../public_startup_windows.c"
/* Synthetic source dimensions, never an admitted native profile. */
#define SL_ARGUMENT_WRITER_STACK_RESERVATION_BYTES UINT64_C(4096)
#define SL_ARGUMENT_WRITER_TURN_CAPACITY UINT64_C(2)
#include "../argument_sender_factory_windows.c"
#include "../argument_root_windows.c"
typedef struct fixture {
 sl_win_public_startup owner;
 sl_win_argument_root_source roots[4];
 WCHAR name[128],output[128],module[128],mutable_command[128];
 union {max_align_t alignment;unsigned char bytes[256];} token,acl,attribute,endpoint_security,security[6],tokens[2];
 sl_win_module_query module_queries[2];
 sl_win_argument_send_turn turns[2];
 sl_win_argument_sender_factory factory;
 sl_win_duplex_completion completions[3];sl_win_duplex_settlement settlements[3];sl_win_service_observation services[2];
} fixture;
int main(void){
 static fixture cases[15];
 for(mode=0;mode<15;mode++){
  fixture *f=&cases[mode];sl_win_public_startup *o=&f->owner;sl_win_birth *b=&o->controller.entry.birth;
  o->duplex.name=f->name;o->duplex.name_capacity=128;o->duplex.token_body=f->token.bytes;o->duplex.token_capacity=256;o->duplex.acl_body=f->acl.bytes;o->duplex.acl_capacity=256;o->duplex.completions=f->completions;o->duplex.completion_capacity=3;o->duplex.settlements=f->settlements;o->duplex.settlement_capacity=3;
  o->arguments.output=f->output;o->arguments.output_capacity=128;o->arguments.module=f->module;o->arguments.module_capacity=128;o->arguments.module_queries=f->module_queries;o->arguments.module_query_capacity=2;
  /* Synthetic stack dimension only; no real profile/finance/native authority. */
  o->argument_sequence.stack_reservation=4096;o->argument_sequence.turns=f->turns;o->argument_sequence.turn_capacity=2;
  for(unsigned i=0;i<2;i++)f->turns[i].sequence_original=&o->argument_sequence;
  o->endpoint.security.backing=f->endpoint_security.bytes;o->endpoint.security.capacity=256;
  b->command_line=f->mutable_command;o->command_line_capacity=128;o->controller.services=f->services;o->controller.service_capacity=2;b->attribute_backing=f->attribute.bytes;b->attribute_capacity=256;
  for(unsigned i=0;i<6;i++){b->security[i].backing=f->security[i].bytes;b->security[i].capacity=256;}for(unsigned i=0;i<2;i++){b->tokens[i].backing=f->tokens[i].bytes;b->tokens[i].capacity=256;}
  if(mode==6)o->arguments.output=f->name;
  if(mode==11)o->argument_sequence.stack_reservation=0;
  if(mode==14){
   unsigned char before[sizeof(f->factory)];
   memset(&o->argument_sequence,0,sizeof(o->argument_sequence));
   memset(f->turns,0,sizeof(f->turns));
   memcpy(before,&f->factory,sizeof(before));
   b->command_line=(WCHAR *)(void *)&f->factory;
   assert(!sl_win_bind_argument_sender_factory(&f->factory,o,f->turns,2,4096));
   assert(!memcmp(before,&f->factory,sizeof(before))&&!o->argument_sequence.turns);
   b->command_line=(WCHAR *)(void *)f->turns;
   assert(!sl_win_bind_argument_sender_factory(&f->factory,o,f->turns,2,4096));
   assert(!memcmp(before,&f->factory,sizeof(before))&&!o->argument_sequence.turns);
   b->command_line=f->mutable_command;
   assert(!sl_win_bind_argument_sender_factory(&f->factory,o,f->turns,1,4096));
   assert(!sl_win_bind_argument_sender_factory(&f->factory,o,f->turns,2,8192));
   assert(sl_win_bind_argument_sender_factory(&f->factory,o,f->turns,2,4096));
   assert(o->argument_sequence.turns==f->turns&&o->argument_sequence.turn_capacity==2&&o->argument_sequence.stack_reservation==4096);
   assert(!sl_win_bind_argument_sender_factory(&f->factory,o,f->turns,2,4096));
  }
  step=completion_calls=0;int result=sl_win_construct_public_startup(o);
  assert(o->attempted&&o->returned);
  if(mode==0||mode==14){assert(result&&step==10&&o->failed_stage==0&&b->command_line_original==f->output&&o->actual_command_line==f->mutable_command);}
  else{assert(!result);if(mode==6||mode==11)assert(!step&&o->failed_stage==1&&!o->arguments.endpoint_original);else if(mode==1)assert(step==1&&o->failed_stage==2&&!b->inherited_child_endpoint);else if(mode==7)assert(step==2&&o->failed_stage==3&&o->channels.read[1]==(HANDLE)(ULONG_PTR)70&&!b->inherited_entry_gate&&!o->duplex.attempted);else if(mode==2||mode==3)assert(o->duplex.pending&&!b->inherited_child_endpoint&&!o->arguments.attempted);else if(mode==4)assert(step==7&&o->arguments_result==-1&&!b->inherited_child_endpoint&&!b->command_line_original);else if(mode==12||mode==13)assert(step==(mode==12?8:9)&&o->failed_stage==6&&!b->attempted);else if(mode>=8)assert(step==6&&o->failed_stage==4&&o->endpoint.returned&&!o->arguments.attempted);else assert(step==10&&b->process.hProcess==(HANDLE)(ULONG_PTR)80&&o->failed_stage==7);}
  {unsigned before=step;assert(!sl_win_construct_public_startup(o)&&step==before);}
  if(mode==0||mode==14){
   sl_win_argument_root_source *root=&f->roots[0];root->startup_original=o;
   assert(sl_win_prepare_argument_root_source(root));
   assert(root->actual_startup==o&&root->returned&&root->root.source_extent==128&&root->root.layout_version==1&&root->root.phase==1);
   assert(!memcmp(root->root.layout_tag,"SLWROOT1",8)&&!memcmp(root->root.argument_header,o->arguments.transport_header,48));
   assert(root->root.endpoint==50&&root->root.entry_gate==71&&root->root.public_count==2&&root->root.security_label_bytes==256&&root->root.terminal_calls==3&&root->root.representation_bytes==4);
   {sl_win_argument_root_source before=*root;assert(!sl_win_prepare_argument_root_source(root)&&!memcmp(&before,root,sizeof(before)));}
   f->roots[1].startup_original=o;o->argument_sequence.span_count=5;
   assert(!sl_win_prepare_argument_root_source(&f->roots[1])&&f->roots[1].returned&&!f->roots[1].result);
   o->argument_sequence.span_count=1;f->roots[2].startup_original=o;o->argument_sequence.workers[0].attempted=1;
   assert(!sl_win_prepare_argument_root_source(&f->roots[2])&&f->roots[2].returned&&!f->roots[2].result);
   o->argument_sequence.workers[0].attempted=0;f->roots[3].startup_original=o;
   o->argument_sender.spans[0].bytes=(const unsigned char *)&f->roots[3];o->argument_sender.spans[0].extent=sizeof(f->roots[3]);
   {sl_win_argument_root_source before=f->roots[3];assert(!sl_win_prepare_argument_root_source(&f->roots[3])&&!memcmp(&before,&f->roots[3],sizeof(before)));}
  }
 }
 return 0;
}
#endif
