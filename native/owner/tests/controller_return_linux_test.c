/* UNRUN actual constructor return publication. Stage substitutions only;
 * no native acquisition, cleanup eligibility or successful birth is proved. */
#include "../public_startup_linux.h"
#include "../candidate_preexec_linux.h"
#include <assert.h>
#include <fcntl.h>
#include <sys/syscall.h>
static unsigned stages;
static int stage_ok(void){stages++;return 0;}
static int unreachable(void){assert(0);return -99;}
static long bind_fixture(sl_linux_public_startup *s){s->candidate_preexec_attempted=s->candidate_preexec_returned=1;s->candidate_preexec_result=0;return 0;}
static long call_fixture(sl_linux_call *c,const char *source,long n,long a,long b,long d,long e,long f,long g){(void)a;(void)b;(void)d;(void)e;(void)f;(void)g;c->source=source;c->number=n;atomic_store(&c->entered,1);c->result=n==SYS_openat?3:n==SYS_ioctl&&d==((long)(O_RDWR|O_NOCTTY|O_CLOEXEC))?4:0;atomic_store(&c->returned,1);return c->result;}
static sl_linux_candidate_preexec_shared shared;
static long prepare_fixture(sl_linux_candidate_preexec *p){p->shared=&shared;return 0;}
#define sl_linux_bind_controller_preexec bind_fixture
#define sl_linux_close_failed_target_before_birth(c) stage_ok()
#define sl_linux_close_failed_archive_before_birth(c) stage_ok()
#define sl_linux_observe_public_terminal(t) stage_ok()
#define sl_linux_call6 call_fixture
#define sl_linux_lift_internal_fd(l,f) (f)
#define sl_linux_prepare_private_output(...) stage_ok()
#define sl_linux_construct_stream_route(...) stage_ok()
#define sl_linux_prepare_entry_channels(...) stage_ok()
#define sl_linux_interpret_startup_socket_contexts(...) stage_ok()
#define sl_linux_interpret_startup_socket_creates(...) stage_ok()
#define sl_linux_interpret_startup_socket_uses(...) stage_ok()
#define sl_linux_prepare_preexec_authority(...) stage_ok()
#define sl_linux_prepare_candidate_preexec prepare_fixture
#define sl_linux_prepare_local_collection(...) unreachable()
#define sl_linux_prepare_final_capture(...) unreachable()
#define sl_linux_prepare_archive_readback(...) unreachable()
#define sl_linux_prepare_owner_channel(...) unreachable()
#define sl_linux_capture_prepare(...) unreachable()
#define sl_linux_prepare_forward_worker(...) unreachable()
#define sl_linux_prepare_raw_readers(...) unreachable()
#define sl_linux_prepare_service_observation(...) unreachable()
#define sl_prepare_private_arguments(...) unreachable()
#define sl_linux_scan_and_build_child_fd_map(...) unreachable()
#define sl_linux_birth_into_cgroup(...) unreachable()
#define sl_linux_release_parent_child_views(...) unreachable()
#define sl_linux_start_forward_worker(...) unreachable()
#define sl_linux_start_raw_readers(...) unreachable()
#define sl_linux_service_until_authority(...) unreachable()
#define sl_linux_continue_failed_sources(...) unreachable()
#define sl_linux_observe_parent_namespace(...) unreachable()
#define sl_linux_interpret_namespace_target(...) unreachable()
#define sl_linux_serve_owner_channel(...) unreachable()
#define pthread_create(...) unreachable()
#include "../public_controller_linux.c"
typedef struct {sl_linux_public_controller c;sl_linux_public_startup s;sl_linux_capture_worker reader;sl_linux_forward_worker forward;sl_linux_service_observation service;sl_linux_collector_join join;sl_linux_bootstrap bootstrap;sl_linux_candidate_preexec preexec;} fixture;
int main(void){fixture cases[2]={0};for(unsigned mode=0;mode<2;mode++){fixture *f=&cases[mode];sl_linux_public_controller *c=&f->c;c->startup_original=&f->s;f->s.controller_original=c;c->construction_result=999;stages=0;
 if(mode){c->readers=&f->reader;c->forwarders=&f->forward;c->initial_service=c->service_roster=&f->service;c->service_capacity=c->reader_count=1;c->collector_join_roster=&f->join;c->collector_join_capacity=1;f->join.controller_original=c;c->entry.public_streams_prepared=1;c->entry.birth.bootstrap=&f->bootstrap;c->candidate_preexec_original=&f->preexec;}
 long r=sl_linux_construct_contained_producer(c);assert(r==-1&&c->construction_returned&&c->construction_return_result==-1&&c->construction_result==-1&&!c->construction_body_completed);assert(f->s.candidate_preexec_returned&&!f->s.candidate_preexec_result);
 if(mode)assert(stages>0&&f->preexec.bootstrap_original==&f->bootstrap&&f->bootstrap.candidate_preexec==&shared);else assert(!stages);
 unsigned before=stages;assert(sl_linux_construct_contained_producer(c)==-1&&stages==before&&c->construction_return_result==-1&&c->construction_result==-1);
 }return 0;}
