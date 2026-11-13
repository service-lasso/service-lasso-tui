#if defined(__linux__) && defined(__x86_64__)
#include "immutable_payload_linux.h"
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <fcntl.h>
#include <stdint.h>
#include <limits.h>
static void image_initialize(sl_linux_call *call){atomic_init(&call->entered,0);atomic_init(&call->returned,0);}
static int image_overlap(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return 1;
 return x<y+bn&&y<x+an;
}
long sl_linux_prepare_immutable_payload(sl_linux_immutable_payload *payload){
 unsigned long i,j,k;long result;int seals=F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL;
 sl_linux_installed_payload *source;
 if(!payload||payload->attempted)return -1;
 payload->attempted=1;payload->fd=-1;payload->result=-1;
 if(payload->returned||payload->count||payload->extent||payload->source_eof||payload->seals_observed||payload->readback_complete||
    !payload->installation_original||!payload->installation_original->returned||payload->installation_original->result||
    !payload->frames||!payload->capacity||payload->capacity>UINTPTR_MAX/sizeof(*payload->frames))goto done;
 source=&payload->installation_original->payload;
 if(!source->returned||source->result||source->payload_fd<3||
    image_overlap(payload->frames,payload->capacity*sizeof(*payload->frames),payload,sizeof(*payload))||
    image_overlap(payload->frames,payload->capacity*sizeof(*payload->frames),payload->installation_original,sizeof(*payload->installation_original)))goto done;
 for(i=0;i<payload->capacity;i++){
  sl_linux_payload_frame *frame=&payload->frames[i];
  if(frame->owner_original||frame->raw_count||frame->read.source||frame->read.number||frame->written||frame->readback_count||frame->write_count||frame->read_count||
     !frame->source_bytes||!frame->readback_bytes||!frame->capacity||frame->capacity>2147483647UL||
     !frame->writes||!frame->readbacks||!frame->write_capacity||!frame->readback_capacity||
     frame->write_capacity>UINTPTR_MAX/sizeof(*frame->writes)||frame->readback_capacity>UINTPTR_MAX/sizeof(*frame->readbacks))goto done;
 }
 for(i=0;i<payload->capacity;i++){
  sl_linux_payload_frame *frame=&payload->frames[i];unsigned a,b;
  const void *regions[4]={frame->source_bytes,frame->readback_bytes,frame->writes,frame->readbacks};
  unsigned long sizes[4]={frame->capacity,frame->capacity,frame->write_capacity*sizeof(*frame->writes),frame->readback_capacity*sizeof(*frame->readbacks)};
  for(a=0;a<4;a++){
   if(image_overlap(regions[a],sizes[a],payload,sizeof(*payload))||
      image_overlap(regions[a],sizes[a],payload->installation_original,sizeof(*payload->installation_original))||
      image_overlap(regions[a],sizes[a],payload->frames,payload->capacity*sizeof(*payload->frames)))goto done;
   for(j=0;j<=i;j++){
    sl_linux_payload_frame *other=&payload->frames[j];
    const void *others[4]={other->source_bytes,other->readback_bytes,other->writes,other->readbacks};
    unsigned long other_sizes[4]={other->capacity,other->capacity,other->write_capacity*sizeof(*other->writes),other->readback_capacity*sizeof(*other->readbacks)};
    for(b=0;b<4;b++)if((j<i||b<a)&&image_overlap(regions[a],sizes[a],others[b],other_sizes[b]))goto done;
   }
  }
 }
 payload->prepared_frames=payload->frames;payload->prepared_capacity=payload->capacity;
  /* Typed attempt reservations are never cleared by this unit. Fresh frame
  * metadata cannot re-enroll an earlier original attempt array. */
 for(i=0;i<payload->prepared_capacity;i++){
  sl_linux_payload_frame *frame=&payload->prepared_frames[i];
  for(j=0;j<frame->write_capacity;j++)if(frame->writes[j].frame_original||frame->writes[j].attempted||frame->writes[j].call.source||frame->writes[j].call.number||frame->writes[j].raw_count)goto done;
  for(j=0;j<frame->readback_capacity;j++)if(frame->readbacks[j].frame_original||frame->readbacks[j].attempted||frame->readbacks[j].call.source||frame->readbacks[j].call.number||frame->readbacks[j].raw_count)goto done;
 }
 for(i=0;i<payload->prepared_capacity;i++){
  sl_linux_payload_frame *frame=&payload->prepared_frames[i];frame->owner_original=payload;
  for(j=0;j<frame->write_capacity;j++)frame->writes[j].frame_original=frame;
  for(j=0;j<frame->readback_capacity;j++)frame->readbacks[j].frame_original=frame;
 }
 image_initialize(&payload->create);image_initialize(&payload->add_seals);image_initialize(&payload->get_seals);image_initialize(&payload->readback_eof);
 result=sl_linux_call6(&payload->create,"payload.immutable.memfd_create",SYS_memfd_create,(long)"service-lasso-tui-go",MFD_CLOEXEC|MFD_ALLOW_SEALING,0,0,0,0);
 payload->result=result;if(result<0||result>2147483647L){if(result>2147483647L)payload->result=-1;goto done;}
 result=sl_linux_lift_internal_fd(&payload->lift,(int)result);payload->result=result;if(result<0)goto done;payload->fd=(int)result;
 for(i=0;i<payload->prepared_capacity;i++){
  sl_linux_payload_frame *frame=&payload->prepared_frames[i];image_initialize(&frame->read);frame->offset=payload->extent;
  result=sl_linux_call6(&frame->read,"payload.immutable.source.pread",SYS_pread64,source->payload_fd,(long)frame->source_bytes,frame->capacity,frame->offset,0,0);
  frame->raw_count=result;payload->count=i+1;payload->result=result;
  if(result<0||result>(long)frame->capacity){if(result>(long)frame->capacity)payload->result=-1;goto done;}
  if(!result){payload->source_eof=1;break;}
  if((unsigned long)result>LONG_MAX-payload->extent){payload->result=-1;goto done;}
  payload->extent+=(unsigned long)result;
  for(j=0;frame->written<(unsigned long)frame->raw_count&&j<frame->write_capacity;j++){
   sl_linux_payload_attempt *attempt=&frame->writes[j];unsigned long remaining=(unsigned long)frame->raw_count-frame->written;
   if(attempt->frame_original!=frame||attempt->attempted){payload->result=-1;goto done;}
   attempt->attempted=1;image_initialize(&attempt->call);
   result=sl_linux_call6(&attempt->call,"payload.immutable.copy.pwrite",SYS_pwrite64,payload->fd,(long)(frame->source_bytes+frame->written),remaining,frame->offset+frame->written,0,0);
   attempt->raw_count=result;frame->write_count=j+1;payload->result=result;
   if(result<=0||result>(long)remaining){if(result>=0)payload->result=-1;goto done;}frame->written+=(unsigned long)result;
  }
  if(frame->written!=(unsigned long)frame->raw_count){payload->result=-1;goto done;}
 }
 if(!payload->source_eof||payload->extent!=source->payload_state.stx_size){payload->result=-1;goto done;}
 result=sl_linux_call6(&payload->add_seals,"payload.immutable.F_ADD_SEALS",SYS_fcntl,payload->fd,F_ADD_SEALS,seals,0,0,0);
 payload->result=result;if(result)goto done;
 result=sl_linux_call6(&payload->get_seals,"payload.immutable.F_GET_SEALS",SYS_fcntl,payload->fd,F_GET_SEALS,0,0,0,0);
 payload->result=result;if(result<0||(result&seals)!=seals){if(result>=0)payload->result=-1;goto done;}payload->seals_observed=1;
 for(i=0;i<payload->count;i++){
  sl_linux_payload_frame *frame=&payload->prepared_frames[i];
  for(j=0;frame->readback_count<(unsigned long)frame->raw_count&&j<frame->readback_capacity;j++){
   sl_linux_payload_attempt *attempt=&frame->readbacks[j];unsigned long remaining=(unsigned long)frame->raw_count-frame->readback_count;
   if(attempt->frame_original!=frame||attempt->attempted){payload->result=-1;goto done;}
   attempt->attempted=1;image_initialize(&attempt->call);
   result=sl_linux_call6(&attempt->call,"payload.immutable.readback.pread",SYS_pread64,payload->fd,(long)(frame->readback_bytes+frame->readback_count),remaining,frame->offset+frame->readback_count,0,0);
   attempt->raw_count=result;frame->read_count=j+1;payload->result=result;
   if(result<=0||result>(long)remaining){if(result>=0)payload->result=-1;goto done;}frame->readback_count+=(unsigned long)result;
  }
  if(frame->readback_count!=(unsigned long)frame->raw_count){payload->result=-1;goto done;}
  for(k=0;k<frame->readback_count;k++)if(frame->source_bytes[k]!=frame->readback_bytes[k]){payload->result=-1;goto done;}
 }
 result=sl_linux_call6(&payload->readback_eof,"payload.immutable.readback.eof",SYS_pread64,payload->fd,(long)&payload->eof_byte,1,payload->extent,0,0);
 payload->raw_eof_count=result;payload->result=result;if(result){if(result>0)payload->result=-1;goto done;}
 payload->readback_complete=1;payload->result=0;
done:
 payload->returned=1;return payload->result;
}
#endif