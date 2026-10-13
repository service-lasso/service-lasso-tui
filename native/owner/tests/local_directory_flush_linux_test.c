/* UNRUN actual LOCAL collector; substituted IO is not durable/native proof. */
#include "../custody_linux.h"
#include <assert.h>
#include <errno.h>
#include <sys/syscall.h>
static unsigned mode,sends,flushes,reads;
long sl_linux_call6(sl_linux_call *c,const char *source,long n,long a,long b,long d,long e,long f,long g){(void)f;(void)g;c->source=source;c->number=n;c->arguments[0]=a;atomic_store(&c->entered,1);long r=0;
 if(n==SYS_read){unsigned char *p=(void*)b;if(d==8){for(unsigned i=0;i<8;i++)p[i]=i?0:1;}else {assert(d==1);p[0]=42;}r=d;}
 else if(n==SYS_pwrite64){assert(a==40&&d==1&&*(unsigned char*)b==42);r=1;}
 else if(n==SYS_fsync){flushes++;assert(a==40||a==30);r=(mode==1&&a==30)||(mode==2&&a==40)?-EIO:0;}
 else if(n==SYS_pread64){reads++;assert(a==40&&d==1);if(e==1){*(unsigned char*)b=99;r=mode==4?1:mode==5?-EIO:0;}else{*(unsigned char*)b=mode==3?43:42;r=1;}}
 else if(n==SYS_sendto){sends++;assert(d==8);r=8;}else assert(0);c->result=r;atomic_store(&c->returned,1);return r;
}
#include "../custody_linux.c"
typedef struct {sl_linux_local_collection collection;unsigned char body,readback;sl_linux_call header[8],receive[1],write[1],read[1],send[8];int birth,storage,directory;} fixture;
int main(void){fixture cases[6]={0};for(mode=0;mode<6;mode++){fixture *f=&cases[mode];sl_linux_local_collection *o=&f->collection;o->birth_original=&f->birth;o->storage_original=&f->storage;o->directory_original=&f->directory;o->endpoint_fd=50;o->storage_fd=40;o->directory_fd=30;o->body=&f->body;o->readback=&f->readback;o->capacity=1;o->receive_header=(sl_linux_custody_io){f->header,8,0,0};o->receive_body=(sl_linux_custody_io){f->receive,1,0,0};o->write_body=(sl_linux_custody_io){f->write,1,0,0};o->read_body=(sl_linux_custody_io){f->read,1,0,0};o->send_reply=(sl_linux_custody_io){f->send,8,0,0};sends=flushes=reads=0;
 assert(!sl_linux_prepare_local_collection(o));o->directory_fd=999;o->directory_original=0;long r=sl_linux_collect_local_representation(o);assert(o->returned&&o->prepared_directory==30&&o->prepared_directory_original==&f->directory&&o->prepared_write_body.completed==1);
 if(!mode)assert(r==8&&sends==1&&flushes==2&&reads==2&&o->readback_equal&&o->readback_eof.result==0);
 else {assert(r<0&&!sends);if(mode==1)assert(r==-EIO&&flushes==2&&!reads&&!o->flush.result&&o->directory_flush.result==-EIO);if(mode==2)assert(flushes==1&&!atomic_load(&o->directory_flush.entered));if(mode==3)assert(flushes==2&&reads==2&&!o->readback_equal);if(mode==4)assert(o->readback_eof.result==1&&o->eof_byte==99&&reads==2&&!o->readback_equal);if(mode==5)assert(r==-EIO&&o->readback_eof.result==-EIO&&reads==2&&!o->readback_equal);}
 unsigned prior=sends;assert(sl_linux_collect_local_representation(o)==-1&&sends==prior);
 }return 0;}