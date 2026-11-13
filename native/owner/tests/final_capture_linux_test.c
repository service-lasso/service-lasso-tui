/* UNRUN: actual capture-component storage body with substituted file syscalls.
 * These are not actual durability, native source closure, seal or ACK proofs. */
#include "../public_startup_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <errno.h>
static unsigned mode,effects,writes;static unsigned char stored[16384];static unsigned long stored_extent;
static uint64_t decoded(const unsigned char *p){uint64_t value=0;for(unsigned i=0;i<8;i++)value|=(uint64_t)p[i]<<(8*i);return value;}
long sl_linux_call6(sl_linux_call *call,const char *source,long number,long fd,long bytes,long length,long offset,long e,long f){
 (void)e;(void)f;assert(!atomic_load(&call->entered));call->source=source;call->number=number;call->arguments[0]=fd;call->arguments[1]=bytes;call->arguments[2]=length;call->arguments[3]=offset;atomic_store(&call->entered,1);effects++;
 if(number==SYS_pwrite64){assert(fd==91&&offset>=0&&length>0&&(unsigned long)(offset+length)<=sizeof(stored));writes++;if(mode==1&&writes==2)call->result=-EIO;else{long n=writes==1?1:length;memcpy(stored+offset,(void*)bytes,(size_t)n);if((unsigned long)(offset+n)>stored_extent)stored_extent=offset+n;call->result=n;}}
 else if(number==SYS_fsync){assert(fd==90||fd==91);call->result=mode==2&&fd==90?-EIO:0;}
 else if(number==SYS_pread64){assert(fd==91&&offset>=0&&length>0);if((unsigned long)offset==stored_extent){call->result=mode==3?1:0;if(call->result)((unsigned char*)bytes)[0]='x';}else{assert((unsigned long)(offset+length)<=stored_extent);memcpy((void*)bytes,stored+offset,(size_t)length);if(mode==4)((unsigned char*)bytes)[0]^=1;call->result=length;}}
 else{assert(0);call->result=-EINVAL;}
 atomic_store(&call->returned,1);return call->result;
}
#include "../final_capture_linux.c"
typedef struct episode {
 sl_linux_public_controller c;sl_linux_public_startup startup;sl_linux_service_observation service;
 sl_candidate_profile candidate;unsigned char candidate_bytes[3];
 sl_linux_capture_worker reader;sl_linux_capture_frame frames[2];
 sl_linux_final_record records[100];unsigned char readback[16384],local[2];char executable[8],directory[8];
 sl_linux_call writes[256],reads[4],public_write;
 sl_linux_membership_frame membership_frames[2];unsigned char membership_body[5],membership_eof;
 sl_linux_call mount_reads[2];unsigned char mount_body[6];
 sl_linux_selinux_snapshot policy;
 sl_linux_policy_frame policy_frames[2];unsigned char policy_raw[3],policy_eof,policy_body[3];char policy_path[2];
 long prepare_result;
} episode;
int main(void){
 static episode cases[29];unsigned char raw_before[80],metadata_before[sizeof(sl_linux_capture_frame)],membership_before[sizeof(sl_linux_membership_frame)];
 for(mode=0;mode<29;mode++){
  episode *e=&cases[mode];sl_linux_public_controller *c=&e->c;sl_linux_final_capture *r=&c->final_capture;
  effects=writes=0;stored_extent=0;memset(stored,0,sizeof(stored));
  c->startup_original=&e->startup;e->startup.archive_directory.fd=90;
  memcpy(e->candidate_bytes,"src",3);e->candidate.source=e->candidate_bytes;e->candidate.source_extent=3;e->candidate.original=(sl_candidate_span){e->candidate_bytes,3};e->candidate.returned=1;e->startup.actual_candidate=&e->candidate;
  e->startup.archive_files.returned=1;e->startup.archive_files.directory_original=&e->startup.archive_directory;
  e->startup.archive_files.final.returned=1;e->startup.archive_files.final.directory_original=&e->startup.archive_directory;e->startup.archive_files.final.fd=91;
  r->controller_original=c;r->records=e->records;r->capacity=100;r->readback=e->readback;r->bytes=sizeof(e->readback);r->writes.calls=e->writes;r->writes.capacity=mode==5?1:256;r->reads.calls=e->reads;r->reads.capacity=4;
  if(mode==17||mode==18){
   sl_linux_local_installation *i=&e->startup.installation;sl_linux_installed_payload *p=&i->payload;
   i->attempted=i->returned=p->attempted=p->returned=1;memcpy(e->executable,"exe",3);e->directory[0]='/';
   i->prepared_executable_path=e->executable;i->prepared_executable_capacity=sizeof(e->executable);i->raw_link_count=3;
   i->prepared_directory_path=e->directory;i->prepared_directory_capacity=sizeof(e->directory);i->directory_length=1;
   p->prepared_locator=e->directory;p->directory_length=1;p->directory_capacity=sizeof(e->directory);
   sl_linux_call *calls[11]={&i->open_proc,&i->proc_filesystem,&i->open_executable,&i->executable_identity,&i->readlink,&i->open_public,&i->public_identity,&p->open_directory,&p->open_payload,&p->directory_identity,&p->payload_identity};
   for(unsigned n=0;n<11;n++){atomic_init(&calls[n]->entered,0);atomic_init(&calls[n]->returned,0);}
   if(mode==18){atomic_init(&e->writes[0].entered,0x123456);i->prepared_executable_path=(char*)&e->writes[0].entered;i->prepared_executable_capacity=3;}
  }
   if(mode==19||mode==20){
   sl_linux_local_membership *m=&e->startup.membership;m->attempted=m->returned=m->eof=1;m->fd=77;m->prepared_frames=e->membership_frames;m->prepared_capacity=m->count=2;
   m->prepared_path=e->directory;m->prepared_path_capacity=sizeof(e->directory);m->path_length=1;e->directory[0]='/';memcpy(e->membership_body,"0::/\n",5);
   atomic_init(&m->open.entered,0);atomic_init(&m->open.returned,0);atomic_init(&m->filesystem_query.entered,0);atomic_init(&m->filesystem_query.returned,0);
   for(unsigned n=0;n<2;n++){sl_linux_membership_frame *f=&e->membership_frames[n];f->bytes=n?&e->membership_eof:e->membership_body;f->capacity=n?1:5;f->raw_count=f->read.result=n?0:5;f->read.number=SYS_read;f->read.arguments[0]=77;f->read.arguments[1]=(long)f->bytes;f->read.arguments[2]=(long)f->capacity;atomic_init(&f->read.entered,1);atomic_init(&f->read.returned,1);}
   if(mode==20){memcpy(e->records,&e->membership_frames[0],sizeof(e->membership_frames[0]));m->prepared_frames=(sl_linux_membership_frame*)e->records;m->prepared_capacity=m->count=1;memcpy(membership_before,e->records,sizeof(membership_before));}
  }
  if(mode==9){uintptr_t start=(uintptr_t)e->frames[0].bytes;unsigned long alignment=_Alignof(sl_linux_final_record);r->records=(sl_linux_final_record*)(start+(alignment-start%alignment)%alignment);}
  if(mode==10)r->records=(sl_linux_final_record*)&e->frames[1];
  if(mode==12)r->capacity=1;
  if(mode==13){e->candidate.source=e->readback;e->candidate.original.bytes=e->readback;}
  if(mode==14){atomic_init(&e->writes[0].entered,0x55AA77);e->candidate.source=(const unsigned char*)&e->writes[0].entered;e->candidate.original.bytes=e->candidate.source;}
  if(mode==15){atomic_init(&e->reads[0].returned,0x66BB88);e->candidate.source=(const unsigned char*)&e->reads[0].returned;e->candidate.original.bytes=e->candidate.source;}
  if(mode==16){r->prepared=0x77CC99;e->candidate.source=(const unsigned char*)&r->prepared;e->candidate.original.bytes=e->candidate.source;}
  unsigned char candidate_before[3];memcpy(candidate_before,e->candidate.source,3);
  if(mode>=21&&mode<=23){
   sl_linux_local_mounts *m=&e->startup.mounts;m->attempted=m->returned=m->eof=1;m->fd=78;m->length=5;m->actual_capacity=6;m->actual_bytes=e->mount_body;m->actual_reads=e->mount_reads;m->actual_read_capacity=m->read_count=2;memcpy(e->mount_body,"mount",5);
   sl_linux_call *fixed[3]={&m->open,&m->filesystem_query,&m->close};for(unsigned n=0;n<3;n++){fixed[n]->source="mount.fixture";atomic_init(&fixed[n]->entered,1);atomic_init(&fixed[n]->returned,1);}
   for(unsigned n=0;n<2;n++){sl_linux_call *call=&e->mount_reads[n];call->source="mount.read.fixture";call->number=SYS_read;call->arguments[0]=78;call->arguments[1]=(long)(e->mount_body+(n?5:0));call->arguments[2]=n?1:6;call->result=n?0:5;atomic_init(&call->entered,1);atomic_init(&call->returned,1);}
   if(mode==22){atomic_init(&e->writes[0].entered,0x654321);m->actual_bytes=(unsigned char*)&e->writes[0].entered;m->actual_capacity=sizeof(e->writes[0].entered);}
   if(mode==23)e->mount_reads[1].arguments[1]=(long)e->mount_body;
  }
  if(mode==24||mode==25){
   sl_linux_candidate_target *t=&e->startup.candidate_target;t->attempted=t->returned=t->bound=1;t->tuple[0]=123;t->tuple[2]=456;t->target_fd=79;t->procs_fd=80;
   sl_linux_call *calls[6]={&t->open_target,&t->open_procs,&t->target_filesystem,&t->procs_filesystem,&t->target_identity,&t->procs_identity};
   for(unsigned n=0;n<6;n++){calls[n]->source="target.fixture";calls[n]->result=n;atomic_init(&calls[n]->entered,1);atomic_init(&calls[n]->returned,1);}
   if(mode==25)atomic_store(&t->open_procs.returned,0);
  }
  if(mode>=26){
   sl_linux_selinux_snapshot *p=&e->policy;e->startup.actual_policies=p;e->startup.actual_policy_capacity=1;
   p->attempted=p->returned=p->eof=1;p->policy_fd=81;p->actual_frames=e->policy_frames;p->actual_frame_capacity=p->frame_count=2;
   p->actual_body=e->policy_body;p->actual_body_capacity=p->length=3;memcpy(e->policy_body,"pol",3);memcpy(e->policy_raw,"pol",3);
   p->actual_path=e->policy_path;p->actual_path_capacity=2;p->path_length=1;e->policy_path[0]='/';
   for(unsigned n=0;n<2;n++){sl_linux_policy_frame *f=&e->policy_frames[n];f->attempted=f->returned=1;f->actual_bytes=n?&e->policy_eof:e->policy_raw;f->actual_capacity=n?1:3;f->length=n?0:3;f->read.source="policy.fixture.read";f->read.number=SYS_read;f->read.arguments[0]=81;f->read.arguments[1]=(long)f->actual_bytes;f->read.arguments[2]=f->actual_capacity;f->read.result=f->length;atomic_init(&f->read.entered,1);atomic_init(&f->read.returned,1);}
   if(mode==27)e->policy_body[1]='X';
   if(mode==28){atomic_init(&e->writes[0].entered,0x777333);p->actual_body=(unsigned char*)&e->writes[0].entered;}
  }
  e->prepare_result=sl_linux_prepare_final_capture(r);
  if(mode==28){assert(e->prepare_result==-EINVAL&&!r->prepare_attempted&&!effects&&atomic_load(&e->writes[0].entered)==0x777333);continue;}
  if((mode>=13&&mode<=16)||mode==18||mode==20||mode==22){assert(e->prepare_result==-EINVAL&&!r->prepare_attempted&&!effects&&!memcmp(candidate_before,e->candidate.source,3));if(mode==14)assert(atomic_load(&e->writes[0].entered)==0x55AA77);if(mode==15)assert(atomic_load(&e->reads[0].returned)==0x66BB88);if(mode==16)assert(r->prepared==0x77CC99);if(mode==18)assert(atomic_load(&e->writes[0].entered)==0x123456);if(mode==20)assert(!memcmp(membership_before,e->records,sizeof(membership_before)));if(mode==22)assert(atomic_load(&e->writes[0].entered)==0x654321);continue;}
  assert(!e->prepare_result);
  /* Exported proposal fields do not replace frozen ownership/backing. */
  r->controller_original=0;r->records=0;r->capacity=0;r->readback=0;r->bytes=0;r->writes.calls=0;
  e->startup.actual_candidate=0; /* frozen constructor relation survives proposal replacement */
  c->drained_by=&e->service.source_cut;c->drain_returned=1;e->service.source_cut.returned=1;e->service.source_cut.controller_original=c;e->service.source_cut.service_original=&e->service;e->service.source_cut.reader_count=e->service.source_cut.checked_readers=1;e->service.source_cut.readers_original=&e->reader;e->service.source_cut.request_capacity=e->service.source_cut.checked_requests=1;
  c->collection.returned=1;c->collection.result=8;c->collection.readback_equal=1;c->collection.prepared_body=e->local;c->collection.body_length=c->collection.prepared_capacity=2;c->collection.prepared_storage=92;c->collection.prepared_storage_original=&e->startup.archive_files.local;e->local[0]='L';e->local[1]='0';
  c->collection.prepared_directory_original=&e->startup.archive_directory;c->collection.prepared_directory=90;
  e->service.source_cut.root_original=&e->service.root;e->service.source_cut.family_original=&e->service.family_join;e->service.family_join.worker_original=&e->service.family;
  e->service.root.wait.number=SYS_waitid;e->service.root.wait.arguments[0]=P_PIDFD;e->service.root.wait.result=0;atomic_init(&e->service.root.wait.entered,1);atomic_init(&e->service.root.wait.returned,1);e->service.root.information.si_pid=123;e->service.root.information.si_code=CLD_EXITED;e->service.root.usage.ru_utime.tv_sec=9;
  static const unsigned char family_bytes[]="populated 0\nfrozen 0\n";
  e->service.family.observation.read_count=22;
  for(unsigned i=0;i<22;i++){sl_linux_family_read *read=&e->service.family.observation.reads[i];read->returned=1;read->count=i<21?1:0;read->byte=i<21?family_bytes[i]:0;read->call.number=SYS_read;read->call.arguments[0]=99;read->call.arguments[1]=(long)&read->byte;read->call.arguments[2]=1;read->call.result=read->count;atomic_init(&read->call.entered,1);atomic_init(&read->call.returned,1);}
  e->reader.prepared_frames=e->frames;e->reader.prepared_capacity=e->reader.read_count=2;e->frames[0].read_returned=e->frames[1].read_returned=1;e->frames[0].raw_count=3;memcpy(e->frames[0].bytes,"abc",3);
  e->frames[0].read.number=SYS_read;e->frames[0].read.arguments[0]=77;e->frames[0].read.arguments[1]=(long)e->frames[0].bytes;e->frames[0].read.arguments[2]=sizeof(e->frames[0].bytes);e->frames[0].read.result=3;atomic_init(&e->frames[0].read.entered,1);atomic_init(&e->frames[0].read.returned,1);
  e->frames[0].public_writes=&e->public_write;e->frames[0].public_write_count=e->frames[0].public_write_capacity=1;e->public_write.number=SYS_write;e->public_write.arguments[0]=88;e->public_write.arguments[1]=(long)e->frames[0].bytes;e->public_write.arguments[2]=3;e->public_write.result=3;atomic_init(&e->public_write.entered,1);atomic_init(&e->public_write.returned,1);
  if(mode==6)e->service.source_cut.result=-EIO;
  if(mode==7)c->collection.prepared_body=e->readback;
  if(mode==8)c->collection.prepared_body=e->records[0].header;
  if(mode==9){e->frames[0].raw_count=sizeof(raw_before);memset(e->frames[0].bytes,0xA5,sizeof(raw_before));memcpy(raw_before,e->frames[0].bytes,sizeof(raw_before));}
  if(mode==10)memcpy(metadata_before,&e->frames[1],sizeof(metadata_before));
  if(mode==11){e->frames[0].raw_count=e->frames[0].read.result=-EAGAIN;e->frames[0].public_write_count=0;}
  long result=sl_linux_persist_final_capture(r);assert(r->returned&&r->result==result&&r->cut_original==&e->service.source_cut);
  if(mode==26){unsigned long expected=5005+12*32+9*80+4;assert(!result&&r->readback_equal&&r->count==74&&r->extent==expected);assert(r->actual_records[70].source_original==&e->policy_frames[0].read&&r->actual_records[71].source_original==&e->policy_frames[0]&&r->actual_records[71].body==e->policy_raw&&decoded(r->actual_records[71].header)==12&&r->actual_records[73].source_original==&e->policy_frames[1]&&!r->actual_records[73].length);}
  else if(mode==24){unsigned long expected=5005+16*32+10*80+2*sizeof(struct statfs)+2*sizeof(struct statx)+2*sizeof(struct open_how);assert(!result&&r->readback_equal&&r->count==78&&r->extent==expected);assert(r->actual_records[62].source_original==&e->startup.candidate_target&&decoded(r->actual_records[63].body)==123&&decoded(r->actual_records[63].body+16)==456&&r->actual_records[66].source_original==&e->startup.candidate_target.open_target&&r->actual_records[77].body==(const unsigned char*)&e->startup.candidate_target.procs_how);}
  else if(mode==21){unsigned long expected=5005+9*32+7*80+sizeof(struct statfs)+5;assert(!result&&r->readback_equal&&r->count==71&&r->extent==expected);assert(r->actual_records[70].body==e->mount_body&&r->actual_records[70].source_original==&e->startup.mounts&&decoded(r->actual_records[70].header)==11&&!memcmp(r->actual_readback+r->actual_records[70].offset+32,"mount",5));}
  else if(mode==19){unsigned long expected=5005+10*32+6*80+sizeof(struct statfs)+6;assert(!result&&r->readback_equal&&r->count==72&&r->extent==expected);assert(r->actual_records[69].source_original==&e->membership_frames[0]&&r->actual_records[69].body==e->membership_body&&r->actual_records[69].length==5&&!memcmp(r->actual_readback+r->actual_records[69].offset+32,"0::/\n",5));assert(r->actual_records[71].source_original==&e->membership_frames[1]&&!r->actual_records[71].length&&decoded(r->actual_records[71].header+24)==0);}
  else if(mode==17){unsigned long expected=5005+28*32+18*80+sizeof(struct statfs)+4*sizeof(struct statx)+2*sizeof(struct open_how)+5;assert(!result&&r->readback_equal&&r->count==90&&r->extent==expected);assert(r->actual_records[62].source_original==&e->startup.installation&&decoded(r->actual_records[62].body)==1&&r->actual_records[69].source_original==&e->startup.installation.open_proc);assert(r->actual_records[87].body==(const unsigned char*)e->executable&&r->actual_records[88].body==(const unsigned char*)e->directory&&r->actual_records[89].body==(const unsigned char*)e->directory);}
  else if(mode==11){assert(!result&&r->readback_equal&&r->count==61&&r->extent==4890&&r->actual_records[48].length==0&&decoded(r->actual_records[48].header+24)==(uint64_t)(long)-EAGAIN&&r->actual_records[50].source_original==&e->frames[0].read&&decoded(r->actual_records[50].body+56)==(uint64_t)(long)-EAGAIN);}
  else if(!mode){assert(!result&&r->readback_equal&&r->count==62&&r->extent==5005&&r->actual_writes.completed==5005&&r->actual_reads.completed==5005&&r->eof.result==0);assert(r->actual_records[0].body==e->local&&r->actual_records[48].source_original==&e->frames[0]&&r->actual_records[48].body==e->frames[0].bytes&&r->actual_records[55].length==0);assert(r->actual_records[50].source_original==&e->frames[0].read&&decoded(r->actual_records[50].body)==SYS_read&&decoded(r->actual_records[50].body+8)==77&&decoded(r->actual_records[50].body+16)==(uint64_t)(uintptr_t)e->frames[0].bytes&&decoded(r->actual_records[50].body+56)==3&&decoded(r->actual_records[50].body+64)==1&&decoded(r->actual_records[50].body+72)==1);assert(r->actual_records[54].source_original==&e->public_write&&decoded(r->actual_records[54].body)==SYS_write&&decoded(r->actual_records[54].header+24)==5);}
  else{assert(result<0&&!r->readback_equal);if(mode==1)assert(r->actual_writes.calls[1].result==-EIO&&stored_extent==1);if(mode==2)assert(r->flush.result==0&&r->directory_flush.result==-EIO&&!r->actual_reads.count);if(mode==3)assert(r->eof.result==1);if(mode==5)assert(result==-ENOSPC&&stored_extent==1);if(mode>=6)assert(!effects&&!r->count);if(mode==7||mode==8)assert(result==-EINVAL&&c->collection.prepared_body==(mode==7?e->readback:e->records[0].header));}
  unsigned before=effects;assert(sl_linux_persist_final_capture(r)==-EINVAL&&effects==before&&r->result==result);
  if(!mode)assert(r->actual_records[1].source_original==&e->service.root.wait&&r->actual_records[2].body==(const unsigned char*)&e->service.root.information&&r->actual_records[3].body==(const unsigned char*)&e->service.root.usage&&r->actual_records[46].source_original==&e->service.family.observation.reads[21].call&&r->actual_records[47].source_original==&e->service.family.observation.reads[21]&&!r->actual_records[47].length);
  if(!mode)assert(r->actual_records[61].source_original==&e->candidate&&r->actual_records[61].body==e->candidate_bytes&&r->actual_records[61].length==3&&decoded(r->actual_records[61].header)==6&&!memcmp(r->actual_readback+r->actual_records[61].offset+32,"src",3));
  if(mode==9)assert(result==-EINVAL&&!memcmp(raw_before,e->frames[0].bytes,sizeof(raw_before))&&!r->count&&!r->extent);
  if(mode==10)assert(result==-EINVAL&&!memcmp(metadata_before,&e->frames[1],sizeof(metadata_before))&&!r->count&&!r->extent);
 }
 return 0;
}
