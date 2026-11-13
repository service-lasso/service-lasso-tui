#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include "public_inputs_linux.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
static int inputs_disjoint(const void *a,unsigned long an,const void *b,unsigned long bn){
 uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return a&&b&&an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&!(x<y+bn&&y<x+an);
}
static int inputs_fresh(const void *p,unsigned long n){const unsigned char *b=p;for(unsigned long i=0;i<n;i++)if(b[i])return 0;return 1;}
static int inputs_source(const sl_linux_public_input_source *s){
 return s&&s->argc>=0&&s->argument_capacity&&s->environment_capacity&&s->argument_bytes&&s->environment_bytes&&
 (unsigned long)s->argc<=s->argument_capacity&&(!s->argc||s->argv)&&
 s->argument_capacity<ULONG_MAX/sizeof(void*)&&s->environment_capacity<ULONG_MAX/sizeof(void*)&&
 s->argument_capacity<=ULONG_MAX/sizeof(sl_linux_public_string)&&s->environment_capacity<=ULONG_MAX/sizeof(sl_linux_public_string)&&
 (!s->argv||((unsigned long)s->argc+1)*sizeof(void*)<=UINTPTR_MAX-(uintptr_t)s->argv)&&
 (!s->environment||(s->environment_capacity+1)*sizeof(void*)<=UINTPTR_MAX-(uintptr_t)s->environment);
}
static unsigned long inputs_length(const char *p,unsigned long maximum){
 if(!p||!maximum||maximum>UINTPTR_MAX-(uintptr_t)p)return 0;
 for(unsigned long i=0;i<maximum;i++)if(!p[i])return i+1;
 return 0;
}
static int inputs_same(const sl_linux_public_input_source *a,const sl_linux_public_input_source *b){
 return a->argc==b->argc&&a->argv==b->argv&&a->environment==b->environment&&
 a->argument_capacity==b->argument_capacity&&a->environment_capacity==b->environment_capacity&&
 a->argument_bytes==b->argument_bytes&&a->environment_bytes==b->environment_bytes;
}
/* Walk only actual original strings. The admitted native entry owns validity
 * of original OS pointer headers; dimensions never authenticate that origin. */
