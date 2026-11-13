#include "candidate_profile.h"
#include <errno.h>
#include <limits.h>
static uint64_t candidate_le(const unsigned char *p,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;i++)v|=(uint64_t)p[i]<<(8*i);return v;}
static long candidate_finish(sl_candidate_profile *p,long result){p->result=result;p->returned=1;return result;}
static int candidate_equal(const unsigned char *a,const unsigned char *b,size_t n){for(size_t i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
static long candidate_records(sl_candidate_profile *p,unsigned index,unsigned fields){
 sl_candidate_span section=p->sections[index+4];size_t at=8;
 if(section.extent<8)return -EINVAL;
 uint64_t count=candidate_le(section.bytes,8);
 if(count>p->actual_capacities[index]||count>SIZE_MAX)return -EOVERFLOW;
 p->counts[index]=(size_t)count;
 for(size_t i=0;i<(size_t)count;i++){
  p->record_index=i;
  if(at>section.extent||section.extent-at<(size_t)fields*8)return -EINVAL;
  sl_candidate_record *row=&p->actual_rows[index][i];size_t begin=at;
  row->field_count=fields;const unsigned char *lengths=section.bytes+at;at+=(size_t)fields*8;
  for(unsigned j=0;j<fields;j++){
   p->field_index=j;uint64_t n=candidate_le(lengths+j*8,8);
   if(n>SIZE_MAX||n>section.extent-at)return -EOVERFLOW;
   row->fields[j].bytes=section.bytes+at;row->fields[j].extent=(size_t)n;at+=(size_t)n;
  }
  row->original.bytes=section.bytes+begin;row->original.extent=at-begin;
 }
 return at==section.extent?0:-EINVAL;
}
long sl_candidate_decode_profile(sl_candidate_profile *p){
 static const unsigned char magic[8]={0x53,0x4c,0x43,0x50,0x52,0x4f,0x46,0x31};
 if(!p||p->attempted)return -EINVAL;p->attempted=1;
 p->original.bytes=p->source;p->original.extent=p->source_extent;
 for(unsigned i=0;i<5;i++){
  p->actual_rows[i]=p->rows[i];p->actual_capacities[i]=p->capacities[i];
  if(p->capacities[i]&&!p->rows[i])return candidate_finish(p,-EINVAL);
 }
 const unsigned char *bytes=p->original.bytes;size_t length=p->original.extent,at=32;
 if(!bytes||length<32||!candidate_equal(bytes,magic,8)||candidate_le(bytes+8,4)!=1||candidate_le(bytes+12,4)!=9||candidate_le(bytes+16,8)!=length||candidate_le(bytes+24,8)!=0)return candidate_finish(p,-EINVAL);
 for(unsigned i=0;i<9;i++){
  p->section_index=i;p->cursor=at;
  if(at>length||length-at<16||candidate_le(bytes+at,4)!=i+1||candidate_le(bytes+at+4,4)!=0)return candidate_finish(p,-EINVAL);
  uint64_t n=candidate_le(bytes+at+8,8);at+=16;
  if(n>SIZE_MAX||n>length-at)return candidate_finish(p,-EOVERFLOW);
  p->sections[i].bytes=bytes+at;p->sections[i].extent=(size_t)n;at+=(size_t)n;
 }
 p->cursor=at;
 if(at!=length||p->sections[0].extent!=32||p->sections[1].extent!=32||p->sections[2].extent!=72||p->sections[3].extent!=24)return candidate_finish(p,-EINVAL);
 p->source_identity=p->sections[0];p->layout_identity=p->sections[1];
 p->policy_identity=(sl_candidate_span){p->sections[2].bytes,32};p->policy_extent=candidate_le(p->sections[2].bytes+32,8);
 p->kernel_interpretation=(sl_candidate_span){p->sections[2].bytes+40,32};
 p->security_label_bytes=candidate_le(p->sections[3].bytes,8);p->terminal_calls=candidate_le(p->sections[3].bytes+8,8);p->representation_bytes=candidate_le(p->sections[3].bytes+16,8);
 if(!p->security_label_bytes||!p->terminal_calls||!p->representation_bytes)return candidate_finish(p,-EINVAL);
 static const unsigned fields[5]={7,8,3,3,4};
 for(unsigned i=0;i<5;i++){long result=candidate_records(p,i,fields[i]);if(result)return candidate_finish(p,result);}
 for(size_t i=0;i<p->counts[0];i++){
  sl_candidate_record *row=&p->actual_rows[0][i];for(unsigned j=0;j<7;j++)if(j!=4&&!row->fields[j].extent)return candidate_finish(p,-EINVAL);
 }
 for(size_t i=0;i<p->counts[1];i++){
  sl_candidate_record *row=&p->actual_rows[1][i];for(unsigned j=0;j<6;j++)if(row->fields[j].extent!=8)return candidate_finish(p,-EINVAL);
  row->evidence_index=candidate_le(row->fields[0].bytes,8);row->statement_offset=candidate_le(row->fields[1].bytes,8);row->statement_extent=candidate_le(row->fields[2].bytes,8);
  row->policy_user=candidate_le(row->fields[3].bytes,8);row->policy_role=candidate_le(row->fields[4].bytes,8);row->policy_type=candidate_le(row->fields[5].bytes,8);
  if(row->evidence_index>=p->counts[0]||!row->statement_extent||!row->fields[6].extent||!row->fields[7].extent)return candidate_finish(p,-EINVAL);
  sl_candidate_record *evidence=&p->actual_rows[0][(size_t)row->evidence_index];row->evidence_original=evidence;
  sl_candidate_span body=evidence->fields[6];
  if(row->statement_offset>body.extent||row->statement_extent>body.extent-row->statement_offset)return candidate_finish(p,-EOVERFLOW);
  row->statement_original=(sl_candidate_span){body.bytes+(size_t)row->statement_offset,(size_t)row->statement_extent};
 }
 return candidate_finish(p,0);
}