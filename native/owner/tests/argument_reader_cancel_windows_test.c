#ifdef _WIN32
/* Synthetic native substitutions exercise actual production order/refusal.
 * No authentic D, provider, thread, reservation or custody is established. */
#include "../argument_reader_cancel_windows.h"
#include <assert.h>
#include <string.h>
static unsigned mode,step;
static DWORD error_original;
static sl_win_argument_reader_cancel *current;
int sl_win_cancel_argument_send(sl_win_argument_send_cancellation *o){assert(step++==(mode==3?3:4));assert(o==&current->writer_cancellation&&o->sequence_original==&current->actual_startup->argument_sequence);o->attempted=o->returned=o->dispatch_stopped=1;return 1;}
int sl_win_public_startup_factory_disjoint(const sl_win_public_startup *s,const void *p,SIZE_T n){assert(s&&p&&n);return 1;}
static BOOL fixture_disconnect(HANDLE h){assert(step++==0&&h==(HANDLE)10);assert(current->actual_startup->argument_sequence.cancelled);error_original=mode==1?ERROR_ACCESS_DENIED:0;return mode==1?FALSE:TRUE;}
static DWORD fixture_thread_id(HANDLE h){assert(step++==1&&h==(HANDLE)20);return 30;}
static DWORD fixture_wait(HANDLE h,DWORD ms){assert(step++==2&&h==(HANDLE)20&&!ms);return mode==3?WAIT_OBJECT_0:WAIT_TIMEOUT;}
static BOOL fixture_cancel(HANDLE h){assert(step++==3&&h==(HANDLE)20);assert(current->disconnect.returned&&current->thread_wait.returned);error_original=mode==2?ERROR_NOT_FOUND:0;return mode==2?FALSE:TRUE;}
static DWORD fixture_error(void){return error_original;}
#define DisconnectNamedPipe fixture_disconnect
#define GetThreadId fixture_thread_id
#define WaitForSingleObject fixture_wait
#define CancelSynchronousIo fixture_cancel
#define GetLastError fixture_error
#include "../argument_reader_cancel_windows.c"

int main(void){
 for(mode=0;mode<4;mode++){
  sl_win_public_startup startup={0};sl_win_debug_release release={0};
  sl_win_root_write write={0};sl_win_mapped_root_observation mapping={0};
  sl_win_debug_event_observation event={0};sl_windows_native_root_storage source={0};
  sl_win_argument_send_turn turns[2]={0};unsigned char bytes[3]={1,2,3};
  sl_win_argument_reader_cancel cancel={0},snapshot;
  startup.returned=startup.result=1;startup.duplex.server=(HANDLE)10;
  startup.controller.entry.birth.process.hThread=(HANDLE)20;
  startup.controller.entry.birth.process.dwThreadId=30;
  startup.argument_sender.span_count=1;startup.argument_sender.spans[0].bytes=bytes;
  startup.argument_sender.spans[0].extent=sizeof(bytes);
  startup.argument_sequence.prepared=1;
  startup.argument_sequence.actual_sender=&startup.argument_sender;
  startup.argument_sequence.actual_turns=turns;startup.argument_sequence.actual_turn_capacity=2;
  release.returned=release.result=1;release.resume_primary.returned=1;
  release.previous_suspend_count=1;release.actual_write=&write;
  write.actual_before=&mapping;write.actual_source=&source;
  mapping.actual_event=&event;event.actual_birth=&startup.controller.entry.birth;
  cancel.startup_original=&startup;cancel.release_original=&release;
  current=&cancel;step=error_original=0;
  assert(sl_win_cancel_argument_reader(&cancel)==(mode==0));
  assert(cancel.returned&&cancel.dispatch_stopped&&startup.argument_sequence.cancelled);
  assert(cancel.disconnected==(mode!=1));
  assert(cancel.disconnect.returned&&cancel.thread_query.returned&&cancel.thread_wait.returned);
  assert(step==(mode==3?4:5));assert(cancel.cancel.returned==(mode!=3));
  assert(cancel.writer_result&&cancel.writer_cancellation.returned);
  if(mode==1)assert(cancel.disconnect.last_error==ERROR_ACCESS_DENIED&&cancel.cancel.result);
  if(mode==2)assert(cancel.cancel.last_error==ERROR_NOT_FOUND);
  memcpy(&snapshot,&cancel,sizeof(cancel));
  assert(!sl_win_cancel_argument_reader(&cancel));assert(!memcmp(&snapshot,&cancel,sizeof(cancel)));
 }
 return 0;
}
#endif
