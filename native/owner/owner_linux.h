#ifndef SERVICE_LASSO_OWNER_LINUX_H
#define SERVICE_LASSO_OWNER_LINUX_H
#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <linux/sched.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/vfs.h>
#include <signal.h>
#include <sys/resource.h>
#include <linux/capability.h>
#include <linux/stat.h>
#include <sys/un.h>

typedef struct sl_linux_call {
    const char *source;
    long number,arguments[6];
    long result;
    _Atomic unsigned entered, returned;
} sl_linux_call;
/* Original containing acquisition/range, never a nested mmap result. */
typedef struct sl_linux_shared_range {
 const sl_linux_call *mapping_original;
 unsigned long offset,bytes;
} sl_linux_shared_range;
/* Embedded in the prebirth shared bootstrap. Fixed capability cells derive
 * from the explicitly supported v3 two-word representation, not a budget. */
typedef struct sl_linux_credential_snapshot {
    sl_linux_call open,filesystem_query,groups_query,close;
    struct statfs filesystem;
    sl_linux_call *reads;
    unsigned char *status;
    unsigned *groups;
    unsigned long status_capacity,read_capacity,group_capacity;
    unsigned long length,read_count;
    unsigned uids[4],gids[4],group_count;
    int fd;long result;
    unsigned attempted,returned,eof;
} sl_linux_credential_snapshot;
struct sl_linux_capability_domain;
typedef struct sl_linux_preexec_authority {
    const struct sl_linux_capability_domain *domain_original;
    unsigned last_cap,prepared,attempted;
    unsigned long status_bytes,status_attempts,group_capacity;
    sl_linux_call credential_mapping;long credential_mapping_result;
    unsigned long credential_mapping_bytes;
    sl_linux_shared_range credential_ranges[2][3]; /* status/read/group */
    sl_linux_credential_snapshot credentials_before,credentials_after;
    sl_linux_call namespace_before[2],namespace_after[2],unshare;
    struct statx namespaces_before[2],namespaces_after[2];
    sl_linux_call uid_before,uid_after,gid_before,gid_after;
    unsigned uids_before[3],uids_after[3],gids_before[3],gids_after[3];
    sl_linux_call securebits_set,securebits_get,bounding_drop[64],bounding_read[64];
    sl_linux_call ambient_clear,ambient_read[64],cap_before,cap_clear,cap_after;
    struct __user_cap_header_struct header_before,header_clear,header_after;
    struct __user_cap_data_struct caps_before[2],caps_clear[2],caps_after[2];
    sl_linux_call nnp_set,nnp_get;
    long result;
    _Atomic unsigned inspected_ready,returned;
} sl_linux_preexec_authority;
long sl_linux_prepare_preexec_authority(sl_linux_preexec_authority *authority,
    const struct sl_linux_capability_domain *domain);
long sl_linux_apply_preexec_authority(sl_linux_preexec_authority *authority);
long sl_linux_prepare_credential_backing(sl_linux_preexec_authority *authority);
long sl_linux_observe_child_credentials(sl_linux_credential_snapshot *snapshot);
enum sl_linux_fd_action_kind { SL_FD_CLOSE_VIEW=1,SL_FD_DUPLICATE_VIEW=2,SL_FD_CLEAR_EXEC=3 };
typedef struct sl_linux_fd_action {
    enum sl_linux_fd_action_kind kind;
    int source_fd,target_fd;
    void *outer_original;
    sl_linux_call call;
} sl_linux_fd_action;

/* Original native bootstrap output cells must live in an actual parent-owned
 * MAP_SHARED mapping made before clone3. Child writes are genuine borrows of
 * this SAME physical backing, not serialized replacement for child Go objects.
 * Mapping/acquisition/identity and full backing remain native-entry obligations. */
struct sl_linux_candidate_preexec_shared;
typedef struct sl_linux_bootstrap {
    struct clone_args arguments;
    int pidfd;
    sl_linux_call clone_parent,clone_child;
    sl_linux_call create_session,acquire_terminal,gate_close;
    sl_linux_preexec_authority authority;
    struct sl_linux_candidate_preexec_shared *candidate_preexec;
    int controlling_terminal_fd;
    sl_linux_fd_action *fd_actions;
    unsigned long fd_action_count;
    sl_linux_call gate_reads[8];
    unsigned char gate_expected[8],gate_received[8];
    unsigned gate_count;
    sl_linux_call execute;
    sl_linux_call failure_pause;
    _Atomic unsigned fd_actions_prepared,exec_entered, failed;
} sl_linux_bootstrap;
typedef struct sl_linux_birth {
    int cgroup_fd,payload_fd,entry_gate_fd;
    char *const *argv;
    char *const *environment;
    sl_linux_bootstrap *bootstrap;
    unsigned attempted;
} sl_linux_birth;

