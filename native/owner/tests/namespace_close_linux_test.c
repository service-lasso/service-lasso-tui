/* UNRUN controlled close traversal. Fixture explicitly models a caller that has
 * already settled borrowers; no native last-borrower/FINAL proof is supplied. */
#if defined(__linux__) && defined(__x86_64__)
#include "../namespace_parent_linux.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
static unsigned calls;static long outcomes[8];static int descriptors[8];
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 (void)b;(void)d;(void)e;(void)f;(void)g;assert(n==SYS_close&&calls<8);c->source=s;c->number=n;c->arguments[0]=a;
 atomic_store(&c->entered,1);descriptors[calls]=(int)a;c->result=outcomes[calls++];atomic_store(&c->returned,1);return c->result;
}
#include "../namespace_close_linux.c"
static void setup(sl_linux_parent_namespace *o){
 memset(o,0,sizeof *o);memset(outcomes,0,sizeof outcomes);memset(descriptors,0,sizeof descriptors);calls=0;o->attempted=o->returned=1;
 for(unsigned i=0;i<6;i++){sl_linux_namespace_view *v=&o->views[i];v->acquire.result=v->raw_fd=v->fd=20+i;atomic_init(&v->acquire.returned,1);v->lift.attempted=1;v->lift.original_fd=v->lift.held_fd=20+i;}
}
int main(void){
 sl_linux_parent_namespace o;setup(&o);outcomes[0]=-EINTR;outcomes[4]=-EIO;
 assert(sl_linux_close_namespace_views(&o)==-EINTR&&calls==6&&o.closes_returned&&o.close_primary==&o.views[0].close_held);
 for(unsigned i=0;i<6;i++)assert(descriptors[i]==20+(int)i&&o.views[i].close_held_attempted&&!o.views[i].close_raw_attempted);
 assert(o.views[4].close_held.result==-EIO);assert(sl_linux_close_namespace_views(&o)<0&&calls==6);
 setup(&o);sl_linux_namespace_view *v=&o.views[0];v->raw_fd=v->acquire.result=0;v->fd=-1;v->lift.original_fd=0;v->lift.held_fd=40;
 v->lift.duplicate.result=40;atomic_init(&v->lift.duplicate.returned,1);v->lift.close_original.result=-EINTR;atomic_init(&v->lift.close_original.entered,1);atomic_init(&v->lift.close_original.returned,1);
 assert(sl_linux_close_namespace_views(&o)==-EINTR&&o.close_primary==&v->lift.close_original&&calls==6&&descriptors[0]==40&&!v->close_raw_attempted&&v->lift.close_original.result==-EINTR);
 setup(&o);v=&o.views[0];v->raw_fd=v->acquire.result=1;v->fd=-1;v->lift.original_fd=1;v->lift.held_fd=-1;v->lift.duplicate.result=-EMFILE;atomic_init(&v->lift.duplicate.returned,1);
 assert(sl_linux_close_namespace_views(&o)==0&&calls==6&&descriptors[0]==1&&v->close_raw_attempted&&!v->close_held_attempted);
 setup(&o);v=&o.views[3];v->acquire.result=-EACCES;v->raw_fd=-EACCES;assert(sl_linux_close_namespace_views(&o)==0&&calls==5&&!v->close_held_attempted&&!v->close_raw_attempted);
 setup(&o);o.views[1].raw_fd=99;assert(sl_linux_close_namespace_views(&o)<0&&calls==5&&o.close_unmapped_original==&o.views[1]);
 setup(&o);o.returned=0;assert(sl_linux_close_namespace_views(&o)<0&&!calls&&!o.closes_attempted);
 return 0;
}
#endif