static int inputs_walk(const sl_linux_public_input_source *s,const void *out,unsigned long bytes,
 const sl_linux_public_string *arguments,const sl_linux_public_string *environment,
 unsigned long *ec,unsigned long *ab,unsigned long *eb){
 if(!inputs_source(s))return 0;
 if(out&&(!inputs_disjoint(out,bytes,s,sizeof(*s))||
    (s->argv&&!inputs_disjoint(out,bytes,s->argv,((unsigned long)s->argc+1)*sizeof(void*)))))return 0;
 unsigned long a=0,e=0,n=0;
 for(unsigned long i=0;i<(unsigned long)s->argc;i++){
  unsigned long length=inputs_length(s->argv[i],s->argument_bytes-a);
  if(!length||(out&&!inputs_disjoint(out,bytes,s->argv[i],length))||
     (arguments&&(arguments[i].bytes!=s->argv[i]||arguments[i].units!=length)))return 0;
  a+=length;
 }
 if(s->argv&&s->argv[s->argc])return 0;
 if(s->environment){
  for(;n<s->environment_capacity&&s->environment[n];n++){
   unsigned long length=inputs_length(s->environment[n],s->environment_bytes-e);
   if(!length||(out&&!inputs_disjoint(out,bytes,s->environment[n],length))||
      (environment&&(environment[n].bytes!=s->environment[n]||environment[n].units!=length)))return 0;
   e+=length;
  }
  if(s->environment[n])return 0;
 }
 if(out&&s->environment&&!inputs_disjoint(out,bytes,s->environment,(n+1)*sizeof(void*)))return 0;
 *ec=n;*ab=a;*eb=e;return 1;
}
int sl_linux_public_input_source_excludes_output(const sl_linux_public_input_source *s,const void *out,unsigned long bytes){
 unsigned long n,a,e;
 return out&&bytes&&inputs_walk(s,out,bytes,0,0,&n,&a,&e);
 /* Pure preeffect pointer/header/string correspondence, no ownership grant. */
}
int sl_linux_public_inputs_match_original(const sl_linux_public_inputs *o){
 if(!o||!o->attempted||!o->returned||o->result||!o->actual_source||o->actual_source!=o->source_original||
    !inputs_source(o->actual_source)||!inputs_same(o->actual_source,&o->source_snapshot)||
    !o->actual_arguments||o->actual_arguments!=o->arguments||!o->actual_environment||o->actual_environment!=o->environment)return 0;
 unsigned long n,a,e;const sl_linux_public_input_source *s=o->actual_source;
 const void *outputs[3]={o,o->actual_arguments,o->actual_environment};
 unsigned long sizes[3]={sizeof(*o),s->argument_capacity*sizeof(*o->actual_arguments),s->environment_capacity*sizeof(*o->actual_environment)};
 for(unsigned i=0;i<3;i++){
  for(unsigned j=0;j<i;j++)if(!inputs_disjoint(outputs[i],sizes[i],outputs[j],sizes[j]))return 0;
  if(!inputs_walk(s,outputs[i],sizes[i],o->actual_arguments,o->actual_environment,&n,&a,&e))return 0;
 }
 return o->argument_count==(unsigned long)s->argc&&o->environment_count==n&&o->argument_extent==a&&o->environment_extent==e;
}
int sl_linux_public_inputs_exclude_output(const sl_linux_public_inputs *o,const void *out,unsigned long bytes){
 if(!bytes||!sl_linux_public_inputs_match_original(o)||!inputs_disjoint(out,bytes,o,sizeof(*o))||
    !inputs_disjoint(out,bytes,o->actual_arguments,o->source_snapshot.argument_capacity*sizeof(*o->actual_arguments))||
    !inputs_disjoint(out,bytes,o->actual_environment,o->source_snapshot.environment_capacity*sizeof(*o->actual_environment)))return 0;
 unsigned long n,a,e;
 return inputs_walk(o->actual_source,out,bytes,o->actual_arguments,o->actual_environment,&n,&a,&e);
}
long sl_linux_capture_public_inputs(sl_linux_public_inputs *o){
 if(!o||!o->arguments||!o->environment||!inputs_source(o->source_original))return -EINVAL;
 const sl_linux_public_input_source *s=o->source_original;
 const void *outputs[3]={o,o->arguments,o->environment};
 unsigned long sizes[3]={sizeof(*o),s->argument_capacity*sizeof(*o->arguments),s->environment_capacity*sizeof(*o->environment)};
 unsigned long n,a,e;
 for(unsigned i=0;i<3;i++){
  for(unsigned j=0;j<i;j++)if(!inputs_disjoint(outputs[i],sizes[i],outputs[j],sizes[j]))return -EINVAL;
  if(!inputs_walk(s,outputs[i],sizes[i],0,0,&n,&a,&e))return -EINVAL;
 }
 if(!inputs_fresh(&o->actual_source,sizeof(*o)-offsetof(sl_linux_public_inputs,actual_source))||
    !inputs_fresh(o->arguments,sizes[1])||!inputs_fresh(o->environment,sizes[2]))return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->actual_source=s;o->source_snapshot=*s;
 o->actual_arguments=o->arguments;o->actual_environment=o->environment;
 for(unsigned long i=0;i<(unsigned long)s->argc;i++){
  unsigned long length=inputs_length(s->argv[i],s->argument_bytes-o->argument_extent);if(!length)goto done;
  o->actual_arguments[i]=(sl_linux_public_string){s->argv[i],length};o->argument_count++;o->argument_extent+=length;
 }
 for(unsigned long i=0;i<n;i++){
  unsigned long length=inputs_length(s->environment[i],s->environment_bytes-o->environment_extent);if(!length)goto done;
  o->actual_environment[i]=(sl_linux_public_string){s->environment[i],length};o->environment_count++;o->environment_extent+=length;
 }
 if(inputs_same(s,&o->source_snapshot))o->result=0;
 done:o->returned=1;if(!o->result&&!sl_linux_public_inputs_match_original(o))o->result=-EINVAL;return o->result;
 /* Source/provider/finance and sole prethread writer proof dominate activation.
  * Parent and child pointer values alone never establish shared physical bytes. */
}
long sl_linux_capture_entry_public_inputs(sl_linux_public_inputs *o,sl_linux_public_input_source *s,int argc,char **argv,char **environment){
 if(!o||!s||o->source_original!=s||!o->arguments||!o->environment||s->argc||s->argv||s->environment)return -EINVAL;
 sl_linux_public_input_source proposed=*s;proposed.argc=argc;proposed.argv=argv;proposed.environment=environment;
 if(!inputs_source(&proposed)||!inputs_fresh(&o->actual_source,sizeof(*o)-offsetof(sl_linux_public_inputs,actual_source)))return -EINVAL;
 const void *outputs[4]={s,o,o->arguments,o->environment};
 unsigned long sizes[4]={sizeof(*s),sizeof(*o),s->argument_capacity*sizeof(*o->arguments),s->environment_capacity*sizeof(*o->environment)};
 unsigned long n,a,e;
 for(unsigned i=0;i<4;i++){
  for(unsigned j=0;j<i;j++)if(!inputs_disjoint(outputs[i],sizes[i],outputs[j],sizes[j]))return -EINVAL;
  if(!inputs_walk(&proposed,outputs[i],sizes[i],0,0,&n,&a,&e))return -EINVAL;
 }
 if(!inputs_fresh(o->arguments,sizes[2])||!inputs_fresh(o->environment,sizes[3]))return -EINVAL;
 s->argc=argc;s->argv=argv;s->environment=environment;
 return sl_linux_capture_public_inputs(o);
 /* Actual caller supplies the native values, not this storage binder. The
  * retained tuple survives failures; neither source metadata nor this return
  * authenticates the OS entry or permits birth/useful public continuation. */
}
#endif