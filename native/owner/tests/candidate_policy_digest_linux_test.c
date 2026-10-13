/* UNRUN source-known digest/partial/status cases; no loaded policy authority. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../candidate_policy_linux.h"
#include <assert.h>
#include <errno.h>
static int samples,secondary;static sl_linux_selinux_status *expected;
int sl_linux_sample_selinux_status(sl_linux_selinux_status_sample *s){assert(s->status_original==expected);samples++;s->attempted=s->returned=1;s->actual_status=s->status_original;s->result=secondary;return secondary;}
int sl_linux_equal_selinux_status(const sl_linux_selinux_status_sample *a,const sl_linux_selinux_status_sample *b){(void)a;(void)b;return 1;}
#include "../owned_sha256.c"
#include "../candidate_policy_linux.c"
typedef struct {sl_linux_candidate_policy binding;sl_candidate_profile candidate;sl_linux_policy_parse parse;sl_linux_selinux_snapshot snapshot;sl_owned_sha256_workspace workspace;unsigned char identity[32],kernel[32],body[3];} fixture;
static void prepare(fixture *f){
 static const unsigned char hash[32]={0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
 for(unsigned i=0;i<32;i++)f->identity[i]=hash[i];f->body[0]='a';f->body[1]='b';f->body[2]='c';
 f->binding.candidate_original=&f->candidate;f->binding.parse_original=&f->parse;f->binding.digest.workspace=&f->workspace;
 f->candidate.attempted=f->candidate.returned=1;f->candidate.policy_extent=3;f->candidate.policy_identity=(sl_candidate_span){f->identity,32};f->candidate.kernel_interpretation=(sl_candidate_span){f->kernel,32};
 f->parse.attempted=f->parse.returned=f->parse.parser.returned=1;f->parse.actual_snapshot=&f->snapshot;f->parse.parser.memory.prepared=f->parse.parser.memory.begin_returned=f->parse.parser.memory.finished=1;f->parse.parser.policy.sl_source_owner=&f->parse.parser.memory;
 f->snapshot.returned=f->snapshot.eof=1;f->snapshot.actual_body=f->body;f->snapshot.length=3;f->parse.parser.actual_body=f->body;f->parse.parser.actual_extent=3;
}
int main(void){
 fixture episodes[6]={0};
 for(unsigned i=0;i<6;i++){
  fixture *f=&episodes[i];prepare(f);expected=&f->snapshot.status;secondary=i>=4?-77:0;
  if(i==1)f->candidate.policy_extent=4;
  if(i==2||i==4)f->identity[0]^=1;
  if(i==3)f->binding.digest.workspace=NULL;
  int before=samples;long result=sl_linux_bind_candidate_policy(&f->binding);assert(f->binding.returned&&samples==before+1&&f->binding.actual_parse==&f->parse&&f->binding.digest.body==f->body);
  if(!i)assert(result==0&&f->binding.digest.returned&&f->binding.digest.actual_body==f->body&&f->workspace.input_original==f->body&&f->binding.declared_identity.bytes==f->identity);
  if(i==1)assert(result==-EINVAL&&f->binding.failure==SL_CANDIDATE_POLICY_EXTENT&&!f->binding.digest.attempted);
  if(i==2||i==4)assert(result==-EINVAL&&f->binding.failure==SL_CANDIDATE_POLICY_MISMATCH&&f->binding.mismatch_offset==0&&f->binding.digest.digest[0]==0xba);
  if(i==3)assert(result==-EINVAL&&f->binding.failure==SL_CANDIDATE_POLICY_DIGEST&&f->binding.digest_result==-EINVAL);
  if(i==4)assert(f->binding.status_result==-77&&f->binding.digest_result==0);
  if(i==5)assert(result==-77&&f->binding.failure==SL_CANDIDATE_POLICY_STATUS&&f->binding.digest_result==0);
  before=samples;assert(sl_linux_bind_candidate_policy(&f->binding)==-EINVAL&&samples==before);
 }
 return 0;
}