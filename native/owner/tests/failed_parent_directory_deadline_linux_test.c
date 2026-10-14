/* Link actual directory/declaration/deadline/FD-lift bodies. Only native calls
 * are synthetic. UNRUN: no OS/parent/source/containing qualification. */
#include "../failed_parent_directory_linux.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>
static unsigned calls;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long a,long b,long d,long e,long f,long g){
 assert(number==SYS_clock_gettime&&a==CLOCK_MONOTONIC&&b&&!d&&!e&&!f&&!g);
 calls++;c->source=source;c->number=number;long args[6]={a,b,d,e,f,g};memcpy(c->arguments,args,sizeof args);
 *(struct timespec *)b=(struct timespec){10,0};c->result=0;atomic_store(&c->entered,1);atomic_store(&c->returned,1);return 0;
}
int main(void){
 static unsigned char operation[8],body[32];
 static sl_linux_deadline_source source;
 static sl_linux_deadline root;
 static sl_linux_deadline_sequence sequence;
 union clock_birth {sl_linux_deadline_observation observations[10];sl_linux_outer_birth birth;} storage={0};
 source=(sl_linux_deadline_source){operation,sizeof operation,1000000};
 assert(!sl_linux_construct_deadline(&root,&source));
 sequence.root_original=&root;sequence.observations_original=storage.observations;sequence.capacity=10;
 assert(!sl_linux_prepare_deadline_sequence(&sequence));
 storage.birth.prepared=1;
 assert(!sl_linux_deadline_sequence_excludes_output(&sequence,&storage.birth,sizeof storage.birth));
 sl_candidate_profile candidate={.source=body,.source_extent=sizeof body,.original={body,sizeof body},.returned=1};
 sl_linux_failed_parent_directory output={0},output_snapshot=output;
 unsigned char path[32]={0},path_snapshot[sizeof path];memcpy(path_snapshot,path,sizeof path);
 sl_linux_archive_declaration rows[1]={0},rows_snapshot[1];memcpy(rows_snapshot,rows,sizeof rows);
 union clock_birth storage_snapshot;memcpy(&storage_snapshot,&storage,sizeof storage);
 sl_linux_deadline_sequence sequence_snapshot=sequence;unsigned before=calls;
 assert(sl_linux_acquire_failed_parent_directory(&output,&candidate,&storage.birth,&sequence,path,sizeof path,rows,1)==-EINVAL);
 assert(calls==before&&!memcmp(&output,&output_snapshot,sizeof output)&&!memcmp(path,path_snapshot,sizeof path)&&
    !memcmp(rows,rows_snapshot,sizeof rows)&&!memcmp(&storage,&storage_snapshot,sizeof storage)&&!memcmp(&sequence,&sequence_snapshot,sizeof sequence));
 return 0;
}
