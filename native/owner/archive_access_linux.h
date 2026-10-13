#ifndef SERVICE_LASSO_ARCHIVE_ACCESS_LINUX_H
#define SERVICE_LASSO_ARCHIVE_ACCESS_LINUX_H
#include "candidate_routes_linux.h"
struct sl_linux_public_startup;
enum {SL_ARCHIVE_PURPOSE_COUNT=25,SL_ARCHIVE_CREATION_PURPOSE_FIRST=19};
typedef struct sl_linux_archive_creation {
 struct sl_linux_public_startup *startup_original;unsigned long policy_index;
 sl_linux_selinux_context *directory_original,*owner_original,*fscreate_original;
 sl_selinux_creation_types types;
 sl_linux_selinux_status_sample after_creation;
 long primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_archive_creation;
typedef struct sl_linux_archive_source {
 sl_candidate_route_source relation;
 sl_linux_selinux_context *owner_original;
 unsigned conditional_closed_owner;
} sl_linux_archive_source;
typedef struct sl_linux_archive_access {
 struct sl_linux_public_startup *startup_original;
 unsigned long policy_index,purpose;
 sl_linux_selinux_context *context_original,*owner_original;
 sl_linux_archive_creation *creation_original;
 sl_linux_candidate_roles *roles_original;
 sl_linux_context_reachability *payload_original;
 sl_selinux_access_sources inventory;
 sl_linux_archive_source *sources,*actual_sources;unsigned long capacity,actual_capacity,count;
 sl_linux_selinux_status_sample after_access;
 const char *class_original,*permission_original;
 long inventory_result,primary_result,status_result,result;
 unsigned attempted,returned;
} sl_linux_archive_access;
long sl_linux_interpret_archive_access(sl_linux_archive_access *root);
long sl_linux_interpret_archive_accesses(struct sl_linux_public_startup *startup);
long sl_linux_interpret_archive_creation(sl_linux_archive_creation *root);
#endif
