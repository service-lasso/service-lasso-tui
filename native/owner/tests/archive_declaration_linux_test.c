/* Actual pure declaration body, synthetic candidate representation, UNRUN. */
#include "../archive_directory_linux.h"
#include <assert.h>
#include <string.h>
static void put64(unsigned char *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(8*i));}
int main(void){
 unsigned char bytes[256]={0};const char *meaning="service-lasso.archive-directory-object.linux.v1";
 memcpy(bytes+24,meaning,strlen(meaning));memcpy(bytes+128,"evidence",8);memcpy(bytes+200,"/archive",8);
 put64(bytes+8,0);put64(bytes+16,8);put64(bytes+160,1);put64(bytes+168,2);put64(bytes+176,3);put64(bytes+184,4);put64(bytes+192,8);
 sl_candidate_record evidence={.field_count=7},declaration={.field_count=3};
 evidence.fields[6]=(sl_candidate_span){bytes+128,8};
 declaration.fields[0]=(sl_candidate_span){bytes,24};declaration.fields[1]=(sl_candidate_span){bytes+24,strlen(meaning)};declaration.fields[2]=(sl_candidate_span){bytes+160,48};
 sl_candidate_profile p={.source=bytes,.source_extent=sizeof(bytes),.original={bytes,sizeof(bytes)},.returned=1};
 p.actual_rows[0]=&evidence;p.actual_rows[3]=&declaration;p.actual_capacities[0]=p.actual_capacities[3]=p.counts[0]=p.counts[3]=1;
 sl_linux_archive_declaration o={0};
 assert(!sl_linux_interpret_archive_declaration(&o,&p,0));
 assert(sl_linux_archive_declaration_matches_original(&o,&p,0));
 assert(o.evidence_original==&evidence&&o.declaration_original==&declaration&&o.path_original.bytes==bytes+200&&o.tuple[3]==4);
 sl_linux_archive_declaration saved=o;
 assert(sl_linux_interpret_archive_declaration(&o,&p,0)<0&&!memcmp(&o,&saved,sizeof(o)));
 for(unsigned mode=0;mode<5;mode++){
  memset(&o,0,sizeof(o));put64(bytes,0);put64(bytes+16,8);put64(bytes+192,8);bytes[201]='a';declaration.fields[2]=(sl_candidate_span){bytes+160,48};
  if(mode==0)put64(bytes,1);
  if(mode==1)put64(bytes+16,9);
  if(mode==2)put64(bytes+192,9);
  if(mode==3)bytes[201]='/';
  if(mode==4)declaration.fields[2]=(sl_candidate_span){bytes+255,48};
  assert(sl_linux_interpret_archive_declaration(&o,&p,0)<0&&o.attempted&&o.returned);
 }
 declaration.fields[2]=(sl_candidate_span){bytes+160,48};bytes[201]='a';put64(bytes,0);put64(bytes+16,8);put64(bytes+192,8);
 memcpy(bytes+208,bytes+160,48);
 sl_candidate_record declarations[2]={declaration,declaration};declarations[1].fields[2]=(sl_candidate_span){bytes+208,48};
 p.actual_rows[3]=declarations;p.actual_capacities[3]=p.counts[3]=2;
 sl_linux_archive_declaration outputs[2]={0};sl_linux_archive_declaration_roster roster={0};
 assert(!sl_linux_interpret_archive_declaration_roster(&roster,&p,outputs,2)&&roster.count==2&&roster.examined==2&&roster.first_index==0);
 assert(sl_linux_archive_declaration_roster_matches_original(&roster));
 outputs[1].tuple[3]=9;assert(!sl_linux_archive_declaration_roster_matches_original(&roster));outputs[1].tuple[3]=4;
 roster.examined=1;assert(!sl_linux_archive_declaration_roster_matches_original(&roster));roster.examined=2;
 put64(bytes+232,9);assert(!sl_linux_archive_declaration_roster_matches_original(&roster));put64(bytes+232,4);
 assert(sl_linux_archive_declaration_roster_matches_original(&roster));
 memset(outputs,0,sizeof(outputs));memset(&roster,0,sizeof(roster));bytes[255]='x';
 assert(sl_linux_interpret_archive_declaration_roster(&roster,&p,outputs,2)<0&&roster.count==2&&outputs[1].returned);
 bytes[255]='e';memset(outputs,0,sizeof(outputs));memset(&roster,0,sizeof(roster));
 assert(sl_linux_interpret_archive_declaration_roster(&roster,&p,outputs,1)<0&&roster.count==1&&roster.examined==2);
 memset(&o,0,sizeof(o));saved=o;
 p.original=(sl_candidate_span){(unsigned char *)&o,sizeof(o)};p.source=p.original.bytes;p.source_extent=p.original.extent;
 assert(sl_linux_interpret_archive_declaration(&o,&p,0)<0&&!memcmp(&o,&saved,sizeof(o)));
 return 0;
}
