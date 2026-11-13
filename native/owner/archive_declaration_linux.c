#if defined(__linux__) && defined(__x86_64__)
#include "archive_directory_linux.h"
#include <errno.h>
static int separate(const void *a,size_t n,const void *b,size_t m){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return a&&b&&n<=UINTPTR_MAX-x&&m<=UINTPTR_MAX-y&&!(x<y+m&&y<x+n);}
static int span(const sl_candidate_profile *p,sl_candidate_span s){uintptr_t base=(uintptr_t)p->original.bytes,at=(uintptr_t)s.bytes;return s.bytes&&at>=base&&s.extent<=p->original.extent&&at-base<=p->original.extent-s.extent;}
static uint64_t u64(const unsigned char *p){uint64_t n=0;for(unsigned i=0;i<8;i++)n|=(uint64_t)p[i]<<(8*i);return n;}
static int path(sl_candidate_span p){
 if(p.extent<2||p.bytes[0]!='/'||p.bytes[p.extent-1]=='/')return 0;
 size_t begin=1;for(size_t i=1;i<=p.extent;i++){
  if(i<p.extent&&!p.bytes[i])return 0;
  if(i==p.extent||p.bytes[i]=='/'){size_t n=i-begin;if(!n||(n==1&&p.bytes[begin]=='.')||(n==2&&p.bytes[begin]=='.'&&p.bytes[begin+1]=='.'))return 0;begin=i+1;}
 }return 1;
}
int sl_linux_archive_candidate_excludes_output(const sl_candidate_profile *p,const void *o,size_t extent){
 if(!extent||!separate(o,extent,p,sizeof(*p))||!p->returned||p->result||
    !p->original.bytes||p->original.bytes!=p->source||p->original.extent!=p->source_extent||
    !separate(o,extent,p->original.bytes,p->original.extent))return 0;
 for(unsigned i=0;i<5;i++){
  if(p->counts[i]>p->actual_capacities[i]||p->actual_capacities[i]>SIZE_MAX/sizeof(sl_candidate_record)||
     (p->actual_capacities[i]&&!separate(o,extent,p->actual_rows[i],p->actual_capacities[i]*sizeof(sl_candidate_record))))return 0;
 }
 return 1;
}
long sl_linux_interpret_archive_declaration(sl_linux_archive_declaration *o,
 const sl_candidate_profile *p,size_t index){
 if(!o||!sl_linux_archive_candidate_excludes_output(p,o,sizeof(*o)))return -EINVAL;
 if(index>=p->counts[3]||!p->actual_rows[3]||!p->actual_rows[0])return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;for(size_t i=0;i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 o->attempted=1;o->result=-EINVAL;o->declaration_original=&p->actual_rows[3][index];
 const sl_candidate_record *d=o->declaration_original;
 static const unsigned char meaning[]="service-lasso.archive-directory-object.linux.v1";
 if(d->field_count!=3||!span(p,d->fields[1])||d->fields[1].extent!=sizeof(meaning)-1)goto done;
 for(size_t i=0;i<sizeof(meaning)-1;i++)if(d->fields[1].bytes[i]!=meaning[i])goto done;
 sl_candidate_span link=d->fields[0],value=d->fields[2];
 if(!span(p,link)||link.extent!=24||!span(p,value)||value.extent<40)goto done;
 o->evidence_index=u64(link.bytes);o->statement_offset=u64(link.bytes+8);o->statement_extent=u64(link.bytes+16);
 if(o->evidence_index>=p->counts[0]||!o->statement_extent)goto done;
 o->evidence_original=&p->actual_rows[0][(size_t)o->evidence_index];
 sl_candidate_span body=o->evidence_original->fields[6];
 if(o->evidence_original->field_count!=7||!span(p,body)||o->statement_offset>body.extent||o->statement_extent>body.extent-o->statement_offset)goto done;
 o->statement_original=(sl_candidate_span){body.bytes+(size_t)o->statement_offset,(size_t)o->statement_extent};
 for(unsigned i=0;i<4;i++)o->tuple[i]=u64(value.bytes+8*i);
 uint64_t extent=u64(value.bytes+32);
 if(o->tuple[0]>UINT32_MAX||o->tuple[1]>UINT32_MAX||!o->tuple[2]||!o->tuple[3]||extent>SIZE_MAX||extent!=value.extent-40)goto done;
 o->path_original=(sl_candidate_span){value.bytes+40,(size_t)extent};if(!path(o->path_original))goto done;
 o->result=0;
 done:o->returned=1;return o->result;
}
long sl_linux_interpret_archive_declaration_roster(sl_linux_archive_declaration_roster *o,
 const sl_candidate_profile *p,sl_linux_archive_declaration *rows,size_t capacity){
 if(!o||!capacity||capacity>SIZE_MAX/sizeof(*rows)||
    !separate(o,sizeof(*o),rows,capacity*sizeof(*rows))||
    !sl_linux_archive_candidate_excludes_output(p,o,sizeof(*o))||
    !sl_linux_archive_candidate_excludes_output(p,rows,capacity*sizeof(*rows)))return -EINVAL;
 const unsigned char *fresh=(const unsigned char *)o;for(size_t i=0;i<sizeof(*o);i++)if(fresh[i])return -EINVAL;
 fresh=(const unsigned char *)rows;for(size_t i=0;i<capacity*sizeof(*rows);i++)if(fresh[i])return -EINVAL;
 o->attempted=1;o->candidate_original=p;o->rows_original=rows;o->capacity=capacity;o->result=-EINVAL;
 static const unsigned char meaning[]="service-lasso.archive-directory-object.linux.v1";
 for(size_t i=0;i<p->counts[3];i++){
  const sl_candidate_record *d=&p->actual_rows[3][i];o->examined=i+1;
  if(d->field_count!=3)continue;
  if(!span(p,d->fields[1]))goto complete;
  if(d->fields[1].extent!=sizeof(meaning)-1)continue;
  unsigned matches=1;for(size_t j=0;j<sizeof(meaning)-1;j++)if(d->fields[1].bytes[j]!=meaning[j]){matches=0;break;}
  if(!matches)continue;
  if(o->count>=capacity){o->result=-EOVERFLOW;goto complete;}
  if(!o->count)o->first_index=i;
  sl_linux_archive_declaration *row=&rows[o->count++];
  long result=sl_linux_interpret_archive_declaration(row,p,i);if(result){o->result=result;goto complete;}
  if(o->count>1){const sl_linux_archive_declaration *first=&rows[0];
   for(unsigned j=0;j<4;j++)if(row->tuple[j]!=first->tuple[j])goto complete;
   if(row->path_original.extent!=first->path_original.extent)goto complete;
   for(size_t j=0;j<row->path_original.extent;j++)if(row->path_original.bytes[j]!=first->path_original.bytes[j])goto complete;
  }
 }
 if(o->count)o->result=0;
 complete:o->returned=1;return o->result;
}
int sl_linux_archive_declaration_matches_original(const sl_linux_archive_declaration *o,
 const sl_candidate_profile *p,size_t index){
 if(!o||!sl_linux_archive_candidate_excludes_output(p,o,sizeof(*o))||
    o->attempted!=1||o->returned!=1||o->result||index>=p->counts[3]||!p->actual_rows[3]||!p->actual_rows[0]||
    o->declaration_original!=&p->actual_rows[3][index])return 0;
 const sl_candidate_record *d=o->declaration_original;
 static const unsigned char meaning[]="service-lasso.archive-directory-object.linux.v1";
 if(d->field_count!=3||!span(p,d->fields[1])||d->fields[1].extent!=sizeof(meaning)-1)return 0;
 for(size_t i=0;i<sizeof(meaning)-1;i++)if(d->fields[1].bytes[i]!=meaning[i])return 0;
 sl_candidate_span link=d->fields[0],value=d->fields[2];
 if(!span(p,link)||link.extent!=24||!span(p,value)||value.extent<40||
    o->evidence_index!=u64(link.bytes)||o->statement_offset!=u64(link.bytes+8)||o->statement_extent!=u64(link.bytes+16)||
    o->evidence_index>=p->counts[0]||!o->statement_extent||o->evidence_original!=&p->actual_rows[0][(size_t)o->evidence_index])return 0;
 sl_candidate_span body=o->evidence_original->fields[6];
 if(o->evidence_original->field_count!=7||!span(p,body)||o->statement_offset>body.extent||o->statement_extent>body.extent-o->statement_offset||
    o->statement_original.bytes!=body.bytes+(size_t)o->statement_offset||o->statement_original.extent!=o->statement_extent)return 0;
 for(unsigned i=0;i<4;i++)if(o->tuple[i]!=u64(value.bytes+8*i))return 0;
 uint64_t extent=u64(value.bytes+32);
 return o->tuple[0]<=UINT32_MAX&&o->tuple[1]<=UINT32_MAX&&o->tuple[2]&&o->tuple[3]&&extent<=SIZE_MAX&&extent==value.extent-40&&
    o->path_original.bytes==value.bytes+40&&o->path_original.extent==extent&&path(o->path_original);
}
int sl_linux_archive_declaration_roster_matches_original(const sl_linux_archive_declaration_roster *o){
 if(!o||!o->capacity||o->capacity>SIZE_MAX/sizeof(*o->rows_original)||
    !separate(o,sizeof(*o),o->rows_original,o->capacity*sizeof(*o->rows_original))||
    !sl_linux_archive_candidate_excludes_output(o->candidate_original,o,sizeof(*o))||
    !sl_linux_archive_candidate_excludes_output(o->candidate_original,o->rows_original,o->capacity*sizeof(*o->rows_original))||
    o->attempted!=1||o->returned!=1||o->result||!o->count||o->count>o->capacity)return 0;
 const sl_candidate_profile *p=o->candidate_original;size_t count=0;
 static const unsigned char meaning[]="service-lasso.archive-directory-object.linux.v1";
 for(size_t i=0;i<p->counts[3];i++){
  const sl_candidate_record *d=&p->actual_rows[3][i];if(d->field_count!=3)continue;
  if(!span(p,d->fields[1]))return 0;if(d->fields[1].extent!=sizeof(meaning)-1)continue;
  unsigned matches=1;for(size_t j=0;j<sizeof(meaning)-1;j++)if(d->fields[1].bytes[j]!=meaning[j]){matches=0;break;}if(!matches)continue;
  if(count>=o->count||(!count&&o->first_index!=i)||!sl_linux_archive_declaration_matches_original(&o->rows_original[count],p,i))return 0;
  const sl_linux_archive_declaration *row=&o->rows_original[count++],*first=&o->rows_original[0];
  for(unsigned j=0;j<4;j++)if(row->tuple[j]!=first->tuple[j])return 0;
  if(row->path_original.extent!=first->path_original.extent)return 0;
  for(size_t j=0;j<row->path_original.extent;j++)if(row->path_original.bytes[j]!=first->path_original.bytes[j])return 0;
 }
 return count==o->count&&o->examined==p->counts[3];
}
#endif
