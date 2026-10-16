#ifdef _WIN32
#include "candidate_prepare_windows.h"
#include "debug_root_windows.h"
#include "argument_root_windows.h"
#include "argument_reader_cancel_windows.h"
#include "candidate_profile.h"
#include <stdint.h>
_Static_assert(sizeof(SIZE_T)==8&&sizeof(void *)==8,"selected Windows amd64 source ABI");
static int fresh(const void *p,SIZE_T n){const unsigned char *b=p;for(SIZE_T i=0;i<n;i++)if(b[i])return 0;return 1;}
static int away(const void *a,SIZE_T an,const void *b,SIZE_T bn){ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;return x&&y&&an&&bn&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);}
static void *view(sl_win_candidate_preparation *p,unsigned role,SIZE_T unit,SIZE_T alignment){
 const sl_win_candidate_layout_item *v=&p->plan_original->copies_original[role];
 if(v->element_extent!=unit||v->alignment<alignment||v->offset%alignment||!v->count)return NULL;
 return (unsigned char *)p->backing_original+v->offset;
}
int sl_win_prepare_candidate_views(sl_win_candidate_preparation *p){
 if(!p||p->attempted||p->returned||p->startup_original||p->factory_original||p->result||
    !sl_win_candidate_layout_matches_original(p->plan_original)||!p->backing_original||
    p->backing_extent!=p->plan_original->bytes||!p->writer_stack_original||
    (ULONG_PTR)p->backing_original%p->plan_original->alignment||
    !away(p,sizeof(*p),p->backing_original,p->backing_extent)||
    !sl_win_candidate_layout_excludes_output(p->plan_original,p,sizeof(*p))||
    !sl_win_candidate_layout_excludes_output(p->plan_original,p->backing_original,p->backing_extent)||
    !fresh(p->backing_original,p->backing_extent))return 0;
 /* Validate every existing native startup/debug/profile typed view before
  * writing backing. Generated Go/query/helper geometry remains on the complete
  * retained plan; its genuine compiled profile/source binding is not invented. */
 void *v[SL_WIN_LAYOUT_ROLE_COUNT]={0};
 #define V(role,type) do{v[role]=view(p,role,sizeof(type),_Alignof(type));if(!v[role])return 0;}while(0)
 V(SL_WIN_LAYOUT_STARTUP,sl_win_public_startup);V(SL_WIN_LAYOUT_FACTORY_CONTROL,sl_win_argument_sender_factory);
 V(SL_WIN_LAYOUT_PIPE_NAME,WCHAR);V(SL_WIN_LAYOUT_TOKEN_USER,unsigned char);V(SL_WIN_LAYOUT_ACL,unsigned char);
 V(SL_WIN_LAYOUT_CONNECTIONS,sl_win_duplex_completion);V(SL_WIN_LAYOUT_SETTLEMENTS,sl_win_duplex_settlement);
 V(SL_WIN_LAYOUT_ARGUMENT_OUTPUT,WCHAR);V(SL_WIN_LAYOUT_MODULE_ARENA,WCHAR);V(SL_WIN_LAYOUT_MODULE_QUERIES,sl_win_module_query);
 V(SL_WIN_LAYOUT_COMMAND,WCHAR);V(SL_WIN_LAYOUT_SERVICES,sl_win_service_observation);V(SL_WIN_LAYOUT_ATTRIBUTES,unsigned char);
 for(unsigned i=SL_WIN_LAYOUT_SECURITY_0;i<=SL_WIN_LAYOUT_ENDPOINT_SECURITY;i++)V(i,unsigned char);
 V(SL_WIN_LAYOUT_TURNS,sl_win_argument_send_turn);V(SL_WIN_LAYOUT_DEBUG_EVENTS,sl_win_debug_event_observation);
 V(SL_WIN_LAYOUT_MAPPED_ROOTS,sl_win_mapped_root_observation);V(SL_WIN_LAYOUT_ROOT_WRITES,sl_win_root_write);
 V(SL_WIN_LAYOUT_DEBUG_RELEASES,sl_win_debug_release);V(SL_WIN_LAYOUT_ARGUMENT_ROOTS,sl_win_argument_root_source);
 V(SL_WIN_LAYOUT_READER_CANCELS,sl_win_argument_reader_cancel);V(SL_WIN_LAYOUT_PROFILE_CONTROL,sl_candidate_profile);
 V(SL_WIN_LAYOUT_PROFILE_BYTES,unsigned char);V(SL_WIN_LAYOUT_PROFILE_ROWS,sl_candidate_record);
 #undef V
 if(p->plan_original->copies_original[SL_WIN_LAYOUT_STARTUP].count!=1||p->plan_original->copies_original[SL_WIN_LAYOUT_FACTORY_CONTROL].count!=1)return 0;
 const sl_win_candidate_layout_item *rows=p->plan_original->copies_original;
 for(unsigned i=SL_WIN_LAYOUT_SECURITY_0;i<=SL_WIN_LAYOUT_SECURITY_5;i++)if(rows[i].extent>MAXDWORD||rows[i].alignment<_Alignof(SECURITY_DESCRIPTOR))return 0;
 for(unsigned i=SL_WIN_LAYOUT_TOKEN_0;i<=SL_WIN_LAYOUT_TOKEN_1;i++)if(rows[i].extent>MAXDWORD||rows[i].extent<sizeof(TOKEN_ACCESS_INFORMATION)||rows[i].alignment<_Alignof(TOKEN_ACCESS_INFORMATION))return 0;
 if(rows[SL_WIN_LAYOUT_TOKEN_USER].extent>MAXDWORD||rows[SL_WIN_LAYOUT_TOKEN_USER].extent<sizeof(TOKEN_USER)||rows[SL_WIN_LAYOUT_TOKEN_USER].alignment<_Alignof(TOKEN_USER)||
    rows[SL_WIN_LAYOUT_ACL].extent>MAXDWORD||rows[SL_WIN_LAYOUT_ACL].extent<sizeof(ACL)||rows[SL_WIN_LAYOUT_ACL].alignment<_Alignof(ACL)||
    rows[SL_WIN_LAYOUT_ENDPOINT_SECURITY].extent>MAXDWORD||rows[SL_WIN_LAYOUT_ENDPOINT_SECURITY].alignment<_Alignof(SECURITY_DESCRIPTOR)||
    rows[SL_WIN_LAYOUT_ATTRIBUTES].alignment<_Alignof(void *)||
    rows[SL_WIN_LAYOUT_COMMAND].count<rows[SL_WIN_LAYOUT_ARGUMENT_OUTPUT].count)return 0;
 p->attempted=1;
 sl_win_public_startup *o=v[SL_WIN_LAYOUT_STARTUP];sl_win_birth *b=&o->controller.entry.birth;
 #define B(field,role) do{field=v[role];}while(0)
 B(o->duplex.name,SL_WIN_LAYOUT_PIPE_NAME);o->duplex.name_capacity=rows[SL_WIN_LAYOUT_PIPE_NAME].count;
 B(o->duplex.token_body,SL_WIN_LAYOUT_TOKEN_USER);o->duplex.token_capacity=(DWORD)rows[SL_WIN_LAYOUT_TOKEN_USER].extent;
 B(o->duplex.acl_body,SL_WIN_LAYOUT_ACL);o->duplex.acl_capacity=(DWORD)rows[SL_WIN_LAYOUT_ACL].extent;
 B(o->duplex.completions,SL_WIN_LAYOUT_CONNECTIONS);o->duplex.completion_capacity=rows[SL_WIN_LAYOUT_CONNECTIONS].count;
 B(o->duplex.settlements,SL_WIN_LAYOUT_SETTLEMENTS);o->duplex.settlement_capacity=rows[SL_WIN_LAYOUT_SETTLEMENTS].count;
 B(o->arguments.output,SL_WIN_LAYOUT_ARGUMENT_OUTPUT);o->arguments.output_capacity=rows[SL_WIN_LAYOUT_ARGUMENT_OUTPUT].count;
 B(o->arguments.module,SL_WIN_LAYOUT_MODULE_ARENA);o->arguments.module_capacity=rows[SL_WIN_LAYOUT_MODULE_ARENA].count;
 B(o->arguments.module_queries,SL_WIN_LAYOUT_MODULE_QUERIES);o->arguments.module_query_capacity=rows[SL_WIN_LAYOUT_MODULE_QUERIES].count;
 B(b->command_line,SL_WIN_LAYOUT_COMMAND);o->command_line_capacity=rows[SL_WIN_LAYOUT_COMMAND].count;
 B(o->controller.services,SL_WIN_LAYOUT_SERVICES);o->controller.service_capacity=rows[SL_WIN_LAYOUT_SERVICES].count;
 B(b->attribute_backing,SL_WIN_LAYOUT_ATTRIBUTES);b->attribute_capacity=rows[SL_WIN_LAYOUT_ATTRIBUTES].extent;
 for(unsigned i=0;i<6;i++){b->security[i].backing=v[SL_WIN_LAYOUT_SECURITY_0+i];b->security[i].capacity=(DWORD)rows[SL_WIN_LAYOUT_SECURITY_0+i].extent;}
 for(unsigned i=0;i<2;i++){b->tokens[i].backing=v[SL_WIN_LAYOUT_TOKEN_0+i];b->tokens[i].capacity=(DWORD)rows[SL_WIN_LAYOUT_TOKEN_0+i].extent;}
 B(o->endpoint.security.backing,SL_WIN_LAYOUT_ENDPOINT_SECURITY);o->endpoint.security.capacity=(DWORD)rows[SL_WIN_LAYOUT_ENDPOINT_SECURITY].extent;
 #undef B
 /* Factory/turn association and startup native calls are deliberately NOT
  * dispatched. Actual typed CLI operation/finance/deadline binding is missing. */
 p->startup_original=o;p->factory_original=v[SL_WIN_LAYOUT_FACTORY_CONTROL];p->returned=1;p->result=1;return 1;
}
#endif
