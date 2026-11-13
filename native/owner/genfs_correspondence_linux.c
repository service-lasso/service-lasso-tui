#include "genfs_correspondence_linux.h"
#include <errno.h>
static uint64_t cohort_le(const unsigned char *p,unsigned n){uint64_t v=0;for(unsigned i=0;i<n;i++)v|=(uint64_t)p[i]<<(8*i);return v;}
static int cohort_equal(sl_candidate_span a,sl_candidate_span b){if(!a.bytes||!b.bytes||a.extent!=b.extent)return 0;for(size_t i=0;i<a.extent;i++)if(a.bytes[i]!=b.bytes[i])return 0;return 1;}
long sl_linux_interpret_genfs_correspondence(sl_linux_genfs_correspondence *o){
 static const unsigned char meaning[]="service-lasso.linux.cgroup-genfs-initial-cohort.v1";
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->primary_result=o->result=-EINVAL;
 o->actual_declarations=o->declarations;o->actual_capacity=o->capacity;
 sl_linux_candidate_policy *binding=o->policy_original;
 if(!binding||!binding->returned||binding->result||binding->status_result||binding->failure||!binding->actual_parse||!binding->actual_candidate||o->count||o->after_correspondence.attempted||o->after_correspondence.status_original)goto done;
 sl_candidate_profile *c=binding->actual_candidate;o->candidate_original=c;
 if(!c->returned||c->result||!c->actual_rows[0]||!c->actual_rows[3]||c->counts[3]>o->actual_capacity||(o->actual_capacity&&!o->actual_declarations))goto done;
 o->after_correspondence.status_original=binding->after_binding.status_original;
 if(!o->after_correspondence.status_original)goto done;
 for(size_t i=0;i<o->actual_capacity;i++)if(o->actual_declarations[i].attempted||o->actual_declarations[i].declaration_original)goto sampled;
 for(size_t i=0;i<c->counts[3];i++){
  const sl_candidate_record *d=&c->actual_rows[3][i];o->examined=i+1;
  if(d->field_count!=3||!cohort_equal(d->fields[1],(sl_candidate_span){meaning,sizeof(meaning)-1}))continue;
  sl_linux_genfs_declaration *r=&o->actual_declarations[o->count++];r->attempted=1;r->result=-EINVAL;r->declaration_original=d;r->interpretation_original=d->fields[2];
  sl_candidate_span link=d->fields[0],value=r->interpretation_original;
  if(!link.bytes||link.extent!=24||!value.bytes||value.extent!=136||cohort_le(value.bytes,4)!=1||cohort_le(value.bytes+4,4))goto failed;
  r->evidence_index=cohort_le(link.bytes,8);r->statement_offset=cohort_le(link.bytes+8,8);r->statement_extent=cohort_le(link.bytes+16,8);
  if(r->evidence_index>=c->counts[0]||!r->statement_extent)goto failed;
  r->evidence_original=&c->actual_rows[0][(size_t)r->evidence_index];sl_candidate_span body=r->evidence_original->fields[6];
  if(r->evidence_original->field_count!=7||!body.bytes||r->statement_offset>body.extent||r->statement_extent>body.extent-r->statement_offset)goto failed;
  r->statement_original=(sl_candidate_span){body.bytes+(size_t)r->statement_offset,(size_t)r->statement_extent};
  sl_candidate_span expected[4]={c->policy_identity,c->kernel_interpretation,c->source_identity,c->layout_identity};
  for(unsigned j=0;j<4;j++){r->identities[j]=(sl_candidate_span){value.bytes+8+32*j,32};r->identity_checked[j]=1;if(!cohort_equal(r->identities[j],expected[j]))goto failed;}
  if(!cohort_equal(r->identities[0],binding->declared_identity)||!cohort_equal(r->identities[1],binding->kernel_interpretation_original))goto failed;
  r->result=0;r->returned=1;continue;
  failed:r->returned=1;goto sampled;
 }
 if(o->count)o->primary_result=0;
 sampled:o->status_result=sl_linux_sample_selinux_status(&o->after_correspondence);
 if(!o->status_result&&!sl_linux_equal_selinux_status(&binding->after_binding,&o->after_correspondence))o->status_result=-EINVAL;
 o->result=o->primary_result?o->primary_result:o->status_result;
 done:o->returned=1;return o->result;
}
