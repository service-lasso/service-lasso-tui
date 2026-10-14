#ifndef SERVICE_LASSO_BOUND_DOMAINS_LINUX_H
#define SERVICE_LASSO_BOUND_DOMAINS_LINUX_H
#include "selinux_context_linux.h"
enum sl_linux_domain_kind {SL_DOMAIN_UNVISITED=0,SL_DOMAIN_OWNER,SL_DOMAIN_ATTRIBUTE,SL_DOMAIN_UNBOUNDED,SL_DOMAIN_BOUNDED};
typedef struct sl_linux_domain_entry {
 type_datum_t *type_original;unsigned index,observed_bounds,observed_flavor;
 unsigned long first_step,step_count;enum sl_linux_domain_kind kind;
} sl_linux_domain_entry;
typedef struct sl_linux_domain_step {
 sl_linux_domain_entry *entry_original;type_datum_t *type_original;
 unsigned index,bounds;
} sl_linux_domain_step;
typedef struct sl_linux_bound_domains {
 sl_linux_selinux_context *context_original,*actual_context;policydb_t *actual_policy;
 sl_linux_domain_entry *entries,*actual_entries;unsigned long capacity,actual_capacity,count,bounded_count;
 sl_linux_domain_step *steps,*actual_steps;unsigned long step_capacity,actual_step_capacity,step_count;
 unsigned owner_type;type_datum_t *owner_original;sl_linux_selinux_status_sample after_domains;int status_result,result;unsigned attempted,returned;
} sl_linux_bound_domains;
/* Read-only complete candidate interpretation, not a selected domain/profile.
 * Actual parsed objects and source-owned distinct visit backing stay retained. */
int sl_linux_interpret_bound_domains(sl_linux_bound_domains *root);
#endif