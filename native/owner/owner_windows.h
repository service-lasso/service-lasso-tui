#ifndef SERVICE_LASSO_OWNER_WINDOWS_H
#define SERVICE_LASSO_OWNER_WINDOWS_H
#ifdef _WIN32
#include <windows.h>
#include <stddef.h>

/* These are actual source cells, not serializable ownership certificates.
 * Their caller must own the entire structure before entering any operation.
 * No numeric handle/return value grants identity, cleanup or public success. */
typedef struct sl_win_call {
    const char *source;
    void *subject;
    ULONG_PTR result;
    DWORD last_error;
    unsigned entered, returned;
} sl_win_call;

typedef struct sl_win_object_security {
 void *backing,*actual_backing;DWORD capacity,actual_capacity,required;
 HANDLE object_original;sl_win_call query;
 SECURITY_INFORMATION requested;
 unsigned attempted,returned;int result;
} sl_win_object_security;
/* Read-only observation; no access exclusion or admission grant. */
int sl_win_observe_object_security(sl_win_object_security *root,HANDLE object);
typedef struct sl_win_token_observation {
 HANDLE process_original,token;void *backing,*actual_backing;DWORD capacity,actual_capacity,required;
 sl_win_call open,before_query,information_query,after_query;
 TOKEN_STATISTICS before,after;DWORD before_required,after_required;
 int primary_result,after_result,result;unsigned attempted,returned;
} sl_win_token_observation;
int sl_win_observe_process_token(sl_win_token_observation *root,HANDLE process);
typedef struct sl_win_birth {
    sl_win_token_observation tokens[2],*token_failure_original;
    sl_win_object_security security[6];
    HANDLE owner_process;sl_win_call owner_process_query;
    sl_win_object_security *security_failure_original;
    HANDLE job;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
    HANDLE inherited_child_endpoint;
    HANDLE inherited_entry_gate;
    HANDLE inherited_handles[2];
    STARTUPINFOEXW startup;
    PROCESS_INFORMATION process;
    LPPROC_THREAD_ATTRIBUTE_LIST attributes;
    void *attribute_backing;
    SIZE_T attribute_capacity, attribute_required;
    const WCHAR *payload_path;
    const WCHAR *command_line_original;
    WCHAR *command_line;
    SIZE_T command_line_units;
    void *environment;
    const WCHAR *directory;
    sl_win_call create_job, set_limits;
    sl_win_call attribute_query, attribute_initialize;
    sl_win_call attribute_job, attribute_handles;
    sl_win_call creator_thread_query,creator_thread_pseudo_query,creator_thread_duplicate,create_process;
    HANDLE creator_thread_pseudo,held_creator_thread;
    DWORD creator_thread_id,creation_flags;
    unsigned attempted;
} sl_win_birth;

typedef struct sl_win_root_observation {
    sl_win_birth *birth_original;
    sl_win_call wait,exit_code;
    DWORD code;
    unsigned attempted;
} sl_win_root_observation;
typedef struct sl_win_family_observation {
    sl_win_birth *birth_original;
    sl_win_call query;
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting;
    DWORD returned_length;
    unsigned attempted;
} sl_win_family_observation;
typedef struct sl_win_service_observation {
 sl_win_birth *birth_original,*actual_birth;
 sl_win_root_observation root;
 sl_win_family_observation family;
 int root_result,family_result,result;
 unsigned root_ready,family_observed;
 unsigned attempted,returned;
} sl_win_service_observation;
/* Independently observes actual process/job; never a resume/release issuer. */
int sl_win_service_sources(sl_win_service_observation *observation);
int sl_win_observe_root_exit(sl_win_root_observation *observation);
int sl_win_observe_family(sl_win_family_observation *observation);
typedef struct sl_win_public_stream {
    DWORD selector;
    HANDLE original,held;
    sl_win_call get_handle,duplicate;
    unsigned observed_absent,observed_invalid,duplicate_failed;
} sl_win_public_stream;
typedef struct sl_win_entry {
    HANDLE current_process;
    sl_win_call current_process_query;
    sl_win_public_stream public_streams[3];
    sl_win_birth birth;
    unsigned public_streams_attempted,public_streams_prepared;
} sl_win_entry;
typedef struct sl_win_public_controller {
 sl_win_entry entry;
 sl_win_service_observation *services,*actual_services,*last_service;
 SIZE_T service_capacity,actual_service_capacity,service_count;
 int streams_result,birth_result,construction_result,last_service_result;
 unsigned attempted,returned,services_prepared,service_exhausted;
} sl_win_public_controller;
/* Owns suspended construction/source observations only, never Resume/Close. */
int sl_win_construct_public_controller(sl_win_public_controller *controller);
int sl_win_service_public_controller(sl_win_public_controller *controller);
int sl_win_construct_public_controller_from_prepared_streams(sl_win_public_controller *controller);
int sl_win_prepare_public_streams(sl_win_entry *entry);
int sl_win_prepare_entry_birth(sl_win_entry *entry);
/* Preeffect native source operation only. Genuine admitted ABI/rights/nesting,
 * exclusion, tool/loaded-byte/full-profile and collector readiness are separate
 * mandatory entry predicates; this function does not issue them. No Resume,
 * wait, Close, containment or successful disposition occurs here. */
int sl_win_prepare_birth(sl_win_birth *birth);
#endif
#endif
