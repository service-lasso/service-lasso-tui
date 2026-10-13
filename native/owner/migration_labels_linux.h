#ifndef SERVICE_LASSO_MIGRATION_LABELS_LINUX_H
#define SERVICE_LASSO_MIGRATION_LABELS_LINUX_H
#include "selinux_context_linux.h"
#include "selinux_policy_relations.h"
#include "candidate_routes_linux.h"
struct sl_linux_public_startup;
typedef struct {
 struct sl_linux_public_startup *startup_original;
 sl_linux_mount_location *location_original;
 sl_linux_cgroup_children *children_original;
 sl_linux_child_view *view_original;
 sl_linux_cgroup_control_label *control_original;
 unsigned long locations_checked,views_checked,controls_checked,required_controls;
 unsigned kind,attempted,returned;long result;
} sl_linux_control_roster;
long sl_linux_interpret_control_roster(sl_linux_control_roster *);
typedef struct sl_linux_migration_labels {
 struct sl_linux_public_startup *startup_original;
 sl_linux_policy_parse *parse_original;
 unsigned long policy_index,observed_capacity;
 sl_selinux_genfs_types labels;
 sl_linux_selinux_status_sample after_labels;
 long primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_migration_labels;
long sl_linux_interpret_migration_labels(sl_linux_migration_labels *);
typedef struct sl_linux_migration_access {
 struct sl_linux_public_startup *startup_original;
 sl_linux_migration_labels *labels_original;
 unsigned long policy_index;unsigned kind;
 sl_selinux_access_sources inventory;
 sl_linux_selinux_status_sample after_access;
 long primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_migration_access;
long sl_linux_interpret_migration_access(sl_linux_migration_access *);
typedef struct sl_linux_migration_routes {
 struct sl_linux_public_startup *startup_original;
 sl_linux_migration_access *access_original;
 sl_linux_candidate_roles *roles_original;
 sl_linux_selinux_context *owner_original;
 sl_linux_context_reachability *payload_original;
 unsigned long index;
 sl_candidate_route_source *sources;unsigned long capacity,count;
 sl_linux_selinux_status_sample after_routes;
 long primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_migration_routes;
long sl_linux_interpret_migration_routes(sl_linux_migration_routes *);
long sl_linux_interpret_startup_migration_routes(struct sl_linux_public_startup *);
#endif
