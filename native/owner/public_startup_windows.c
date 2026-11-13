#ifdef _WIN32
#include "public_startup_windows.h"
static int startup_zero(const void *p,SIZE_T n){const unsigned char *s=p;for(SIZE_T i=0;i<n;i++)if(s[i])return 0;return 1;}
static int startup_sequence_fresh(const sl_win_argument_send_sequence *s){
 return s&&!s->sender_original&&!s->actual_sender&&s->stack_reservation&&s->turns&&s->turn_capacity&&!s->actual_turns&&!s->actual_turn_capacity&&!s->turn_index&&!s->turn_exhausted&&!s->actual_stack_reservation&&!s->span_count&&!s->active_span&&!s->attempted&&!s->returned&&!s->prepared&&!s->cancelled&&!s->failed&&!s->delivered&&!s->active_settled&&startup_zero(s->writes,sizeof(s->writes))&&startup_zero(s->workers,sizeof(s->workers));
}
static int startup_region(const void *a,SIZE_T an,const void *b,SIZE_T bn){ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;if(!x||!y||!an||!bn||x>~(ULONG_PTR)0-an||y>~(ULONG_PTR)0-bn)return 0;return !(x<y+bn&&y<x+an);}
static int startup_backing(const sl_win_public_startup *o,const void *factory,SIZE_T factory_extent,unsigned include_turns){
 const sl_win_birth *b=&o->controller.entry.birth;const void *regions[21];SIZE_T sizes[21],n=0;
 if(o->argument_sequence.turn_capacity>~(SIZE_T)0/sizeof(sl_win_argument_send_turn)||o->arguments.module_query_capacity>~(SIZE_T)0/sizeof(sl_win_module_query)||o->duplex.name_capacity>~(SIZE_T)0/sizeof(WCHAR)||o->duplex.completion_capacity>~(SIZE_T)0/sizeof(sl_win_duplex_completion)||o->duplex.settlement_capacity>~(SIZE_T)0/sizeof(sl_win_duplex_settlement)||o->arguments.output_capacity>~(SIZE_T)0/sizeof(WCHAR)||o->arguments.module_capacity>~(SIZE_T)0/sizeof(WCHAR)||o->command_line_capacity>~(SIZE_T)0/sizeof(WCHAR)||o->controller.service_capacity>~(SIZE_T)0/sizeof(sl_win_service_observation))return 0;
 #define REGION(pointer,bytes) do{regions[n]=(pointer);sizes[n]=(bytes);n++;}while(0)
 REGION(o->duplex.name,o->duplex.name_capacity*sizeof(WCHAR));REGION(o->duplex.token_body,o->duplex.token_capacity);REGION(o->duplex.acl_body,o->duplex.acl_capacity);REGION(o->duplex.completions,o->duplex.completion_capacity*sizeof(sl_win_duplex_completion));
 REGION(o->duplex.settlements,o->duplex.settlement_capacity*sizeof(sl_win_duplex_settlement));
 REGION(o->arguments.output,o->arguments.output_capacity*sizeof(WCHAR));REGION(o->arguments.module,o->arguments.module_capacity*sizeof(WCHAR));REGION(o->arguments.module_queries,o->arguments.module_query_capacity*sizeof(sl_win_module_query));REGION(b->command_line,o->command_line_capacity*sizeof(WCHAR));REGION(o->controller.services,o->controller.service_capacity*sizeof(sl_win_service_observation));REGION(b->attribute_backing,b->attribute_capacity);
 for(unsigned i=0;i<6;i++)REGION(b->security[i].backing,b->security[i].capacity);
 for(unsigned i=0;i<2;i++)REGION(b->tokens[i].backing,b->tokens[i].capacity);
 REGION(o->endpoint.security.backing,o->endpoint.security.capacity);
 if(include_turns)REGION(o->argument_sequence.turns,o->argument_sequence.turn_capacity*sizeof(sl_win_argument_send_turn));
 #undef REGION
 if(n!=(include_turns?21:20))return 0;
 if(factory&&!startup_region(factory,factory_extent,o,sizeof(*o)))return 0;
 for(SIZE_T i=0;i<n;i++){if(!startup_region(regions[i],sizes[i],o,sizeof(*o))||(factory&&!startup_region(regions[i],sizes[i],factory,factory_extent)))return 0;for(SIZE_T j=0;j<i;j++)if(!startup_region(regions[i],sizes[i],regions[j],sizes[j]))return 0;}
 return 1;
}
int sl_win_public_startup_factory_disjoint(const sl_win_public_startup *o,const void *factory,SIZE_T extent){
 return o&&factory&&extent&&startup_backing(o,factory,extent,0);
}
static int startup_body(sl_win_public_startup *o){
 sl_win_birth *b=&o->controller.entry.birth;
 o->failed_stage=1;
 if(o->controller.attempted||o->controller.returned||o->duplex.attempted||o->duplex.returned||o->arguments.attempted||o->arguments.returned||o->arguments.endpoint_original||o->channels.attempted||o->channels.returned||!startup_zero(&o->argument_sender,sizeof(o->argument_sender))||!startup_sequence_fresh(&o->argument_sequence)||b->inherited_entry_gate||b->inherited_child_endpoint||b->command_line_original||b->command_line_units||!b->command_line||o->command_line_capacity<o->arguments.output_capacity||!startup_backing(o,NULL,0,1))return 0;
 o->actual_command_line=b->command_line;o->actual_command_line_capacity=o->command_line_capacity;
 o->failed_stage=2;o->streams_result=sl_win_prepare_public_streams(&o->controller.entry);if(!o->streams_result)return 0;
 o->failed_stage=3;o->channels_result=sl_win_construct_channels(&o->channels);
 if(!o->channels_result||!o->channels.returned||!o->channels.result||!o->channels.inherited_gate||!o->channels.read[1]||!o->channels.write[2])return 0;
 b->inherited_entry_gate=o->channels.inherited_gate;
 o->failed_stage=3;o->duplex_result=sl_win_construct_duplex(&o->duplex);if(!o->duplex_result)return 0;
 while(o->duplex.pending&&!o->duplex.connected){
  o->completion_result=sl_win_observe_duplex_completion(&o->duplex);if(!o->completion_result)return 0;
 }
 if(!o->duplex.connected||!o->duplex.client_duplicate.returned||!o->duplex.client_duplicate.result||!o->duplex.inherited_client)return 0;
 o->failed_stage=4;
 if(o->endpoint.endpoint_original||o->endpoint.attempted||o->endpoint.returned)return 0;
 o->endpoint.endpoint_original=o->duplex.client;
 o->endpoint_result=sl_win_observe_entry_endpoint(&o->endpoint);if(!o->endpoint_result)return 0;
 /* Supplier construction correspondence only; child earliest authentication
  * still joins its separate SAME-endpoint native originals before any bytes. */
 if(o->endpoint.server_process_id!=o->duplex.pid||
    (o->endpoint.flags&(PIPE_SERVER_END|PIPE_TYPE_MESSAGE))||o->endpoint.maximum_instances!=1)return 0;
 o->failed_stage=5;o->arguments.endpoint_original=o->duplex.inherited_client;
 o->arguments_result=sl_win_construct_private_arguments(&o->arguments);
 if(o->arguments_result||!o->arguments.returned||!o->arguments.actual_output||!o->arguments.output_units||o->arguments.output_units>o->actual_command_line_capacity)return 0;
 b->inherited_child_endpoint=o->duplex.inherited_client;b->command_line_original=o->arguments.actual_output;b->command_line=o->actual_command_line;b->command_line_units=o->arguments.output_units;
 o->failed_stage=6;o->argument_sender.arguments_original=&o->arguments;o->argument_sender.duplex_original=&o->duplex;
 o->sender_result=sl_win_prepare_argument_sender(&o->argument_sender);if(!o->sender_result)return 0;
 if(o->argument_sequence.sender_original)return 0;
 o->argument_sequence.sender_original=&o->argument_sender;
 o->sequence_result=sl_win_prepare_argument_send_sequence(&o->argument_sequence);if(!o->sequence_result)return 0;
 o->failed_stage=7;o->controller_result=sl_win_construct_public_controller_from_prepared_streams(&o->controller);if(!o->controller_result)return 0;
 o->failed_stage=0;return 1;
}
int sl_win_construct_public_startup(sl_win_public_startup *o){
 if(!o||o->attempted||o->returned||o->actual_command_line||o->actual_command_line_capacity||o->failed_stage||o->streams_result||o->channels_result||o->duplex_result||o->completion_result||o->endpoint_result||o->arguments_result||o->sender_result||o->sequence_result||o->controller_result||o->result||o->cancellation_result||o->settlement_result||o->settlement_exhausted)return 0;
 o->attempted=1;o->result=startup_body(o);
 if(!o->result&&o->duplex.returned&&o->duplex.pending&&!o->duplex.operation_settled){
  o->cancellation_result=sl_win_request_duplex_cancel(&o->duplex);
  while(o->duplex.cancel_returned&&o->duplex.pending&&o->duplex.settlement_count<o->duplex.actual_settlement_capacity){
   o->settlement_result=sl_win_observe_duplex_settlement(&o->duplex);if(!o->settlement_result)break;
  }
  if(o->duplex.pending&&o->duplex.settlement_count==o->duplex.actual_settlement_capacity)o->settlement_exhausted=1;
 }
 /* Primary failed stage/result remains unchanged even if the kernel borrower
  * settles. All handles/backing still await genuine custody/disposition. */
 o->returned=1;return o->result;
}
#endif
