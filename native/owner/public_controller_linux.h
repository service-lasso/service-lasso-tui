#ifndef SERVICE_LASSO_PUBLIC_CONTROLLER_LINUX_H
#define SERVICE_LASSO_PUBLIC_CONTROLLER_LINUX_H
#include "capture_workers_linux.h"
#include "custody_linux.h"
#include "terminal_linux.h"
#include "owner_channel_linux.h"
#include "stream_routes_linux.h"
#include "namespace_parent_linux.h"
#include "forward_workers_linux.h"
#include "private_arguments.h"
#include "family_events_linux.h"
#include "candidate_preexec_linux.h"
#include "schedule_linux.h"
#include "final_capture_linux.h"
#include "archive_reader_linux.h"
struct sl_linux_public_startup;
struct sl_linux_candidate_preexec;
struct sl_linux_collector_join;
struct sl_linux_service_observation;
typedef struct sl_linux_source_cut {
    struct sl_linux_public_controller *controller_original;
    struct sl_linux_service_observation *service_original;
    sl_linux_root_observation *root_original;
    sl_linux_family_join *family_original;
    struct sl_linux_collector_join *collector_original;
    sl_linux_capture_worker *readers_original;
    sl_linux_forward_worker *forwarders_original;
    unsigned long reader_count,checked_readers;
    sl_linux_terminal_request *requests_original;
    unsigned long request_capacity,checked_requests;
    void *failure_original;
    long result;unsigned attempted,returned;
} sl_linux_source_cut;
typedef struct sl_linux_namespace_target {
    struct sl_linux_public_controller *controller_original;
    struct sl_linux_candidate_target *target_original;
    sl_linux_parent_namespace *namespace_original;
    struct sl_linux_mount_location *location_original;
    struct sl_linux_mount_row *row_original;
    const unsigned char *options_original;unsigned long options_extent,checked_options;
    long result;unsigned attempted,returned,nsdelegate_observed;
} sl_linux_namespace_target;

