/* UNRUN source model: caller-settled borrowers are a fixture, not native proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../public_startup_linux.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
static unsigned calls;static int descriptors[8];static long outcomes[8];
long sl_linux_call6(sl_linux_call *c,const char *s,long n,long a,long b,long d,long e,long f,long g){
 (void)b;(void)d;(void)e;(void)f;(void)g;assert(n==SYS_close&&calls<8);c->source=s;c->number=n;c->arguments[0]=a;atomic_store(&c->entered,1);descriptors[calls]=a;c->result=outcomes[calls++];atomic_store(&c->returned,1);return c->result;
}
#include "../cgroup_control_close_linux.c"
static void setup(sl_linux_public_startup *s,sl_linux_cgroup_control_label *r){
 memset(s,0,sizeof *s);memset(r,0,4*sizeof *r);memset(outcomes,0,sizeof outcomes);calls=0;s->attempted=s->returned=1;s->actual_controls=r;s->actual_control_capacity=4;s->control_count=1;
 for(unsigned i=0;i<4;i++){r[i].attempted=r[i].returned=1;r[i].raw_fd=r[i].fd=20+i;r[i].open.source="actual.open";r[i].open.number=SYS_openat2;r[i].open.result=20+i;atomic_init(&r[i].open.returned,1);r[i].lift.attempted=1;r[i].lift.original_fd=r[i].lift.held_fd=20+i;atomic_init(&r[i].lift.close_original.entered,0);}
}
int main(void){
 sl_linux_public_startup s;sl_linux_cgroup_control_label r[4];setup(&s,r);outcomes[0]=-EINTR;outcomes[3]=-EIO;
 assert(sl_linux_close_cgroup_controls(&s)==-EINTR&&calls==4&&s.control_close_primary==r&&r[3].close_held.result==-EIO);
 assert(sl_linux_close_cgroup_controls(&s)==-1&&calls==4);
 setup(&s,r);outcomes[0]=-EINTR;r[2].returned=0;assert(sl_linux_close_cgroup_controls(&s)==-EINTR&&calls==3&&s.control_close_primary==r);
 setup(&s,r);r[1].attempted=0;r[2].returned=0;
 assert(sl_linux_close_cgroup_controls(&s)==-1&&calls==2&&descriptors[0]==20&&descriptors[1]==23&&!r[1].closes_attempted&&!r[2].closes_attempted);
 setup(&s,r);r[0].raw_fd=r[0].open.result=0;r[0].fd=-1;r[0].lift.original_fd=0;r[0].lift.held_fd=40;r[0].lift.duplicate.result=40;atomic_init(&r[0].lift.duplicate.returned,1);r[0].lift.close_original.result=-EINTR;atomic_store(&r[0].lift.close_original.entered,1);atomic_init(&r[0].lift.close_original.returned,1);
 assert(sl_linux_close_cgroup_controls(&s)==-EINTR&&calls==4&&descriptors[0]==40&&r[0].close_primary==&r[0].lift.close_original&&!r[0].close_raw_attempted);
 setup(&s,r);r[0].raw_fd=r[0].open.result=1;r[0].lift.original_fd=1;r[0].lift.held_fd=-1;
 assert(sl_linux_close_cgroup_controls(&s)==0&&calls==4&&descriptors[0]==1&&r[0].close_raw_attempted);
 setup(&s,r);r[0].open.source=0;r[0].open.number=0;assert(sl_linux_close_cgroup_controls(&s)==0&&calls==3);
 return 0;
}
#endif