typedef struct sl_linux_authority_inspection {
    sl_linux_birth *birth_original;
    sl_linux_preexec_authority *authority_original;
    long result;
    unsigned attempted,returned,not_ready,bootstrap_failed;
    sl_linux_fd_action *fd_failure_original;
    const sl_linux_fd_action *fd_inputs_original;
    unsigned long checked_fd_actions;
} sl_linux_authority_inspection;
/* Observes the completed SAME transition only; never grants profile/gate. */
long sl_linux_inspect_preexec_authority(sl_linux_authority_inspection *inspection);
typedef struct sl_linux_root_observation {
    sl_linux_birth *birth_original;
    sl_linux_call wait;
    siginfo_t information;
    struct rusage usage;
    unsigned attempted;
} sl_linux_root_observation;
long sl_linux_observe_root_exit(sl_linux_root_observation *observation);
typedef struct sl_linux_fd_view {
    int fd;
    void *original;
    sl_linux_call inspect_flags;
} sl_linux_fd_view;
typedef struct sl_linux_fd_scan_frame {
    void *scan_original;
    unsigned char *bytes;
    unsigned long capacity;
    sl_linux_call read;
    long raw_count;
} sl_linux_fd_scan_frame;
typedef struct sl_linux_fd_scan {
    sl_linux_call open_directory,filesystem_query;
    struct statfs filesystem;
    int directory_fd;
    sl_linux_fd_scan_frame *frames;
    unsigned long frame_capacity,frame_count;
    sl_linux_fd_view *views;
    unsigned long view_capacity,view_count;
    unsigned attempted,eof;
} sl_linux_fd_scan;
typedef struct sl_linux_fd_map {
    sl_linux_fd_scan *scan_original;
    sl_linux_fd_view *views;
    unsigned long count;
    int payload_fd,custody_fd,pty_slave_fd;
    int standard_fds[3];
    sl_linux_call standard_state[3];
    sl_linux_fd_action *actions;
    unsigned long action_capacity,action_count;
    unsigned attempted;
} sl_linux_fd_map;
typedef struct sl_linux_public_stream {
    int original_fd,held_fd;
    sl_linux_call original_flags,duplicate;
    unsigned observed_absent;
} sl_linux_public_stream;
typedef struct sl_linux_fd_lift {
    int original_fd,held_fd;
    sl_linux_call duplicate,close_original;
    unsigned attempted;
} sl_linux_fd_lift;
typedef struct sl_linux_socket_peer {
 sl_linux_call domain_call,type_call,protocol_call,security_call;
 unsigned char *bytes,*actual_bytes;unsigned long capacity,actual_capacity,extent,examined;
 uint32_t domain_length,type_length,protocol_length,security_length;
 int descriptor,domain,type,protocol;unsigned endpoint_index;
 long domain_result,type_result,protocol_result,security_result,result;
 unsigned attempted,returned,framing_complete;
} sl_linux_socket_peer;
typedef struct sl_linux_socket_state {
 sl_linux_call local_query,peer_query,accept_query;
 struct sockaddr_un local_address,peer_address;
 uint32_t local_length,peer_length,accept_length;int descriptor,accepting;
 long local_result,peer_result,accept_result,result;
 unsigned endpoint_index,attempted,returned,framing_complete;
} sl_linux_socket_state;
typedef struct sl_linux_entry {
    sl_linux_shared_range bootstrap_range,fd_action_range,credential_ranges[2][3];
    sl_linux_public_stream public_streams[3];
    unsigned public_streams_attempted,public_streams_prepared;
    sl_linux_birth birth;
    sl_linux_call shared_mapping,custody_pair,entry_pipe,fd_action_mapping;
    const sl_linux_fd_action *fd_action_inputs;
    unsigned long fd_action_input_count,fd_action_mapping_bytes;
    long fd_action_mapping_result;
    unsigned fd_actions_attempted;
    long mapping_result;
    int custody_raw_endpoints[2],entry_raw_endpoints[2];
    int custody_endpoints[2],entry_endpoints[2];
    sl_linux_fd_lift custody_lifts[2],entry_lifts[2];
    sl_linux_socket_peer custody_peers[2];
    sl_linux_socket_state custody_states[2];
    unsigned attempted;
} sl_linux_entry;
long sl_linux_call6(sl_linux_call *call,const char *source,long number,
                   long a0,long a1,long a2,long a3,long a4,long a5);
long sl_linux_observe_entry_peer(sl_linux_entry *entry,unsigned index);
long sl_linux_observe_entry_socket_state(sl_linux_entry *entry,unsigned index);
long sl_linux_prepare_entry_channels(sl_linux_entry *entry);
long sl_linux_prepare_public_streams(sl_linux_entry *entry);
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd);
long sl_linux_prepare_fd_actions(sl_linux_entry *entry,const sl_linux_fd_action *inputs,unsigned long count);
long sl_linux_build_child_fd_map(sl_linux_entry *entry,sl_linux_fd_map *map);
long sl_linux_scan_current_fds(sl_linux_fd_scan *scan);
long sl_linux_scan_and_build_child_fd_map(sl_linux_entry *entry,sl_linux_fd_scan *scan,sl_linux_fd_map *map);

/* Native call observation ONLY, no admission/family closure/disposition grant.
 * Exact protected cgroup/native credentials/migration exclusion, source/ABI,
 * shared-map identity, closed inherited FD map, PTY and capture readiness must
 * be established by the native entry before calling this constructor. */
long sl_linux_birth_into_cgroup(sl_linux_birth *birth);
#endif
#endif
