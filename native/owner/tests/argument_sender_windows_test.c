#ifdef _WIN32
/* Synthetic native-call fixture for the actual worker body. This does not
 * qualify native threads, provider behavior, authentication or admission. */
#include "../argument_sender_windows.h"
#include <assert.h>
#include <string.h>
static DWORD fixture_error;
static unsigned fixture_mode,fixture_queries,fixture_writes,fixture_sleeps;
static unsigned fixture_cancels;
static sl_win_argument_write *fixture_write;
static sl_win_argument_write_cancel *fixture_cancel;
static BOOL fixture_cancel_io(HANDLE h,LPOVERLAPPED overlap){
 assert(h==fixture_write->actual_server&&overlap==&fixture_write->overlap);fixture_cancels++;
 fixture_error=overlap->Internal==STATUS_PENDING?0:ERROR_NOT_FOUND;
 return overlap->Internal==STATUS_PENDING?TRUE:FALSE;
}
static LONG fixture_exchange(LONG volatile *p,LONG value){LONG old=*p;*p=value;return old;}
static LONG fixture_compare(LONG volatile *p,LONG value,LONG compare){LONG old=*p;if(old==compare)*p=value;return old;}
static DWORD fixture_thread_id(void){return 77;}
static DWORD fixture_last_error(void){return fixture_error;}
static BOOL fixture_write_file(HANDLE h,LPCVOID bytes,DWORD extent,LPDWORD count,LPOVERLAPPED overlap){
 assert(h==fixture_write->actual_server&&bytes==fixture_write->actual_span.bytes&&extent==3&&!count&&overlap==&fixture_write->overlap);
 assert(!fixture_write->returned);fixture_writes++;
 if(fixture_mode==2){fixture_error=ERROR_ACCESS_DENIED;return FALSE;}
 if(fixture_mode==4){assert(!sl_win_request_argument_write_cancel(fixture_cancel));assert(fixture_cancel->request.last_error==ERROR_NOT_FOUND);}
 overlap->Internal=STATUS_PENDING;fixture_error=ERROR_IO_PENDING;return FALSE;
}
static BOOL fixture_query(HANDLE h,LPOVERLAPPED overlap,LPDWORD count,BOOL wait){
 assert(h==fixture_write->actual_server&&overlap==&fixture_write->overlap&&wait);
 assert(!fixture_write->returned&&fixture_write->write.returned&&fixture_write->write.last_error==ERROR_IO_PENDING);
 fixture_queries++;*count=fixture_mode==1?0:3;
 if(fixture_mode==5)assert(sl_win_request_argument_write_cancel(fixture_cancel));
 if(fixture_mode==3){overlap->Internal=STATUS_PENDING;fixture_error=ERROR_IO_INCOMPLETE;return FALSE;}
 overlap->Internal=(fixture_mode==1||fixture_mode>=4)?0xc0000120:0;
 fixture_error=(fixture_mode==1||fixture_mode>=4)?ERROR_OPERATION_ABORTED:0;
 return (fixture_mode==1||fixture_mode>=4)?FALSE:TRUE;
}
static VOID fixture_sleep(DWORD duration){assert(duration==INFINITE);fixture_sleeps++;/* unsupported provider return, separately refused */}
#define GetCurrentThreadId fixture_thread_id
#define GetLastError fixture_last_error
#define WriteFile fixture_write_file
#define GetOverlappedResult fixture_query
#define Sleep fixture_sleep
#define CancelIoEx fixture_cancel_io
#undef InterlockedExchange
#undef InterlockedCompareExchange
#define InterlockedExchange fixture_exchange
#define InterlockedCompareExchange fixture_compare
#include "../argument_sender_windows.c"
static void fixture_worker(unsigned mode){
 sl_win_private_arguments arguments;sl_win_duplex duplex;
 sl_win_argument_sender sender;sl_win_argument_write write;sl_win_argument_worker worker;
 sl_win_argument_write_cancel cancel;
 const unsigned char bytes[]={1,2,3};
 memset(&arguments,0,sizeof(arguments));memset(&duplex,0,sizeof(duplex));
 memset(&sender,0,sizeof(sender));memset(&write,0,sizeof(write));memset(&worker,0,sizeof(worker));
 memset(&cancel,0,sizeof(cancel));
 sender.returned=1;sender.result=1;sender.actual_arguments=&arguments;sender.actual_duplex=&duplex;
 sender.actual_server=(HANDLE)(ULONG_PTR)20;sender.span_count=1;sender.spans[0].bytes=bytes;sender.spans[0].extent=3;
 write.worker_original=&worker;write.sender_original=&sender;write.actual_sender=&sender;write.actual_server=sender.actual_server;
 write.actual_span=sender.spans[0];write.event=(HANDLE)(ULONG_PTR)30;write.overlap.hEvent=write.event;
 write.event_create.returned=1;write.event_create.result=(ULONG_PTR)write.event;
 worker.actual_write=&write;worker.returned=1;worker.create.returned=1;worker.create.result=40;worker.thread=(HANDLE)(ULONG_PTR)40;worker.thread_id=77;
 cancel.worker_original=&worker;fixture_cancel=&cancel;
 fixture_mode=mode;fixture_write=&write;fixture_error=0;fixture_queries=fixture_writes=fixture_sleeps=fixture_cancels=0;
 DWORD exit_code=sl_win_argument_write_worker(&write);
 if(mode==3){
  assert(exit_code==1&&fixture_queries==1&&fixture_sleeps==1);
  assert(!write.returned&&!write.worker_settled&&!write.operation_settled&&!write.delivered);
  assert(write.suspension.entered&&write.suspension.returned);
  return;
 }
 assert(exit_code==0);
 assert(fixture_writes==1&&write.write.returned&&write.returned&&!fixture_sleeps);
 if(mode==2){assert(!fixture_queries&&!write.worker_query.entered&&!write.initiation_supported&&!write.worker_settled);}
 else {
  assert(fixture_queries==1&&write.worker_query.entered&&write.worker_query.returned&&write.worker_settled&&write.initiation_supported);
  assert(write.worker_macro_before==0&&write.worker_macro_after==1);
  assert(write.worker_transferred==(mode==1?0:3));
  assert(write.worker_query.result==((mode==1||mode>=4)?FALSE:TRUE));
  if(mode==4){assert(fixture_cancels==2&&write.worker_cancellation_observed==1&&write.worker_cancel.returned&&cancel.publication.returned);}
  if(mode==5){assert(fixture_cancels==1&&write.worker_cancellation_observed==0&&!write.worker_cancel.entered&&cancel.publication.returned);}
  /* Worker settlement never substitutes independent controller delivery. */
  assert(!write.operation_settled&&!write.delivered);
 }
}
static void fixture_output_alias_refusal(void){
 sl_win_private_arguments arguments;sl_win_duplex duplex;sl_win_argument_sender sender;
 sl_win_argument_send_sequence sequence;
 union {max_align_t alignment;sl_win_argument_send_sequence value;unsigned char bytes[sizeof(sl_win_argument_send_sequence)];} aliased_sequence;
 union {max_align_t alignment;sl_win_argument_send_turn value;unsigned char bytes[sizeof(sl_win_argument_send_turn)];} aliased_turn;
 union {max_align_t alignment;sl_win_argument_send_cancellation value;unsigned char bytes[sizeof(sl_win_argument_send_cancellation)];} aliased_cancel;
 unsigned char sequence_before[sizeof(aliased_sequence)],turn_before[sizeof(aliased_turn)],cancel_before[sizeof(aliased_cancel)];
 memset(&arguments,0,sizeof(arguments));memset(&duplex,0,sizeof(duplex));memset(&sender,0,sizeof(sender));memset(&sequence,0,sizeof(sequence));
 memset(&aliased_sequence,0,sizeof(aliased_sequence));memset(&aliased_turn,0,sizeof(aliased_turn));memset(&aliased_cancel,0,sizeof(aliased_cancel));
 sender.returned=sender.result=1;sender.actual_arguments=&arguments;sender.actual_duplex=&duplex;sender.span_count=1;
 aliased_sequence.value.sender_original=&sender;aliased_sequence.value.stack_reservation=4096;
 sender.spans[0].bytes=aliased_sequence.bytes;sender.spans[0].extent=sizeof(aliased_sequence);
 memcpy(sequence_before,&aliased_sequence,sizeof(aliased_sequence));
 assert(!sl_win_prepare_argument_send_sequence(&aliased_sequence.value));
 assert(!memcmp(sequence_before,&aliased_sequence,sizeof(aliased_sequence)));
 sequence.returned=sequence.prepared=1;sequence.actual_sender=&sender;sequence.span_count=1;
 aliased_turn.value.sequence_original=&sequence;sender.spans[0].bytes=aliased_turn.bytes;sender.spans[0].extent=sizeof(aliased_turn);
 memcpy(turn_before,&aliased_turn,sizeof(aliased_turn));
 assert(!sl_win_service_argument_send(&aliased_turn.value));
 assert(!memcmp(turn_before,&aliased_turn,sizeof(aliased_turn)));
 aliased_cancel.value.sequence_original=&sequence;sender.spans[0].bytes=aliased_cancel.bytes;sender.spans[0].extent=sizeof(aliased_cancel);
 memcpy(cancel_before,&aliased_cancel,sizeof(aliased_cancel));
 assert(!sl_win_cancel_argument_send(&aliased_cancel.value));
 assert(!memcmp(cancel_before,&aliased_cancel,sizeof(aliased_cancel))&&!sequence.cancelled);
}
static void fixture_cancellation_roster_alias(void){
 sl_win_private_arguments arguments;sl_win_duplex duplex;sl_win_argument_sender sender;sl_win_argument_send_sequence sequence;
 union {max_align_t alignment;sl_win_argument_send_turn turns[2];unsigned char bytes[2*sizeof(sl_win_argument_send_turn)];} roster;
 unsigned char before[sizeof(roster)];const unsigned char source[]={1,2,3};
 _Static_assert(sizeof(sl_win_argument_send_cancellation)<=sizeof(sl_win_argument_send_turn),"fixture cancellation fits future turn");
 memset(&arguments,0,sizeof(arguments));memset(&duplex,0,sizeof(duplex));memset(&sender,0,sizeof(sender));memset(&sequence,0,sizeof(sequence));memset(&roster,0,sizeof(roster));
 sender.returned=sender.result=1;sender.actual_arguments=&arguments;sender.actual_duplex=&duplex;sender.span_count=1;sender.spans[0].bytes=source;sender.spans[0].extent=sizeof(source);
 sequence.returned=sequence.prepared=1;sequence.actual_sender=&sender;sequence.span_count=1;sequence.actual_turns=roster.turns;sequence.actual_turn_capacity=2;
 sl_win_argument_send_cancellation *cancel=(sl_win_argument_send_cancellation *)(void *)&roster.turns[1];
 cancel->sequence_original=&sequence;memcpy(before,&roster,sizeof(roster));
 assert(!sl_win_cancel_argument_send(cancel));
 assert(!memcmp(before,&roster,sizeof(roster))&&!sequence.cancelled);
}
int main(void){fixture_worker(0);fixture_worker(1);fixture_worker(2);fixture_worker(3);fixture_worker(4);fixture_worker(5);fixture_output_alias_refusal();fixture_cancellation_roster_alias();return 0;}
#endif
