#include <assert.h>
#include <string.h>
#include "../private_arguments.c"
/* UNRUN source-only fixture; geometry values are test inputs, not product fit. */
static unsigned long security=256,terminal=3,representation=64;
static void setup(sl_private_arguments *o,char **public,char **private,unsigned long *lengths){
 memset(o,0,sizeof *o);memset(private,0,12*sizeof *private);memset(lengths,0,4*sizeof *lengths);
 o->public_argc=4;o->public_argv=public;o->custody_fd=19;o->private_argv=private;o->pointer_capacity=12;
 o->security_quantity_original=&security;o->terminal_quantity_original=&terminal;o->representation_quantity_original=&representation;
 o->lengths=lengths;o->length_capacity=4;o->string_byte_capacity=256;
}
int main(void){
 char *public[]={"/installed/service-lasso-tui","--api","https://runtime.example","--compat-v1",0};
 char *private[12];unsigned long lengths[4];sl_private_arguments o;setup(&o,public,private,lengths);
 assert(sl_prepare_private_arguments(&o)==0&&o.private_count==11&&private[11]==0);
 assert(private[0]==public[0]&&private[8]==public[1]&&private[10]==public[3]);
 assert(!strcmp(private[1],"service-lasso-tui-native-v2")&&!strcmp(private[2],"19")&&!strcmp(private[3],"4")&&!strcmp(private[7],"--"));
 assert(!strcmp(private[4],"256")&&!strcmp(private[5],"3")&&!strcmp(private[6],"64")&&o.actual_terminal_calls==terminal);
 assert(lengths[0]==strlen(public[0])+1&&o.actual_public_argv==public);
 assert(sl_prepare_private_arguments(&o)<0&&private[8]==public[1]);
 setup(&o,public,private,lengths);o.pointer_capacity=11;assert(sl_prepare_private_arguments(&o)<0&&!private[0]&&!lengths[0]);
 setup(&o,public,private,lengths);o.string_byte_capacity=2;assert(sl_prepare_private_arguments(&o)<0&&!private[0]&&!lengths[0]);
 setup(&o,public,private,lengths);o.private_argv=public;assert(sl_prepare_private_arguments(&o)<0&&!strcmp(public[1],"--api")&&!lengths[0]);
 setup(&o,public,private,lengths);char *saved=public[3];public[3]=(char*)lengths;
 assert(sl_prepare_private_arguments(&o)<0&&!lengths[0]&&!private[0]);public[3]=saved;
 setup(&o,public,private,lengths);unsigned long huge=ULONG_MAX;o.terminal_quantity_original=&huge;
 assert(sl_prepare_private_arguments(&o)<0&&!private[0]&&!lengths[0]);
 setup(&o,public,private,lengths);unsigned long zero=0;o.representation_quantity_original=&zero;
 assert(sl_prepare_private_arguments(&o)<0&&!private[0]&&!lengths[0]);
 setup(&o,public,private,lengths);o.security_quantity_original=(unsigned long*)lengths;
 assert(sl_prepare_private_arguments(&o)<0&&!private[0]&&!lengths[0]);
 sl_private_arguments unchanged;
 setup(&o,public,private,lengths);o.security_quantity_original=&o.string_byte_capacity;unchanged=o;
 assert(sl_prepare_private_arguments(&o)<0&&!memcmp(&o,&unchanged,sizeof(o))&&!private[0]&&!lengths[0]);
 setup(&o,public,private,lengths);saved=public[3];public[3]=o.descriptor;unchanged=o;
 assert(sl_prepare_private_arguments(&o)<0&&!memcmp(&o,&unchanged,sizeof(o))&&!private[0]&&!lengths[0]);public[3]=saved;
 setup(&o,public,private,lengths);o.public_argv=(char**)&o.private_argv;unchanged=o;
 assert(sl_prepare_private_arguments(&o)<0&&!memcmp(&o,&unchanged,sizeof(o))&&!private[0]&&!lengths[0]);
 setup(&o,public,private,lengths);o.public_argc=0;o.public_argv=0;
 assert(!sl_prepare_private_arguments(&o)&&o.private_count==8&&!strcmp(private[0],"")&&!strcmp(private[3],"0")&&!private[8]&&!o.public_bytes&&!lengths[0]&&!o.actual_public_argv);
 setup(&o,public,private,lengths);o.public_argc=0;o.public_argv=0;o.pointer_capacity=8;unchanged=o;
 assert(sl_prepare_private_arguments(&o)<0&&!memcmp(&o,&unchanged,sizeof(o))&&!private[0]);
 return 0;
}
