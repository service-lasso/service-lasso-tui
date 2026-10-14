/* UNRUN actual held-reader + ELF bodies; all native calls are synthetic. */
#if defined(__linux__) && defined(__x86_64__)
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../creator_elf_linux.h"
static unsigned char input[256];static unsigned long input_extent=256;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a0,long a1,long a2,long a3,long a4,long a5){
 c->source=source;c->number=number;c->arguments[0]=a0;c->arguments[1]=a1;c->arguments[2]=a2;c->arguments[3]=a3;c->arguments[4]=a4;c->arguments[5]=a5;
 atomic_store(&c->entered,1);long result=0;
 if(number==SYS_clock_gettime){assert(a0==CLOCK_MONOTONIC);*(struct timespec*)a1=(struct timespec){1,0};c->result=0;atomic_store(&c->returned,1);return 0;}
 assert(a0==5);
 if(number==SYS_statx){struct statx *s=(void*)a4;s->stx_mask=a3;s->stx_mode=S_IFREG|0600;s->stx_size=input_extent;s->stx_ino=19;}
 else{assert(number==SYS_pread64);assert((unsigned long)a3<=input_extent);result=input_extent-(unsigned long)a3;if(result>a2)result=a2;if(result)memcpy((void*)a1,input+a3,result);}
 c->result=result;atomic_store(&c->returned,1);return result;
}
#include "../deadline_linux.c"
#include "../creator_file_linux.c"
#include "../creator_elf_linux.c"
static void put(unsigned long offset,unsigned long value,unsigned bytes){for(unsigned i=0;i<bytes;i++)input[offset+i]=(unsigned char)(value>>(8*i));}
static void image(void){
 memset(input,0,sizeof(input));input_extent=sizeof(input);input[0]=0x7f;input[1]='E';input[2]='L';input[3]='F';input[4]=2;input[5]=1;input[6]=1;
 put(16,2,2);put(18,62,2);put(20,1,4);put(24,0x400000,8);put(32,64,8);put(52,64,2);put(54,56,2);put(56,2,2);
 put(64,3,4);put(72,184,8);put(96,4,8);memcpy(input+184,"/ld",4);
 put(120,1,4);put(124,5,4);put(136,0x400000,8);put(152,256,8);put(160,256,8);put(168,4096,8);
}
typedef struct fixture {
 sl_linux_entry entry;sl_linux_local_installation installation;sl_linux_creator_file file;
 sl_linux_deadline deadline;sl_linux_deadline_source source;sl_linux_deadline_sequence sequence;sl_linux_deadline_observation observations[10];
 unsigned char body[256];sl_linux_creator_read reads[2];sl_linux_creator_elf elf;sl_linux_elf_program programs[3];
} fixture;
static void capture(fixture *x){
 memset(x,0,sizeof(*x));sl_linux_local_installation *s=&x->installation;s->attempted=s->returned=1;s->entry_original=&x->entry;s->proc_fd=4;s->executable_fd=5;
 s->executable_lift.attempted=1;s->executable_lift.original_fd=s->executable_lift.held_fd=5;
 s->open_executable.number=SYS_openat;s->open_executable.result=5;s->open_executable.arguments[0]=4;s->open_executable.arguments[1]=(long)"exe";
 s->open_executable.arguments[2]=O_RDONLY|O_NONBLOCK|O_CLOEXEC;atomic_init(&s->open_executable.entered,1);atomic_init(&s->open_executable.returned,1);
 s->executable_identity.number=SYS_statx;s->executable_identity.arguments[0]=5;s->executable_identity.arguments[1]=(long)"";
 s->executable_identity.arguments[2]=AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW;
 s->executable_identity.arguments[3]=STATX_TYPE|STATX_MODE|STATX_UID|STATX_GID|STATX_INO|STATX_MNT_ID|STATX_SIZE;
 s->executable_identity.arguments[4]=(long)&s->executable_state;atomic_init(&s->executable_identity.entered,1);atomic_init(&s->executable_identity.returned,1);
 x->file.installation_original=s;x->file.body=x->body;x->file.body_capacity=sizeof(x->body);x->file.reads=x->reads;x->file.read_capacity=2;
 x->source=(sl_linux_deadline_source){input,sizeof(input),1000000000UL};
 assert(!sl_linux_construct_deadline(&x->deadline,&x->source));
 x->sequence.root_original=&x->deadline;x->sequence.observations_original=x->observations;x->sequence.capacity=10;
 assert(!sl_linux_prepare_deadline_sequence(&x->sequence));x->file.sequence_original=&x->sequence;
 assert(!sl_linux_capture_creator_file(&x->file)&&sl_linux_creator_file_matches_original(&x->file));
 x->elf.file_original=&x->file;x->elf.programs=x->programs;x->elf.capacity=3;
}
int main(void){
 fixture x;image();capture(&x);assert(!sl_linux_interpret_creator_elf(&x.elf)&&x.elf.complete&&x.elf.load_count==1&&x.elf.interpreter_count==1);
 assert(x.elf.first_interpreter==0&&!x.elf.interpreter_after_load&&x.programs[0].interpreter_bytes==3);
 sl_linux_creator_elf saved=x.elf;assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&!memcmp(&saved,&x.elf,sizeof(saved)));
 image();put(56,0xffff,2);put(40,192,8);put(58,64,2);put(236,2,4);capture(&x);
 assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&x.elf.extended_count&&x.elf.extended_count_offset==236&&x.elf.program_count==2&&!x.elf.complete);
 image();put(56,0xffff,2);put(40,192,8);put(58,64,2);put(196,1,4);put(236,0xffff,4);capture(&x);
 assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&x.elf.extended_count&&!x.elf.complete&&!x.programs[0].interpreted);
 image();put(56,0xffff,2);put(40,192,8);put(58,64,2);put(236,0xffff,4);capture(&x);
 assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&x.elf.program_count==0xffff&&!x.elf.complete); /* bounded table extent */
 image();put(32,ULONG_MAX,8);capture(&x);assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&!x.elf.complete);
 image();memset(input+184,'x',4);capture(&x);assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&!x.programs[0].interpreted);
 image();put(160,1,8);capture(&x);assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&x.elf.load_count==1&&!x.elf.complete);
 image();capture(&x);x.elf.capacity=1;assert(sl_linux_interpret_creator_elf(&x.elf)==-ENOSPC&&!x.programs[0].interpreted);
 image();input_extent=20;capture(&x);assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&!x.elf.complete);
 image();capture(&x);x.elf.programs=(void*)&x.entry;x.elf.capacity=1;saved=x.elf;sl_linux_entry entry_saved=x.entry;
 assert(sl_linux_interpret_creator_elf(&x.elf)==-EINVAL&&!memcmp(&saved,&x.elf,sizeof(saved))&&!memcmp(&entry_saved,&x.entry,sizeof(entry_saved)));
 return 0;
}
#endif