typedef struct sl_linux_birth_admission {
    struct sl_linux_public_controller *controller_original;
    struct sl_linux_public_startup *startup_original;
    void *failure_original;
    unsigned long checked_policies,checked_edges;
    unsigned stage,attempted,returned;long result;
} sl_linux_birth_admission;
typedef struct sl_linux_public_controller {
    sl_linux_entry entry;
    sl_linux_schedule schedule;
    sl_linux_schedule_wake *collector_wake;
    struct sl_linux_public_startup *startup_original;
    struct sl_linux_candidate_preexec *candidate_preexec_original;
    sl_private_arguments private_arguments;
    const struct sl_linux_capability_domain *capability_domain_original;
    unsigned long credential_status_bytes,credential_status_attempts,credential_group_capacity;
    sl_linux_fd_scan scan;
    sl_linux_fd_map child_map;
    sl_linux_terminal_views terminal;
    sl_linux_stream_route routes[3];
    sl_linux_call private_query,private_set,parent_slave_close;
    struct termios private_state,private_output_state;
    unsigned parent_slave_close_attempted;
    sl_linux_call parent_custody_child_close,parent_gate_child_close;
    unsigned parent_custody_child_close_attempted,parent_gate_child_close_attempted;
    sl_linux_call *parent_close_primary;
    unsigned parent_views_attempted;
    sl_linux_call pty_open,pty_unlock,pty_peer,master_flags,master_nonblocking;
    sl_linux_fd_lift master_lift,slave_lift;
    int pty_master,pty_slave,unlock_value;
    sl_linux_capture_worker *readers;
    sl_linux_forward_worker *forwarders;
    struct sl_linux_service_observation *initial_service,*service_roster;
    unsigned long service_capacity,service_index;
    struct sl_linux_service_observation *authority_observed_by;
    struct sl_linux_service_observation *collector_failure_observed_by;
    struct sl_linux_service_observation *source_failure_observed_by;
    sl_linux_family_worker *family_active,*family_latest_joined;
    sl_linux_root_observation *root_terminal_original;
    sl_linux_family_join *family_dispatch_stopped_by;
    struct sl_linux_service_observation *family_failure_observed_by;
    sl_linux_parent_namespace namespace_observation;
    sl_linux_namespace_target namespace_target;
    sl_linux_birth_admission birth_admission;
    sl_linux_local_collection collection;
    sl_linux_final_capture final_capture;
    sl_linux_archive_readback final_reader_task;
    sl_linux_archive_reader_join *final_reader_joins;
    unsigned long final_reader_join_capacity,final_reader_join_index;
    sl_linux_owner_channel channel;
    pthread_t collector_thread;
    int collector_create_result;
    void *collector_result;
    struct sl_linux_collector_join *collector_joined_by,*collector_join_roster;
    unsigned long collector_join_capacity,collector_join_index;
    _Atomic unsigned collector_entered,collector_returned;
    unsigned collector_create_entered,collector_create_returned;
    unsigned long reader_count;
    long construction_result,birth_result,construction_return_result;
    unsigned construction_returned,construction_body_completed,construction_body_returned;
    struct sl_linux_service_observation *failure_continuation_last;
    long failure_continuation_result;unsigned failure_continuation_attempted,failure_continuation_returned,failure_continuation_exhausted;
    int reader_start_result;
    sl_linux_source_cut *drained_by;
    struct sl_linux_service_observation *drain_last;
    long drain_result;unsigned drain_attempted,drain_returned,drain_exhausted;
    sl_linux_source_cut *capture_cut_original;
    sl_linux_final_capture *capture_original;
    long normal_capture_drain_result,normal_capture_result;
    unsigned normal_capture_attempted,normal_capture_returned;
    unsigned attempted;
} sl_linux_public_controller;
typedef struct sl_linux_collector_join {
    sl_linux_public_controller *controller_original;
    void *joined_result;
    int result;
    sl_linux_owner_channel *channel_original;
    sl_linux_local_collection *collection_original;
    long channel_result,collection_result;
    long interpretation_result;
    unsigned attempted,returned,interpretation_returned;
} sl_linux_collector_join;
typedef struct sl_linux_service_observation {
 sl_linux_public_controller *controller_original;
 sl_linux_root_observation root;
 sl_linux_authority_inspection authority;long authority_result;
 sl_linux_candidate_preexec_inspection selinux;long selinux_result;
 sl_linux_call schedule_yield;long yield_result;
 sl_linux_schedule_wait *schedule_original;long schedule_observe_result,schedule_wait_result;
 sl_linux_reader_join *raw_joins;
 sl_linux_forward_release *forward_releases;
 sl_linux_forward_join *forward_joins;
 unsigned long capacity;
 sl_linux_collector_join *collector_original;
 long collector_result;unsigned collector_exhausted;
 sl_linux_family_worker family;sl_linux_family_join family_join;long family_result;
 long root_result,source_result;
 sl_linux_reader_join *raw_failure_original;sl_linux_forward_join *forward_failure_original;
 sl_linux_source_cut source_cut;long source_cut_result;
 unsigned prepared,attempted,returned;
} sl_linux_service_observation;
long sl_linux_admit_candidate_birth(sl_linux_public_controller *controller);
long sl_linux_interpret_namespace_target(sl_linux_public_controller *controller);
long sl_linux_release_parent_child_views(sl_linux_public_controller *controller);
long sl_linux_construct_contained_producer(sl_linux_public_controller *controller);
int sl_linux_observe_collector_join(sl_linux_collector_join *observation);
long sl_linux_prepare_service_observation(sl_linux_service_observation *observation,sl_linux_public_controller *controller);
long sl_linux_service_sources(sl_linux_service_observation *observation);
sl_linux_service_observation *sl_linux_service_next(sl_linux_public_controller *controller);
long sl_linux_service_until_authority(sl_linux_public_controller *controller);
long sl_linux_continue_failed_sources(sl_linux_public_controller *controller);
long sl_linux_observe_source_cut(sl_linux_source_cut *cut,sl_linux_service_observation *service);
long sl_linux_drain_sources(sl_linux_public_controller *controller);
long sl_linux_capture_drained_sources(sl_linux_public_controller *controller);
#endif
