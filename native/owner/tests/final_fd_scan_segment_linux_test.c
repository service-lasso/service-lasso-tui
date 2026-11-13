/* UNRUN: settled raw FD scan/input-map segment, not native exclusion proof. */
#include "final_segment_test_support.h"
typedef struct scan_case {bootstrap_case base;sl_linux_fd_scan_frame frames[2];sl_linux_fd_view view;sl_linux_fd_action action;unsigned char raw[2];} scan_case;
int main(void){
 static scan_case cases[4];
 for(unsigned m=0;m<4;m++){
  scan_case *e=&cases[m];sl_linux_public_controller *c=&e->base.c;sl_linux_final_capture *r=&c->final_capture;sl_linux_fd_scan *scan=&c->scan;sl_linux_fd_map *map=&c->child_map;
  r->prepared_controller=c;r->actual_records=e->base.records;r->actual_capacity=100;r->actual_readback=e->base.bytes;r->actual_bytes=sizeof(e->base.bytes);r->actual_writes.calls=e->base.writes;r->actual_writes.capacity=2;r->actual_reads.calls=e->base.reads;r->actual_reads.capacity=2;
  scan->attempted=scan->eof=1;scan->frames=e->frames;scan->frame_count=scan->frame_capacity=2;scan->views=&e->view;scan->view_count=scan->view_capacity=1;
  map->attempted=1;map->scan_original=scan;map->views=&e->view;map->count=1;map->actions=&e->action;map->action_count=map->action_capacity=1;e->action.outer_original=&e->view;e->view.original=&e->frames[0];
  sl_linux_call *calls[8]={&scan->open_directory,&scan->filesystem_query,&map->standard_state[0],&map->standard_state[1],&map->standard_state[2],&e->view.inspect_flags,&e->frames[0].read,&e->frames[1].read};
  for(unsigned i=0;i<8;i++){calls[i]->source="actual.fixture";atomic_init(&calls[i]->entered,1);atomic_init(&calls[i]->returned,1);}
  e->raw[0]='x';for(unsigned i=0;i<2;i++){e->frames[i].scan_original=scan;e->frames[i].bytes=&e->raw[i];e->frames[i].capacity=1;}e->frames[0].raw_count=e->frames[0].read.result=1;
  if(m==1)e->frames[0].bytes=e->base.records[0].header;
  if(m==2)e->frames[1].raw_count=e->frames[1].read.result=1;
  if(m==3)e->action.outer_original=NULL;
  unsigned char before[sizeof(e->frames)];memcpy(before,e->frames,sizeof(before));unsigned long count=0,extent=0;long result=final_fd_scan(r,0,&count,&extent);
  assert(!r->count&&!r->extent&&!memcmp(before,e->frames,sizeof(before)));
  if(!m){assert(!result&&count==15);assert(!final_fd_scan(r,1,&count,&extent));assert(r->count==15&&r->actual_records[9].body==&e->raw[0]&&r->actual_records[11].length==0&&r->actual_records[14].source_original==&e->action);}
  else assert(result<0&&!r->count&&!r->extent);
 }
 return 0;
}