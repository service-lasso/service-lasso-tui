/* UNRUN actual source body, synthetic original C inputs; no native entry proof. */
#if defined(__linux__) && defined(__x86_64__)
#include <assert.h>
#include <string.h>
#include "../public_inputs_linux.c"
static void setup(sl_linux_public_inputs *o,sl_linux_public_input_source *s,
 sl_linux_public_string *a,sl_linux_public_string *e,char **argv,char **env){
 memset(o,0,sizeof(*o));memset(a,0,2*sizeof(*a));memset(e,0,2*sizeof(*e));
 *s=(sl_linux_public_input_source){2,argv,env,2,2,16,16};
 o->source_original=s;o->arguments=a;o->environment=e;
}
int main(void){
 char first[]="tool",second[]="",setting[]="A=B";
 char *argv[3]={first,second,0},*env[3]={setting,first,0};
 sl_linux_public_inputs o,saved;sl_linux_public_input_source source;
 sl_linux_public_string arguments[2],environment[2];
 setup(&o,&source,arguments,environment,argv,env);
 assert(sl_linux_public_input_source_excludes_output(&source,&o,sizeof(o)));
 assert(!sl_linux_public_input_source_excludes_output(&source,argv,sizeof(argv)));
 assert(!sl_linux_public_input_source_excludes_output(&source,env,sizeof(env)));
 assert(!sl_linux_public_input_source_excludes_output(&source,first,sizeof(first)));
 assert(!sl_linux_capture_public_inputs(&o)&&sl_linux_public_inputs_match_original(&o));
 assert(o.argument_count==2&&o.environment_count==2&&o.argument_extent==6&&o.environment_extent==9);
 assert(arguments[0].bytes==first&&arguments[0].units==5&&arguments[1].units==1&&environment[1].bytes==first);
 saved=o;assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o)));
 arguments[0].units--;assert(!sl_linux_public_inputs_match_original(&o));arguments[0].units++;
 unsigned char separate=0;assert(sl_linux_public_inputs_exclude_output(&o,&separate,1));
 assert(!sl_linux_public_inputs_exclude_output(&o,setting,sizeof(setting)));
 setup(&o,&source,arguments,environment,argv,env);source.argc=0;source.argv=0;source.environment=0;
 assert(!sl_linux_capture_public_inputs(&o)&&!o.argument_count&&!o.environment_count);
 setup(&o,&source,arguments,environment,argv,env);source.argument_bytes=4;saved=o;
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o)));
 setup(&o,&source,arguments,environment,argv,env);source.environment_capacity=1;saved=o;
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o)));
 setup(&o,&source,arguments,environment,argv,env);o.arguments=(void*)&source;saved=o;
 sl_linux_public_input_source source_saved=source;
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o))&&!memcmp(&source,&source_saved,sizeof(source)));
 setup(&o,&source,arguments,environment,argv,env);argv[0]=(char*)arguments;saved=o;
 sl_linux_public_string views_saved[2];memcpy(views_saved,arguments,sizeof(views_saved));
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o))&&!memcmp(views_saved,arguments,sizeof(views_saved)));
 argv[0]=first;setup(&o,&source,arguments,environment,argv,env);env[0]=0;
 assert(!sl_linux_capture_public_inputs(&o)&&o.argument_count==2&&!o.environment_count);env[0]=setting;
 /* Exact source capacity is allowed when its next original slot is NULL. */
 setup(&o,&source,arguments,environment,argv,env);assert(!sl_linux_capture_public_inputs(&o)&&o.environment_count==source.environment_capacity);
 setup(&o,&source,arguments,environment,argv,env);env[2]=setting;saved=o;
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o)));env[2]=0;
 setup(&o,&source,arguments,environment,argv,env);source.environment_bytes=8;saved=o;
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o)));
 setup(&o,&source,arguments,environment,argv,env);o.arguments=(void*)argv;saved=o;
 char *argv_saved[3];memcpy(argv_saved,argv,sizeof(argv_saved));
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o))&&!memcmp(argv_saved,argv,sizeof(argv_saved)));
 setup(&o,&source,arguments,environment,argv,env);o.environment=(void*)env;saved=o;
 char *env_saved[3];memcpy(env_saved,env,sizeof(env_saved));
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o))&&!memcmp(env_saved,env,sizeof(env_saved)));
 setup(&o,&source,arguments,environment,argv,env);env[0]=(char*)environment;saved=o;
 memcpy(views_saved,environment,sizeof(views_saved));
 assert(sl_linux_capture_public_inputs(&o)==-EINVAL&&!memcmp(&o,&saved,sizeof(o))&&!memcmp(views_saved,environment,sizeof(views_saved)));
 env[0]=setting;argv[0]=first;setup(&o,&source,arguments,environment,argv,env);
 source.argc=0;source.argv=0;source.environment=0;
 assert(!sl_linux_capture_entry_public_inputs(&o,&source,2,argv,env)&&sl_linux_public_inputs_match_original(&o));
 assert(source.argc==2&&source.argv==argv&&source.environment==env);
 saved=o;source_saved=source;
 assert(sl_linux_capture_entry_public_inputs(&o,&source,2,argv,env)==-EINVAL&&
 !memcmp(&o,&saved,sizeof(o))&&!memcmp(&source,&source_saved,sizeof(source)));
 setup(&o,&source,arguments,environment,argv,env);source.argc=0;source.argv=0;source.environment=0;
 argv[0]=(char*)&source;saved=o;source_saved=source;
 assert(sl_linux_capture_entry_public_inputs(&o,&source,2,argv,env)==-EINVAL&&
 !memcmp(&o,&saved,sizeof(o))&&!memcmp(&source,&source_saved,sizeof(source)));
 argv[0]=first;setup(&o,&source,arguments,environment,argv,env);source.argc=0;source.argv=0;source.environment=0;
 saved=o;source_saved=source;
 assert(sl_linux_capture_entry_public_inputs(&o,&source,0,(char**)&source,env)==-EINVAL&&
 !memcmp(&o,&saved,sizeof(o))&&!memcmp(&source,&source_saved,sizeof(source)));
 return 0;
}
#endif