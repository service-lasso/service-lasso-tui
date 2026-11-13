/* UNRUN controlled parser and status memory seams, not native/profile proof. */
#if defined(__linux__) && defined(__x86_64__)
#include <assert.h>
#include <string.h>
#include "../policy_parse_linux.h"
static unsigned calls,change_status;static int parse_result=-47;static unsigned char bytes[3]={1,2,3};static uint32_t words[5]={1,2,1,0,1};
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){assert(0);return -1;}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *o,int fd){assert(0);return -1;}
#include "../selinux_status_linux.c"
int sl_sepol_parse_owned(sl_sepol_parse_owner *o){
 calls++;assert(o->body==bytes&&o->extent==3);o->attempted=o->returned=1;o->init_entered=o->init_returned=1;o->init_result=parse_result;o->result=parse_result;if(change_status)words[1]=4;return parse_result;
}
#include "../policy_parse_linux.c"
static void snapshot_ready(sl_linux_selinux_snapshot *s){
 memset(s,0,sizeof *s);words[1]=2;s->attempted=s->returned=s->eof=1;s->actual_body=bytes;s->length=s->actual_body_capacity=3;
 s->status.returned=1;s->status.actual_words=words;s->before_policy.status_original=s->after_policy.status_original=&s->status;
 assert(!sl_linux_sample_selinux_status(&s->before_policy)&&!sl_linux_sample_selinux_status(&s->after_policy));
}
int main(void){
 sl_linux_policy_parse root={0};sl_linux_selinux_snapshot snapshot={0};
 root.snapshot_original=&snapshot;assert(sl_linux_parse_policy_snapshot(&root)==-1&&!calls&&!root.parser.attempted);
 snapshot_ready(&snapshot);memset(&root,0,sizeof root);snapshot.eof=0;root.snapshot_original=&snapshot;
 assert(sl_linux_parse_policy_snapshot(&root)==-1&&!calls);
 snapshot_ready(&snapshot);memset(&root,0,sizeof root);root.snapshot_original=&snapshot;
 assert(sl_linux_parse_policy_snapshot(&root)==-47&&calls==1&&root.returned&&root.actual_snapshot==&snapshot);
 assert(root.parser.body==snapshot.actual_body&&root.parser.extent==snapshot.length&&root.parser.init_result==-47&&!root.status_result);
 assert(sl_linux_parse_policy_snapshot(&root)==-1&&calls==1&&root.parser.init_result==-47);
 snapshot_ready(&snapshot);memset(&root,0,sizeof root);root.snapshot_original=&snapshot;parse_result=0;change_status=1;
 assert(sl_linux_parse_policy_snapshot(&root)==-1&&calls==2&&root.parser.result==0&&root.status_result==-1&&root.after_parse.sequence_after==4);
 snapshot_ready(&snapshot);memset(&root,0,sizeof root);root.snapshot_original=&snapshot;parse_result=-47;
 assert(sl_linux_parse_policy_snapshot(&root)==-47&&calls==3&&root.parser.init_result==-47&&root.status_result==-1);
 return 0;
}
#endif