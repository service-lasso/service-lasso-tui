/* UNRUN component fixture. Controlled IO/thread operations are not seal,
 * native EOF, actual worker scheduling, or independent FINAL ACK evidence. */
#define SERVICE_LASSO_OWNED_SEPOL 1
#include "../public_controller_linux.h"
#include "../public_startup_linux.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
typedef struct episode {sl_linux_public_controller controller;sl_linux_public_startup startup;sl_linux_final_record record;sl_linux_call calls[16];unsigned char bytes[35],wire[35],body[3];sl_linux_schedule_wake wake;sl_linux_archive_reader_join join;} episode;
static episode cases[12];static episode *current;static unsigned mode,effects,joins;
long sl_linux_call6(sl_linux_call *c,const char *source,long nr,long a,long b,long d,long e,long f,long g){
 effects++;assert((nr==SYS_pread64||nr==SYS_statx)&&a==9);c->source=source;c->number=nr;c->arguments[0]=a;c->arguments[1]=b;c->arguments[2]=d;c->arguments[3]=e;c->arguments[4]=f;c->arguments[5]=g;atomic_store(&c->entered,1);
 long result=0;if(nr==SYS_statx){*(struct statx*)f=current->startup.final_reader.identity;((struct statx*)f)->stx_size=35;if(c==&current->controller.final_reader_task.before&&mode>=10)result=-EIO;if(c==&current->controller.final_reader_task.after){if(mode==7)result=-EIO;if(mode==8||mode==11)result=-EPERM;if(mode==9)((struct statx*)f)->stx_size++;}}
 else if(mode==3||mode==8)result=-EIO;else if(e<35){unsigned long n=35-(unsigned long)e;if(n>(unsigned long)d)n=d;if(mode==1&&n>7)n=7;if(mode==2)n=0;if(n)memcpy((void*)b,current->wire+e,n);result=(long)n;}
 c->result=result;atomic_store(&c->returned,1);return result;
}
static int controlled_create(pthread_t *thread,const pthread_attr_t *attr,void *(*body)(void*),void *arg){(void)attr;*thread=(pthread_t)1;assert(body(arg)==arg);return 0;}
static int controlled_join(pthread_t thread,void **out){assert(thread==(pthread_t)1);joins++;*out=mode==5?(void*)&current->record:(void*)&current->controller.final_reader_task;return 0;}
long sl_linux_schedule_notify(sl_linux_schedule_wake *w){assert(w==&current->wake);atomic_store(&w->returned,1);return 0;}
long sl_linux_schedule_joined_notification(sl_linux_schedule_wake *w,const void *source){assert(w==&current->wake&&source==&current->controller.final_reader_task);return mode==6?-EPERM:0;}
#define pthread_create controlled_create
#define pthread_tryjoin_np controlled_join
#include "../shared_range_archive_linux.c"
#include "../archive_readback_linux.c"
int main(void){for(mode=0;mode<12;mode++){
 episode *e=current=&cases[mode];sl_linux_archive_readback *r=&e->controller.final_reader_task;sl_linux_final_capture *f=&e->controller.final_capture;
 e->controller.startup_original=&e->startup;e->controller.final_reader_joins=&e->join;e->controller.final_reader_join_capacity=1;r->controller_original=&e->controller;r->view_original=&e->startup.final_reader;r->capture_original=f;r->bytes=e->bytes;r->capacity=35;r->reads=e->calls;r->read_capacity=16;r->terminal_wake=&e->wake;e->wake.source_original=r;
 for(unsigned i=0;i<16;i++){atomic_init(&e->calls[i].entered,0);atomic_init(&e->calls[i].returned,0);}
 e->startup.final_reader.returned=1;e->startup.final_reader.fd=9;f->returned=1;f->readback_equal=1;f->actual_records=&e->record;f->count=f->actual_capacity=1;f->extent=35;
 e->startup.final_reader.identity.stx_mask=STATX_TYPE|STATX_INO|STATX_MNT_ID|STATX_SIZE;e->startup.final_reader.identity.stx_mode=S_IFREG|0600;e->startup.final_reader.identity.stx_ino=7;e->startup.final_reader.identity.stx_mnt_id=11;
 e->body[0]='a';e->body[1]='b';e->body[2]='c';e->record.body=e->body;e->record.length=3;memcpy(e->wire,e->record.header,32);memcpy(e->wire+32,e->body,3);if(mode==4)e->wire[34]='x';
 unsigned before=effects;assert(!sl_linux_prepare_archive_readback(r));assert(!sl_linux_start_archive_readback(r));assert(atomic_load(&r->body_returned)&&r->create_returned);
 e->join.reader_original=r;unsigned oldjoins=joins;long result=sl_linux_join_archive_readback(&e->join);assert(joins==oldjoins+1&&r->joined_by==&e->join&&!e->join.native_result&&e->join.interpretation_returned);
 if(mode<2)assert(!result&&r->readback_equal&&r->extent==35&&r->read_count==(mode?6:2));
 if(mode==2||mode==4)assert(result==-EIO&&!r->readback_equal);
 if(mode==3)assert(result==-EIO&&e->calls[0].result==-EIO&&r->read_count==1);
 if(mode==5)assert(result==-EINVAL&&r->readback_equal&&e->join.output==&e->record);
 if(mode==6)assert(result==-EPERM&&r->readback_equal);
 if(mode==7)assert(result==-EIO&&!r->primary_result&&r->after_result==-EIO&&r->readback_equal);
 if(mode==8)assert(result==-EIO&&r->primary_result==-EIO&&r->after_result==-EPERM&&r->after.result==-EPERM&&!r->readback_equal);
 if(mode==9)assert(result==-EINVAL&&!r->primary_result&&r->after_result==-EINVAL&&r->after_state.stx_size==36&&r->readback_equal);
 if(mode>=10)assert(result==-EIO&&r->primary_result==-EIO&&r->before.result==-EIO&&r->after_result==(mode==11?-EPERM:0)&&!r->read_count&&!r->extent&&!r->readback_equal);
 assert(atomic_load(&r->before.returned)&&atomic_load(&r->after.returned));
 assert(effects>before);oldjoins=joins;before=effects;assert(sl_linux_join_archive_readback(&e->join)==-EINVAL&&joins==oldjoins);sl_linux_archive_reader_join late={.reader_original=r};assert(sl_linux_join_archive_readback(&late)==-EINVAL&&!late.attempted&&joins==oldjoins&&effects==before);assert(sl_linux_start_archive_readback(r)==-EINVAL&&effects==before);
 }return 0;}
