/* UNRUN controlled native-IO model; not real memfd/seal/image/native proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../immutable_payload_linux.h"
#include <sys/syscall.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
static unsigned calls,writes,seal_calls;static long seal_error;static int missing_seal,mismatch,write_failure;
static unsigned char copied[16];static const unsigned char original[]="abcdef";
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long a,long b,long c,long d,long e,long f){
 long result=0;unsigned long n;calls++;
 call->source=source;call->number=number;call->arguments[0]=a;call->arguments[1]=b;call->arguments[2]=c;
 call->arguments[3]=d;call->arguments[4]=e;call->arguments[5]=f;atomic_store(&call->entered,1);
 if(number==SYS_memfd_create)result=11;
 else if(number==SYS_pread64){
  assert(d>=0);n=d>=6?0:6-(unsigned long)d;if(n>(unsigned long)c)n=c;if(a==11&&n>2)n=2;
  memcpy((void *)b,(a==10?original:copied)+d,n);if(a==11&&mismatch&&n)((unsigned char *)b)[0]^=1;result=n;
 }else if(number==SYS_pwrite64){
  writes++;if(write_failure&&writes==2)result=-EIO;
  else{n=c>2?2:c;assert(d>=0&&(unsigned long)d+n<=sizeof(copied));memcpy(copied+d,(void *)b,n);result=n;}
 }else if(number==SYS_fcntl){
  if(b==F_ADD_SEALS){seal_calls++;result=seal_error;}
  else{assert(b==F_GET_SEALS);result=missing_seal?F_SEAL_SEAL:F_SEAL_WRITE|F_SEAL_GROW|F_SEAL_SHRINK|F_SEAL_SEAL;}
 }
 call->result=result;atomic_store(&call->returned,1);return result;
}
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd){lift->attempted=1;lift->original_fd=fd;lift->held_fd=fd;return fd;}
#include "../immutable_payload_linux.c"
static void prepare(sl_linux_immutable_payload *payload,sl_linux_local_installation *installation,
 sl_linux_payload_frame frames[3],unsigned char bytes[3][4],unsigned char readback[3][4],
 sl_linux_payload_attempt attempts[3][2],sl_linux_payload_attempt reads[3][2]){
 unsigned i;memset(payload,0,sizeof(*payload));memset(frames,0,3*sizeof(*frames));
 memset(attempts,0,3*2*sizeof(**attempts));memset(reads,0,3*2*sizeof(**reads));memset(copied,0,sizeof(copied));
 payload->installation_original=installation;payload->frames=frames;payload->capacity=3;
 for(i=0;i<3;i++){frames[i].source_bytes=bytes[i];frames[i].readback_bytes=readback[i];frames[i].capacity=4;
  frames[i].writes=attempts[i];frames[i].readbacks=reads[i];frames[i].write_capacity=frames[i].readback_capacity=2;}
 calls=writes=seal_calls=0;seal_error=0;missing_seal=mismatch=write_failure=0;
}
int main(void){
 sl_linux_local_installation installation={0};sl_linux_immutable_payload payload,other;
 sl_linux_payload_frame frames[3],other_frames[3];unsigned char bytes[3][4],readback[3][4];sl_linux_payload_attempt attempts[3][2],reads[3][2];unsigned before;
 installation.returned=1;installation.payload.returned=1;installation.payload.payload_fd=10;installation.payload.payload_state.stx_size=6;
 prepare(&payload,&installation,frames,bytes,readback,attempts,reads);
 assert(sl_linux_prepare_immutable_payload(&payload)==0&&payload.source_eof&&payload.seals_observed&&payload.readback_complete);
 assert(payload.extent==6&&frames[0].write_count==2&&frames[0].read_count==2&&frames[2].raw_count==0);
 assert(!memcmp(copied,original,6)&&payload.raw_eof_count==0&&atomic_load(&payload.readback_eof.returned));
 before=calls;assert(sl_linux_prepare_immutable_payload(&payload)==-1&&calls==before);
 memset(&other,0,sizeof(other));other.installation_original=&installation;other.frames=frames;other.capacity=3;
 assert(sl_linux_prepare_immutable_payload(&other)==-1&&calls==before&&frames[0].owner_original==&payload);
  /* Frame array itself must remain separate from the held installation. */
 memset(&other,0,sizeof(other));other.installation_original=&installation;
 other.frames=(sl_linux_payload_frame *)&installation;other.capacity=1;
 assert(sl_linux_prepare_immutable_payload(&other)==-1&&calls==before);
 assert(installation.returned==1&&installation.payload.payload_fd==10);
 /* Different fresh frames still cannot reuse the prior attempt originals. */
 memset(&other,0,sizeof(other));memset(other_frames,0,sizeof(other_frames));
 for(unsigned index=0;index<3;index++){
  other_frames[index].source_bytes=bytes[index];other_frames[index].readback_bytes=readback[index];other_frames[index].capacity=4;
  other_frames[index].writes=attempts[index];other_frames[index].readbacks=reads[index];
  other_frames[index].write_capacity=other_frames[index].readback_capacity=2;
 }
 other.installation_original=&installation;other.frames=other_frames;other.capacity=3;
 assert(sl_linux_prepare_immutable_payload(&other)==-1&&calls==before);
 assert(attempts[0][0].frame_original==&frames[0]&&atomic_load(&attempts[0][0].call.returned));
 prepare(&payload,&installation,frames,bytes,readback,attempts,reads);write_failure=1;
 assert(sl_linux_prepare_immutable_payload(&payload)==-EIO&&frames[0].written==2&&frames[0].writes[1].raw_count==-EIO&&seal_calls==0);
 prepare(&payload,&installation,frames,bytes,readback,attempts,reads);seal_error=-EPERM;
 assert(sl_linux_prepare_immutable_payload(&payload)==-EPERM&&payload.extent==6&&!payload.seals_observed);
 prepare(&payload,&installation,frames,bytes,readback,attempts,reads);missing_seal=1;
 assert(sl_linux_prepare_immutable_payload(&payload)==-1&&payload.get_seals.result==F_SEAL_SEAL&&!payload.readback_complete);
 prepare(&payload,&installation,frames,bytes,readback,attempts,reads);mismatch=1;
 assert(sl_linux_prepare_immutable_payload(&payload)==-1&&payload.seals_observed&&!payload.readback_complete);
 prepare(&payload,&installation,frames,bytes,readback,attempts,reads);payload.capacity=2;
 assert(sl_linux_prepare_immutable_payload(&payload)==-1&&payload.extent==6&&!payload.source_eof&&seal_calls==0);
  prepare(&payload,&installation,frames,bytes,readback,attempts,reads);
 /* An actual entered original with no frame reservation is also not fresh. */
 sl_linux_call6(&attempts[0][0].call,"fixture.prior.native",SYS_fcntl,11,F_GET_SEALS,0,0,0,0);
 before=calls;
 assert(sl_linux_prepare_immutable_payload(&payload)==-1&&calls==before);
 assert(attempts[0][0].call.source&&!strcmp(attempts[0][0].call.source,"fixture.prior.native"));
 return 0;
}
#endif