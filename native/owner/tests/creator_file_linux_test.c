/* UNRUN: actual source body with synthetic native returns, not OS/file proof. */
#if defined(__linux__) && defined(__x86_64__)
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../creator_file_linux.h"
static unsigned mode,calls,reads,clocks;
static sl_linux_deadline test_root;static sl_linux_deadline_source test_source;
static sl_linux_deadline_sequence test_sequence;static sl_linux_deadline_observation test_observations[12];
static const unsigned char test_operation=1;
static const unsigned char bytes[4]={'E','L','F','!'};
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 c->source=source;c->number=number;c->arguments[0]=a0;c->arguments[1]=a1;c->arguments[2]=a2;
 c->arguments[3]=a3;c->arguments[4]=a4;c->arguments[5]=a5;atomic_store(&c->entered,1);
 long result=0;
 if(number==SYS_clock_gettime){clocks++;assert(a0==CLOCK_MONOTONIC);struct timespec *t=(void*)a1;*t=(struct timespec){1,0};if(mode==4&&clocks==3)t->tv_sec=2;c->result=0;atomic_store(&c->returned,1);return 0;}
 calls++;assert(a0==5);
 if(number==SYS_statx){
  struct statx *s=(void*)a4;s->stx_mask=(unsigned)a3;s->stx_mode=S_IFREG|0600;s->stx_size=4;s->stx_ino=19;
  if(mode==3&&reads)s->stx_ino++;
 }else{
  assert(number==SYS_pread64);
  if(a3==4){assert(a2==1);result=mode==2?1:0;}
  else{reads++;result=mode==1?-EINTR:(a2>2?2:a2);if(result>0)memcpy((void*)a1,bytes+a3,(size_t)result);}
 }
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "../deadline_linux.c"
#include "../creator_file_linux.c"
static void setup(sl_linux_creator_file *o,sl_linux_local_installation *s,sl_linux_entry *entry,
 unsigned char *body,sl_linux_creator_read *rows){
 memset(o,0,sizeof(*o));memset(s,0,sizeof(*s));memset(entry,0,sizeof(*entry));memset(body,0,8);memset(rows,0,3*sizeof(*rows));
 mode=calls=reads=clocks=0;
 memset(&test_root,0,sizeof(test_root));memset(&test_sequence,0,sizeof(test_sequence));memset(test_observations,0,sizeof(test_observations));
 test_source=(sl_linux_deadline_source){&test_operation,sizeof(test_operation),1000000000UL};
 assert(!sl_linux_construct_deadline(&test_root,&test_source));
 test_sequence.root_original=&test_root;test_sequence.observations_original=test_observations;test_sequence.capacity=12;
 assert(!sl_linux_prepare_deadline_sequence(&test_sequence));o->sequence_original=&test_sequence;s->attempted=s->returned=1;s->entry_original=entry;s->executable_fd=5;s->proc_fd=4;
 s->executable_lift.attempted=1;s->executable_lift.original_fd=s->executable_lift.held_fd=5;
 s->open_executable.number=SYS_openat;s->open_executable.result=5;s->open_executable.arguments[0]=4;
 s->open_executable.arguments[1]=(long)"exe";
 s->open_executable.arguments[2]=O_RDONLY|O_NONBLOCK|O_CLOEXEC;
 atomic_init(&s->open_executable.entered,1);atomic_init(&s->open_executable.returned,1);
 s->executable_identity.number=SYS_statx;s->executable_identity.arguments[0]=5;
 s->executable_identity.arguments[1]=(long)"";s->executable_identity.arguments[2]=AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW;
 s->executable_identity.arguments[3]=STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 s->executable_identity.arguments[4]=(long)&s->executable_state;
 atomic_init(&s->executable_identity.entered,1);atomic_init(&s->executable_identity.returned,1);
 o->installation_original=s;o->body=body;o->body_capacity=8;o->reads=rows;o->read_capacity=3;
}
int main(void){
 sl_linux_creator_file o;sl_linux_local_installation s;sl_linux_entry entry;unsigned char body[8];sl_linux_creator_read rows[3];
 setup(&o,&s,&entry,body,rows);assert(!sl_linux_capture_creator_file(&o)&&o.complete&&o.extent==4&&o.count==2&&calls==5);
 assert(!memcmp(body,bytes,4)&&rows[0].offset==0&&rows[1].offset==2&&rows[0].raw==2&&rows[1].raw==2);
 sl_linux_creator_file saved=o;assert(sl_linux_capture_creator_file(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o))&&calls==5);
 setup(&o,&s,&entry,body,rows);mode=1;assert(sl_linux_capture_creator_file(&o)==-EINTR&&o.count==1&&!o.complete&&rows[0].raw==-EINTR);
 setup(&o,&s,&entry,body,rows);mode=2;assert(sl_linux_capture_creator_file(&o)==-EIO&&o.eof_raw==1&&!o.complete);
 setup(&o,&s,&entry,body,rows);mode=3;assert(sl_linux_capture_creator_file(&o)==-EINVAL&&o.after.stx_ino!=o.before.stx_ino&&!o.complete);
 setup(&o,&s,&entry,body,rows);o.read_capacity=1;assert(sl_linux_capture_creator_file(&o)==-ENOSPC&&o.extent==2&&o.count==1&&!o.complete);
 setup(&o,&s,&entry,body,rows);o.body=(void*)&entry;o.body_capacity=sizeof(entry);saved=o;sl_linux_entry entry_saved=entry;
 assert(sl_linux_capture_creator_file(&o)==-EINVAL&&!calls&&!memcmp(&o,&saved,sizeof(o))&&!memcmp(&entry,&entry_saved,sizeof(entry)));
 setup(&o,&s,&entry,body,rows);s.executable_path=(char*)body;s.executable_capacity=8;saved=o;
 assert(sl_linux_capture_creator_file(&o)==-EINVAL&&!calls&&!memcmp(&o,&saved,sizeof(o))&&!body[0]);
 setup(&o,&s,&entry,body,rows);mode=4;assert(sl_linux_capture_creator_file(&o)==1&&!o.complete&&o.before_boundary.after->expired&&o.before_call.result==0&&atomic_load(&o.before_call.returned));
 setup(&o,&s,&entry,body,rows);s.executable_path=(char*)test_observations;s.executable_capacity=sizeof(test_observations);saved=o;
 sl_linux_deadline_observation clock_saved[12];memcpy(clock_saved,test_observations,sizeof(clock_saved));
 sl_linux_deadline_sequence sequence_saved=test_sequence;unsigned clocks_before=clocks;
 assert(sl_linux_capture_creator_file(&o)==-EINVAL&&!calls&&clocks==clocks_before&&
 !memcmp(&o,&saved,sizeof(o))&&!memcmp(clock_saved,test_observations,sizeof(clock_saved))&&
 !memcmp(&sequence_saved,&test_sequence,sizeof(sequence_saved)));
 setup(&o,&s,&entry,body,rows);s.payload.prepared_locator=(char*)&test_sequence;s.payload.directory_locator="x";s.payload.directory_capacity=1;saved=o;
 sequence_saved=test_sequence;clocks_before=clocks;
 assert(sl_linux_capture_creator_file(&o)==-EINVAL&&!calls&&clocks==clocks_before&&
 !memcmp(&o,&saved,sizeof(o))&&!memcmp(&sequence_saved,&test_sequence,sizeof(sequence_saved)));
 setup(&o,&s,&entry,body,rows);entry.birth.bootstrap=(void*)body;saved=o;entry_saved=entry;clocks_before=clocks;
 assert(sl_linux_capture_creator_file(&o)==-EINVAL&&!calls&&clocks==clocks_before&&
 !memcmp(&o,&saved,sizeof(o))&&!memcmp(&entry,&entry_saved,sizeof(entry))&&!body[0]);
 static sl_linux_bootstrap bootstrap;
 setup(&o,&s,&entry,body,rows);memset(&bootstrap,0,sizeof(bootstrap));entry.birth.bootstrap=&bootstrap;
 bootstrap.authority.credentials_before.groups=(void*)test_observations;bootstrap.authority.credentials_before.group_capacity=1;
 saved=o;memcpy(clock_saved,test_observations,sizeof(clock_saved));clocks_before=clocks;
 assert(sl_linux_capture_creator_file(&o)==-EINVAL&&!calls&&clocks==clocks_before&&
 !memcmp(&o,&saved,sizeof(o))&&!memcmp(clock_saved,test_observations,sizeof(clock_saved)));
 return 0;
}
#endif
