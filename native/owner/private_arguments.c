#include "private_arguments.h"
#include <limits.h>
#include <stdint.h>
static int argument_overlap(const void *a,unsigned long n,const void *b,unsigned long m){
 uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
 if(!a||!b||!n||!m||n>UINTPTR_MAX-aa||m>UINTPTR_MAX-bb)return 1;
 return aa<bb+m&&bb<aa+n;
}
static int argument_output_overlap(sl_private_arguments *o,const void *p,unsigned long n){
 return argument_overlap(p,n,o,sizeof *o)||
  argument_overlap(p,n,o->actual_private_argv,o->actual_pointer_capacity*sizeof(char*))||
  argument_overlap(p,n,o->actual_lengths,o->actual_length_capacity*sizeof(unsigned long));
}
static int argument_decimal(char *out,unsigned long capacity,unsigned long value,unsigned long *length){
 unsigned long divisor=1;unsigned long n=1;
 while(value/divisor>=10){if(divisor>ULONG_MAX/10)return -1;divisor*=10;n++;}
 if(n>=capacity)return -1;
 for(unsigned long i=0;i<n;i++){out[i]=(char)('0'+value/divisor);value%=divisor;divisor/=10;}
 out[n]=0;*length=n;return 0;
}
long sl_prepare_private_arguments(sl_private_arguments *o){
 if(!o||o->attempted)return -1;
 /* Read the original graph before any occurrence/output write. The later
  * retained captures cannot repair an input already overwritten by them. */
 if(o->public_argc<0||(o->public_argc&&!o->public_argv)||!o->private_argv||!o->lengths||!o->string_byte_capacity||
    o->pointer_capacity>ULONG_MAX/sizeof(char*)||o->length_capacity>ULONG_MAX/sizeof(unsigned long)||
    (unsigned long)o->public_argc>ULONG_MAX-8||o->pointer_capacity<(unsigned long)(o->public_argc?o->public_argc:1)+8||
    o->length_capacity<(unsigned long)o->public_argc||
    (unsigned long)o->public_argc+1>ULONG_MAX/sizeof(char*))return -1;
 unsigned long pointer_bytes=o->pointer_capacity*sizeof(char*),length_bytes=o->length_capacity*sizeof(unsigned long);
 if(argument_overlap(o,sizeof(*o),o->private_argv,pointer_bytes)||
    argument_overlap(o,sizeof(*o),o->lengths,length_bytes)||
    argument_overlap(o->private_argv,pointer_bytes,o->lengths,length_bytes))return -1;
 const void *originals[4]={o->public_argv,o->security_quantity_original,o->terminal_quantity_original,o->representation_quantity_original};
 unsigned long extents[4]={((unsigned long)o->public_argc+1)*sizeof(char*),sizeof(unsigned long),sizeof(unsigned long),sizeof(unsigned long)};
 for(unsigned i=0;i<4;i++)if((i||o->public_argv)&&(argument_overlap(o,sizeof(*o),originals[i],extents[i])||
    argument_overlap(o->private_argv,pointer_bytes,originals[i],extents[i])||
    argument_overlap(o->lengths,length_bytes,originals[i],extents[i])))return -1;
 unsigned long input_bytes=0;
 for(unsigned long i=0;i<(unsigned long)o->public_argc;i++){
  const char *s=o->public_argv[i];unsigned long n=0;if(!s)return -1;
  while(n<o->string_byte_capacity-input_bytes&&s[n])n++;
  if(n==o->string_byte_capacity-input_bytes||
     argument_overlap(o,sizeof(*o),s,n+1)||argument_overlap(o->private_argv,pointer_bytes,s,n+1)||
     argument_overlap(o->lengths,length_bytes,s,n+1))return -1;
  input_bytes+=n+1;
 }
 if(o->public_argv&&o->public_argv[o->public_argc])return -1;
 o->attempted=1;o->result=-1;
 o->actual_public_argc=o->public_argc;o->actual_public_argv=o->public_argv;o->actual_custody_fd=o->custody_fd;
 o->actual_private_argv=o->private_argv;o->actual_pointer_capacity=o->pointer_capacity;
 o->actual_lengths=o->lengths;o->actual_length_capacity=o->length_capacity;o->actual_string_byte_capacity=o->string_byte_capacity;
 if(o->actual_public_argc<0||o->actual_custody_fd<3||(o->actual_public_argc&&!o->actual_public_argv)||!o->actual_private_argv||!o->actual_lengths||
    !o->actual_string_byte_capacity||o->public_bytes||o->private_count||o->descriptor_length||o->count_length||o->security_length||o->terminal_length||o->representation_length||
    (unsigned long)o->actual_public_argc>ULONG_MAX-8||o->actual_pointer_capacity<(unsigned long)(o->actual_public_argc?o->actual_public_argc:1)+8||
    o->actual_length_capacity<(unsigned long)o->actual_public_argc)goto done;
 if(o->actual_pointer_capacity>ULONG_MAX/sizeof(char*)||o->actual_length_capacity>ULONG_MAX/sizeof(unsigned long)||
    (unsigned long)o->actual_public_argc+1>ULONG_MAX/sizeof(char*))goto done;
 if(argument_overlap(o,sizeof *o,o->actual_private_argv,o->actual_pointer_capacity*sizeof(char*))||
    argument_overlap(o,sizeof *o,o->actual_lengths,o->actual_length_capacity*sizeof(unsigned long))||
    argument_overlap(o->actual_private_argv,o->actual_pointer_capacity*sizeof(char*),o->actual_lengths,o->actual_length_capacity*sizeof(unsigned long))||
    (o->actual_public_argv&&argument_output_overlap(o,o->actual_public_argv,((unsigned long)o->actual_public_argc+1)*sizeof(char*))))goto done;
 if(!o->security_quantity_original||!o->terminal_quantity_original||!o->representation_quantity_original||
    argument_output_overlap(o,o->security_quantity_original,sizeof(unsigned long))||
    argument_output_overlap(o,o->terminal_quantity_original,sizeof(unsigned long))||
    argument_output_overlap(o,o->representation_quantity_original,sizeof(unsigned long)))goto done;
 o->actual_security_bytes=*o->security_quantity_original;o->actual_terminal_calls=*o->terminal_quantity_original;o->actual_representation_bytes=*o->representation_quantity_original;
 if(!o->actual_security_bytes||o->actual_security_bytes>UINT_MAX||!o->actual_terminal_calls||o->actual_terminal_calls>LONG_MAX||
    !o->actual_representation_bytes||o->actual_representation_bytes>LONG_MAX)goto done;
 /* Actual public main supplies kernel argv storage. Each string's complete
  * byte extent is bounded against separately selected source geometry. */
 for(unsigned long i=0;i<(unsigned long)o->actual_public_argc;i++){
  char *s=o->actual_public_argv[i];unsigned long n=0;if(!s||o->actual_lengths[i])goto done;
  while(n<o->actual_string_byte_capacity-o->public_bytes&&s[n])n++;
  if(n==o->actual_string_byte_capacity-o->public_bytes)goto done;
  if(argument_output_overlap(o,s,n+1))goto done;
  o->public_bytes+=n+1;
 }
 if(o->actual_public_argv&&o->actual_public_argv[o->actual_public_argc])goto done;
 /* All input aliases are separated before the first length/pointer write. */
 for(unsigned long i=0;i<(unsigned long)o->actual_public_argc;i++){unsigned long n=0;while(n<o->actual_string_byte_capacity&&o->actual_public_argv[i][n])n++;if(n==o->actual_string_byte_capacity)goto done;o->actual_lengths[i]=n+1;}
 for(unsigned long i=0;i<(unsigned long)(o->actual_public_argc?o->actual_public_argc:1)+8;i++)if(o->actual_private_argv[i])goto done;
 if(argument_decimal(o->descriptor,sizeof o->descriptor,(unsigned long)o->actual_custody_fd,&o->descriptor_length)||
    argument_decimal(o->count,sizeof o->count,(unsigned long)o->actual_public_argc,&o->count_length)||
    argument_decimal(o->security,sizeof o->security,o->actual_security_bytes,&o->security_length)||
    argument_decimal(o->terminal,sizeof o->terminal,o->actual_terminal_calls,&o->terminal_length)||
    argument_decimal(o->representation,sizeof o->representation,o->actual_representation_bytes,&o->representation_length))goto done;
 o->actual_private_argv[0]=o->actual_public_argc?o->actual_public_argv[0]:"";o->actual_private_argv[1]="service-lasso-tui-native-v2";
 o->actual_private_argv[2]=o->descriptor;o->actual_private_argv[3]=o->count;o->actual_private_argv[4]=o->security;o->actual_private_argv[5]=o->terminal;o->actual_private_argv[6]=o->representation;o->actual_private_argv[7]="--";
 for(unsigned long i=1;i<(unsigned long)o->actual_public_argc;i++)o->actual_private_argv[i+7]=o->actual_public_argv[i];
 o->private_count=(unsigned long)(o->actual_public_argc?o->actual_public_argc:1)+7;o->actual_private_argv[o->private_count]=0;o->result=0;
 done:o->returned=1;return o->result;
}
