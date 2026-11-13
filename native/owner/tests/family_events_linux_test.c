/* UNRUN substituted raw and pthread calls; not native family-zero evidence. */
#include "../candidate_target_linux.h"
#include "../family_events_linux.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/magic.h>
static unsigned calls,reads,closes,joins,creates;static int mode,join_result;static void *join_pointer;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 (void)a0;(void)a3;(void)a5;calls++;c->source=source;c->number=number;atomic_store(&c->entered,1);
 if(number==SYS_openat2)c->result=mode==9?0:40;
 else if(number==SYS_fstatfs){((struct statfs*)a1)->f_type=CGROUP2_SUPER_MAGIC;c->result=0;}
 else if(number==SYS_statx){*(struct statx*)a4=(struct statx){.stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID,.stx_mode=S_IFREG|0444,.stx_ino=99,.stx_mnt_id=mode==8?8:7,.stx_dev_minor=28};c->result=0;}
 else if(number==SYS_read){assert(a2==1);static const unsigned char original[]="populated 0\nfrozen 0\n";unsigned at=reads++;c->result=at<21?1:0;if(mode==4&&at==20)c->result=0;if(mode==5&&at==21)c->result=1;if(mode==6&&at==5)c->result=-EIO;if(c->result==1){*(unsigned char*)a1=at<21?original[at]:'x';if(mode==1&&at==10)*(unsigned char*)a1='1';if(mode==2&&at==19)*(unsigned char*)a1='1';if(mode==3&&at==10)*(unsigned char*)a1='2';}}
 else if(number==SYS_close){closes++;c->result=mode==7?-EINTR:0;}
 else assert(0);atomic_store(&c->returned,1);return c->result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *l,int fd){l->attempted=1;l->original_fd=l->held_fd=fd;if(mode==9){assert(fd==0);l->held_fd=40;l->duplicate.result=40;atomic_init(&l->duplicate.entered,1);atomic_init(&l->duplicate.returned,1);l->close_original.result=-EINTR;atomic_init(&l->close_original.entered,1);atomic_init(&l->close_original.returned,1);return -EINTR;}return fd;}
static int fixture_create(pthread_t *t,const pthread_attr_t *a,void *(*f)(void*),void *v){(void)a;(void)f;(void)v;creates++;*t=(pthread_t)1;return 0;}
static int fixture_join(pthread_t t,void **out){(void)t;joins++;if(!join_result)*out=join_pointer;return join_result;}
#define pthread_create fixture_create
#define pthread_tryjoin_np fixture_join
#include "../family_events_linux.c"
#undef pthread_create
#undef pthread_tryjoin_np
static void target(sl_linux_candidate_target *t){t->returned=t->bound=1;t->target_fd=30;t->target_fs.f_type=CGROUP2_SUPER_MAGIC;t->target_state=(struct statx){.stx_mode=S_IFDIR|0700,.stx_mnt_id=7,.stx_dev_minor=28};}
int main(void){sl_linux_family_events cases[10]={0};sl_linux_candidate_target roots[10]={0};for(mode=0;mode<10;mode++){
 target(&roots[mode]);sl_linux_family_events *o=&cases[mode];o->target_original=&roots[mode];calls=reads=closes=0;
 long r=sl_linux_observe_family_events(o);assert(o->returned&&closes==1&&o->close_attempted&&o->close_returned);
 if(mode<3){assert(!r&&o->framing_complete&&o->eof&&o->extent==21&&reads==22&&calls==26);assert(o->populated==(mode==1)&&o->frozen==(mode==2));}
 else assert(r<0);if(mode==4)assert(o->eof&&o->extent==20);if(mode==5)assert(o->excess&&!o->eof&&o->extent==22);if(mode==6)assert(o->primary_result==-EIO&&o->reads[5].count==-EIO);if(mode==7)assert(!o->primary_result&&o->close_result==-EINTR&&o->close.result==-EINTR);if(mode==8)assert(!reads);if(mode==9)assert(!reads&&o->raw_fd==0&&o->fd==40&&o->primary_result==-EINTR&&o->lift.close_original.result==-EINTR&&!o->close_result);
 unsigned before=calls;assert(sl_linux_observe_family_events(o)==-EINVAL&&calls==before);
 }
 sl_linux_family_worker worker={0};sl_linux_family_join pending={0},complete={0},repeat={0};sl_linux_candidate_target root={0};target(&root);worker.observation.target_original=&root;
 mode=0;calls=reads=closes=0;assert(!sl_linux_start_family_worker(&worker)&&creates==1&&worker.create_returned);assert(sl_linux_start_family_worker(&worker)==-EINVAL&&creates==1);
 pending.worker_original=&worker;join_result=EBUSY;assert(sl_linux_observe_family_join(&pending)==EBUSY&&!worker.joined_by&&pending.returned&&pending.native_result==EBUSY);
 assert(family_worker_body(&worker)==&worker.observation);complete.worker_original=&worker;join_result=0;join_pointer=&worker.observation;assert(!sl_linux_observe_family_join(&complete)&&worker.joined_by==&complete&&complete.joined_result==&worker.observation&&worker.observation.eof);
 repeat.worker_original=&worker;unsigned before=joins;assert(sl_linux_observe_family_join(&repeat)==-EINVAL&&joins==before);
 return 0;}