#if defined(__linux__) && defined(__x86_64__)
#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
#include <sys/syscall.h>
static void failed_archive_close(sl_linux_target_close_view *v,sl_linux_call *c,unsigned *attempted,int fd,const char *source){
 *attempted=1;atomic_init(&c->entered,0);atomic_init(&c->returned,0);
 long r=sl_linux_call6(c,source,SYS_close,fd,0,0,0,0,0);if(r&&!v->failure_original)v->failure_original=c;
}
static void failed_archive_view(sl_linux_target_close_view *v,sl_linux_call *open,sl_linux_fd_lift *lift,int recorded_raw){
 v->attempted=1;v->open_original=open;v->lift_original=lift;v->raw_fd=v->held_fd=-1;v->result=0;
 if(!open->source)goto done;
 if(open->number!=SYS_openat2||!atomic_load_explicit(&open->returned,memory_order_acquire)||open->result>INT_MAX){v->unsettled_original=open;goto recorded;}
 if(open->result<0)goto done;
 int raw=(int)open->result;v->raw_fd=raw;if(raw!=recorded_raw){v->unmapped=1;goto recorded;}
 if(lift->attempted){
  if(lift->original_fd!=raw){v->unmapped=1;goto recorded;}
  if(raw>=3){if(lift->held_fd!=raw){v->unmapped=1;goto recorded;}v->held_fd=raw;}
  else if(atomic_load_explicit(&lift->duplicate.entered,memory_order_acquire)){
   if(!atomic_load_explicit(&lift->duplicate.returned,memory_order_acquire)){v->unsettled_original=&lift->duplicate;goto recorded;}
   if(lift->duplicate.result>=3&&lift->duplicate.result<=INT_MAX&&lift->duplicate.result==lift->held_fd)v->held_fd=lift->held_fd;
   else if(lift->duplicate.result>=0){v->unmapped=1;goto recorded;}
  }
 }
 if(raw!=v->held_fd&&lift->attempted&&atomic_load_explicit(&lift->close_original.entered,memory_order_acquire)){
  if(!atomic_load_explicit(&lift->close_original.returned,memory_order_acquire))v->unsettled_original=&lift->close_original;
  else if(lift->close_original.result)v->failure_original=&lift->close_original;
 }
 if(v->held_fd>=3)failed_archive_close(v,&v->close_held,&v->held_attempted,v->held_fd,"failed-prebirth.archive.held.close-once");
 if(raw!=v->held_fd&&(!lift->attempted||!atomic_load_explicit(&lift->close_original.entered,memory_order_acquire)))failed_archive_close(v,&v->close_raw,&v->raw_attempted,raw,"failed-prebirth.archive.raw.close-once");
 done:v->returned=1;
 recorded:v->returned=1;if(v->failure_original)v->result=v->failure_original->result<0?v->failure_original->result:-EINVAL;else if(v->unsettled_original||v->unmapped)v->result=-EINVAL;
}
long sl_linux_close_failed_archive_before_birth(sl_linux_public_controller *c){
 if(!c||!c->attempted||!c->construction_result||!c->startup_original||c->startup_original->controller_original!=c||c->entry.attempted||c->entry.birth.attempted||c->family_active||c->collector_create_entered)return -EINVAL;
 sl_linux_public_startup *s=c->startup_original;sl_linux_archive_files *o=&s->archive_files;sl_linux_archive_directory *d=&s->archive_directory;
 if(!s->candidate_preexec_returned||!s->candidate_preexec_result||(d->attempted&&!d->returned)||(o->attempted&&!o->returned)||(o->local.attempted&&!o->local.returned)||(o->final.attempted&&!o->final.returned)||(s->final_reader.attempted&&!s->final_reader.returned)||o->closes_attempted)return -EINVAL;
 for(unsigned i=0;i<4;i++)if(o->closes[i].attempted||o->closes[i].close_raw.source||o->closes[i].close_held.source)return -EINVAL;
 o->closes_attempted=1;o->close_result=0;
 failed_archive_view(&o->closes[3],&s->final_reader.open,&s->final_reader.lift,s->final_reader.raw_fd);
 failed_archive_view(&o->closes[0],&o->final.open,&o->final.lift,o->final.raw_fd);
 failed_archive_view(&o->closes[1],&o->local.open,&o->local.lift,o->local.raw_fd);
 failed_archive_view(&o->closes[2],&d->open,&d->lift,d->raw_fd);
 static const unsigned order[4]={3,0,1,2};
 for(unsigned i=0;i<4;i++){sl_linux_target_close_view *v=&o->closes[order[i]];if(v->result&&!o->close_failure_original){o->close_failure_original=v;o->close_result=v->result;}}
 o->closes_returned=1;return o->close_result;
 /* No postbirth borrower, graph disposition, unlink or storage acceptance. */
}
#endif