/* Production directory/declaration/lift bodies with synthetic native clocks.
 * UNRUN: no real parent process, policy, descriptor or provider qualification. */
#include "../failed_parent_directory_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
static unsigned mode,calls,ends;
int sl_linux_deadline_sequence_excludes_output(const sl_linux_deadline_sequence *s,const void *p,size_t n){return s&&p&&n;}
long sl_linux_begin_deadline_boundary(sl_linux_deadline_boundary *b,sl_linux_deadline_sequence *s){b->sequence=s;b->attempted=b->ready=1;return 0;}
long sl_linux_end_deadline_boundary(sl_linux_deadline_boundary *b){b->after_attempted=b->returned=1;ends++;return (mode==2&&ends==1)||(mode==3&&ends==4)?1:0;}
int sl_linux_deadline_boundary_matches_original(const sl_linux_deadline_boundary *b){return b->returned;}
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 calls++;c->source=source;c->number=number;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;long raw=0;
 if(number==SYS_openat2){assert(!strcmp((char *)b,"/archive"));raw=mode==1?-EACCES:0;}
 else if(number==SYS_fcntl){assert(a==0&&b==F_DUPFD_CLOEXEC&&d==3);raw=7;}
 else if(number==SYS_close){assert(a==0);raw=mode==4?-EINTR:0;}
 else if(number==SYS_fstatfs)((struct statfs *)b)->f_type=0xEF53;
 else if(number==SYS_statx){struct statx *v=(struct statx *)f;v->stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID;v->stx_mode=S_IFDIR;v->stx_dev_major=1;v->stx_dev_minor=2;v->stx_ino=mode==5?99:3;v->stx_mnt_id=4;}
 else assert(0);
 c->result=raw;atomic_store(&c->entered,1);atomic_store(&c->returned,1);return raw;
}
static void put64(unsigned char *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(8*i));}
int main(void){
 _Alignas(sl_linux_deadline_observation) unsigned char body[512]={0};const char *meaning="service-lasso.archive-directory-object.linux.v1";
 memcpy(body+24,meaning,strlen(meaning));memcpy(body+128,"evidence",8);memcpy(body+200,"/archive",8);
 put64(body+16,8);put64(body+160,1);put64(body+168,2);put64(body+176,3);put64(body+184,4);put64(body+192,8);
 sl_candidate_record evidence={.field_count=7},declaration={.field_count=3};
 evidence.fields[6]=(sl_candidate_span){body+128,8};declaration.fields[0]=(sl_candidate_span){body,24};declaration.fields[1]=(sl_candidate_span){body+24,strlen(meaning)};declaration.fields[2]=(sl_candidate_span){body+160,48};
 sl_candidate_profile p={.source=body,.source_extent=sizeof(body),.original={body,sizeof(body)},.returned=1};
 memcpy(body+208,body+160,48);sl_candidate_record declarations[2]={declaration,declaration};declarations[1].fields[2]=(sl_candidate_span){body+208,48};
 p.actual_rows[0]=&evidence;p.actual_rows[3]=declarations;p.actual_capacities[0]=p.counts[0]=1;p.actual_capacities[3]=p.counts[3]=2;
 sl_linux_deadline_observation observations[10]={0};sl_linux_deadline_sequence sequence={.actual_observations=observations,.actual_capacity=10};
 sl_linux_outer_birth birth={.prepared=1};
 for(mode=0;mode<8;mode++){
  put64(body+232,mode==6?99:4);
  calls=ends=0;sl_linux_failed_parent_directory o={0};unsigned char path[32]={0};sl_linux_archive_declaration rows[2]={0};
  long result=sl_linux_acquire_failed_parent_directory(&o,&p,&birth,&sequence,path,sizeof(path),rows,mode==7?1:2);
  assert(o.returned&&o.candidate_original==&p&&o.path_original==path);
  if(mode==0){
   assert(!result&&calls==5&&o.raw_fd==0&&o.fd==7&&o.identity.stx_ino==3);assert(sl_linux_failed_parent_directory_matches_original(&o));
   o.identity_query.arguments[0]=8;assert(!sl_linux_failed_parent_directory_matches_original(&o));o.identity_query.arguments[0]=7;
   path[1]='x';assert(!sl_linux_failed_parent_directory_matches_original(&o));path[1]='a';assert(sl_linux_failed_parent_directory_matches_original(&o));
   sl_linux_failed_parent_directory original=o;sl_linux_outer_birth original_birth=birth;
   memcpy(&birth,"/archive",9);o.path_original=(unsigned char *)&birth;o.path_capacity=sizeof(birth);o.open.arguments[1]=(long)&birth;
   sl_linux_outer_birth alias_birth=birth;sl_linux_failed_parent_directory alias_o=o;
   assert(!sl_linux_failed_parent_directory_matches_original(&o)&&!memcmp(&o,&alias_o,sizeof(o))&&!memcmp(&birth,&alias_birth,sizeof(birth)));
   o=original;birth=original_birth;
   sequence.actual_observations=(sl_linux_deadline_observation *)body;
   unsigned char original_body[sizeof(body)];memcpy(original_body,body,sizeof(body));
   assert(!sl_linux_failed_parent_directory_matches_original(&o)&&!memcmp(body,original_body,sizeof(body)));
   sequence.actual_observations=observations;assert(sl_linux_failed_parent_directory_matches_original(&o));
   memcpy(original_body,body,sizeof(body));
   sl_linux_deadline_sequence *alias_sequence=(sl_linux_deadline_sequence *)(body+256);
   memcpy(alias_sequence,&sequence,sizeof(sequence));o.sequence_original=alias_sequence;
   o.open_boundary.sequence=o.filesystem_boundary.sequence=o.identity_boundary.sequence=alias_sequence;
   o.lift.sequence_original=o.lift.duplicate_boundary.sequence=o.lift.close_boundary.sequence=alias_sequence;
   unsigned char alias_body[sizeof(body)];memcpy(alias_body,body,sizeof(body));alias_o=o;
   assert(!sl_linux_failed_parent_directory_matches_original(&o)&&!memcmp(body,alias_body,sizeof(body))&&!memcmp(&o,&alias_o,sizeof(o)));
   memcpy(body,original_body,sizeof(body));o=original;assert(sl_linux_failed_parent_directory_matches_original(&o));
  }
  if(mode==1)assert(result==-EACCES&&calls==1&&o.raw_fd==-1);
  if(mode==2)assert(result==-ETIMEDOUT&&calls==1&&o.raw_fd==0&&!o.lift.attempted);
  if(mode==3)assert(result==-ETIMEDOUT&&calls==4&&o.fd==7&&o.filesystem.f_type==0xEF53);
  if(mode==4)assert(result==-EINTR&&calls==3&&o.lift.lift.held_fd==7);
  if(mode==5)assert(result<0&&calls==5&&o.identity.stx_ino==99);
  if(mode==6)assert(result<0&&!calls&&o.declarations.count==2);
  if(mode==7)assert(result==-EOVERFLOW&&!calls&&o.declarations.count==1);
  sl_linux_failed_parent_directory saved=o;
  assert(sl_linux_acquire_failed_parent_directory(&o,&p,&birth,&sequence,path,sizeof(path),rows,2)<0&&!memcmp(&o,&saved,sizeof(o)));
 }
 sl_linux_failed_parent_directory o={0};unsigned char path[32]={0};sl_linux_archive_declaration rows[2]={0};sequence.actual_capacity=9;calls=0;
 assert(sl_linux_acquire_failed_parent_directory(&o,&p,&birth,&sequence,path,sizeof(path),rows,2)<0&&!o.attempted&&!calls);
 sequence.actual_capacity=10;unsigned char saved[sizeof(body)];memcpy(saved,body,sizeof(body));
 assert(sl_linux_acquire_failed_parent_directory(&o,&p,&birth,&sequence,body,32,rows,2)<0&&!o.attempted&&!memcmp(saved,body,sizeof(body)));
 return 0;
}
