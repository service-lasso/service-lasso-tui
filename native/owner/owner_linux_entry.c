#if defined(__linux__) && defined(__x86_64__)
#include "owner_linux.h"
#include "shared_range_linux.h"
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <linux/magic.h>

static void sl_linux_initialize_call(sl_linux_call *call) {
    atomic_init(&call->entered,0);
    atomic_init(&call->returned,0);
}

/* Called by the genuine outside-family native entry BEFORE clone3 and BEFORE
 * any Go payload. The caller-preowned entry retains every original partial
 * output. Native rights/profile/source/loaded-image admission remains required;
 * this constructor is not ordinary public invocation integration by itself. */
long sl_linux_prepare_entry_channels(sl_linux_entry *entry) {
    long result;
    unsigned index;
    if (!entry || entry->attempted) return -1;
    sl_linux_bootstrap *contained=0;
    if(entry->bootstrap_range.mapping_original){
        contained=sl_linux_shared_range_address(&entry->bootstrap_range,sizeof(*contained),_Alignof(sl_linux_bootstrap));
        if(!contained)return -1;
        uintptr_t start=(uintptr_t)contained,end=start+entry->bootstrap_range.bytes;
        uintptr_t root=(uintptr_t)entry,original=(uintptr_t)entry->bootstrap_range.mapping_original;
        if(sizeof(*entry)>UINTPTR_MAX-root||sizeof(sl_linux_call)>UINTPTR_MAX-original||
           (original<root+sizeof(*entry)&&root<original+sizeof(sl_linux_call))||
           (root<end&&start<root+sizeof(*entry))||(original<end&&start<original+sizeof(sl_linux_call)))return -1;
        if(entry->shared_mapping.source||entry->shared_mapping.number||entry->mapping_result||
           atomic_load(&entry->shared_mapping.entered)||atomic_load(&entry->shared_mapping.returned))return -1;
        const unsigned char *fresh=(const unsigned char *)contained;
        for(unsigned long i=0;i<entry->bootstrap_range.bytes;i++)if(fresh[i])return -1;
    }
    if (!entry->public_streams_prepared) {
        result=sl_linux_prepare_public_streams(entry);
        if (result<0) return result;
    }
    entry->attempted=1;
    entry->custody_endpoints[0]=-1;entry->custody_endpoints[1]=-1;
    entry->entry_endpoints[0]=-1;entry->entry_endpoints[1]=-1;
    entry->custody_raw_endpoints[0]=-1;entry->custody_raw_endpoints[1]=-1;
    entry->entry_raw_endpoints[0]=-1;entry->entry_raw_endpoints[1]=-1;
    if(contained)entry->birth.bootstrap=contained;
    else{
    result=sl_linux_call6(&entry->shared_mapping,"owner.bootstrap.mmap.shared",SYS_mmap,
                0,sizeof(sl_linux_bootstrap),PROT_READ|PROT_WRITE,
                MAP_SHARED|MAP_ANONYMOUS,-1,0);
    entry->mapping_result=result;
    if (result<0) return result;
    entry->birth.bootstrap=(sl_linux_bootstrap *)result;
    }
    if(contained)for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++)
        contained->authority.credential_ranges[i][j]=entry->credential_ranges[i][j];
    sl_linux_initialize_call(&entry->birth.bootstrap->clone_parent);
    sl_linux_initialize_call(&entry->birth.bootstrap->clone_child);
    sl_linux_initialize_call(&entry->birth.bootstrap->create_session);
    sl_linux_initialize_call(&entry->birth.bootstrap->acquire_terminal);
    entry->birth.bootstrap->controlling_terminal_fd=-1;
    sl_linux_initialize_call(&entry->birth.bootstrap->gate_close);
    sl_linux_initialize_call(&entry->birth.bootstrap->execute);
    sl_linux_initialize_call(&entry->birth.bootstrap->failure_pause);
    for (index=0;index<sizeof(entry->birth.bootstrap->gate_reads)/sizeof(entry->birth.bootstrap->gate_reads[0]);index++)
        sl_linux_initialize_call(&entry->birth.bootstrap->gate_reads[index]);
    atomic_init(&entry->birth.bootstrap->fd_actions_prepared,0);
    atomic_init(&entry->birth.bootstrap->exec_entered,0);
    atomic_init(&entry->birth.bootstrap->failed,0);
    result=sl_linux_call6(&entry->custody_pair,"owner.custody.socketpair",SYS_socketpair,
                AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0,(long)entry->custody_raw_endpoints,0,0);
    if (result<0) return result;
    for (index=0;index<2;index++) {
        result=sl_linux_lift_internal_fd(&entry->custody_lifts[index],entry->custody_raw_endpoints[index]);
        if (result<0) return result;
        entry->custody_endpoints[index]=(int)result;
    }
    long peer_first=sl_linux_observe_entry_peer(entry,0),peer_second=sl_linux_observe_entry_peer(entry,1);
    if(peer_first)return peer_first;if(peer_second)return peer_second;
    long state_first=sl_linux_observe_entry_socket_state(entry,0),state_second=sl_linux_observe_entry_socket_state(entry,1);
    if(state_first)return state_first;if(state_second)return state_second;
    result=sl_linux_call6(&entry->entry_pipe,"owner.entry-gate.pipe2",SYS_pipe2,
                (long)entry->entry_raw_endpoints,O_CLOEXEC,0,0,0,0);
    if (result<0) return result;
    for (index=0;index<2;index++) {
        result=sl_linux_lift_internal_fd(&entry->entry_lifts[index],entry->entry_raw_endpoints[index]);
        if (result<0) return result;
        entry->entry_endpoints[index]=(int)result;
    }
    entry->birth.entry_gate_fd=entry->entry_endpoints[0];
    /* No automatic munmap/close of partial outputs, no inferred valid FD,
     * caller authentication, entry release, birth or successful disposal. */
    return result;
}
/* Internal acquisition can occupy an originally closed low public FD. Keep
 * the actual acquisition output untouched, then independently duplicate above
 * the three child targets and close that internal original exactly once.
 * Public absence remains represented by the earlier original stdio query.
 * Uncertain close never retries or permits child construction to continue. */
long sl_linux_lift_internal_fd(sl_linux_fd_lift *lift,int fd) {
    long result;
    if (!lift || lift->attempted) return -1;
    lift->attempted=1;lift->original_fd=fd;lift->held_fd=-1;
    if (fd<0) return -1;
    if (fd>=3) {lift->held_fd=fd;return fd;}
    sl_linux_initialize_call(&lift->duplicate);
    sl_linux_initialize_call(&lift->close_original);
    result=sl_linux_call6(&lift->duplicate,"owner.internal-low-fd.duplicate",SYS_fcntl,
                         fd,F_DUPFD_CLOEXEC,3,0,0,0);
    if (result<3 || result>2147483647L) return result<0?result:-1;
    lift->held_fd=(int)result;
    result=sl_linux_call6(&lift->close_original,"owner.internal-low-fd.close-original",SYS_close,fd,0,0,0,0,0);
    if (result<0) return result;
    return lift->held_fd;
}
/* Allocate the actual child-call output cells before birth. The input roster
 * stays held by the outside owner as source input; copying its FD/reference
 * fields does not transfer the referenced originals or authenticate the map.
 * This function supplies backing, not closed-roster/target-vacancy admission. */
long sl_linux_prepare_fd_actions(sl_linux_entry *entry,const sl_linux_fd_action *inputs,unsigned long count) {
    unsigned long index,bytes;
    long result;
    sl_linux_fd_action *actions;
    if (!entry || entry->fd_actions_attempted || !entry->birth.bootstrap || entry->birth.attempted) return -1;
    actions=0;
    if(entry->fd_action_range.mapping_original){
        if(!inputs||!count||count>ULONG_MAX/sizeof(*inputs))return -1;
        bytes=count*sizeof(*inputs);
        actions=sl_linux_shared_range_address(&entry->fd_action_range,bytes,_Alignof(sl_linux_fd_action));
        if(!actions)return -1;
        uintptr_t start=(uintptr_t)actions,end=start+entry->fd_action_range.bytes;
        const void *originals[4]={entry,entry->birth.bootstrap,inputs,entry->fd_action_range.mapping_original};
        unsigned long sizes[4]={sizeof(*entry),sizeof(*entry->birth.bootstrap),bytes,sizeof(sl_linux_call)};
        for(unsigned i=0;i<4;i++){
            uintptr_t p=(uintptr_t)originals[i];
            if(sizes[i]>UINTPTR_MAX-p||(p<end&&start<p+sizes[i]))return -1;
        }
        uintptr_t root=(uintptr_t)entry,origin=(uintptr_t)entry->fd_action_range.mapping_original;
        if(sizeof(*entry)>UINTPTR_MAX-root||(origin<root+sizeof(*entry)&&root<origin+sizeof(sl_linux_call)))return -1;
        uintptr_t input=(uintptr_t)inputs,bootstrap=(uintptr_t)entry->birth.bootstrap;
        if(sizeof(*entry->birth.bootstrap)>UINTPTR_MAX-bootstrap||
           (input<root+sizeof(*entry)&&root<input+bytes)||
           (input<bootstrap+sizeof(*entry->birth.bootstrap)&&bootstrap<input+bytes))return -1;
        if(entry->fd_action_mapping.source||entry->fd_action_mapping.number||entry->fd_action_mapping_result||
           atomic_load(&entry->fd_action_mapping.entered)||atomic_load(&entry->fd_action_mapping.returned))return -1;
        const unsigned char *fresh=(const unsigned char *)actions;
        for(unsigned long i=0;i<entry->fd_action_range.bytes;i++)if(fresh[i])return -1;
    }
    entry->fd_actions_attempted=1;
    entry->fd_action_inputs=inputs;
    entry->fd_action_input_count=count;
    if (!inputs || !count || count>((unsigned long)-1)/sizeof(sl_linux_fd_action)) return -1;
    bytes=count*sizeof(sl_linux_fd_action);
    /* Linux mmap length must fit a positive signed syscall argument. */
    if (bytes>(((unsigned long)-1)>>1)) return -1;
    entry->fd_action_mapping_bytes=bytes;
    if(!actions){
    result=sl_linux_call6(&entry->fd_action_mapping,"owner.fd-actions.mmap.shared",SYS_mmap,
                         0,bytes,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
    entry->fd_action_mapping_result=result;
    if (result<0) return result;
    actions=(sl_linux_fd_action *)result;
    }
    /* Publish the actual partial mapping before inspecting each input. No
     * failure path unmaps it or erases an already-produced native original. */
    entry->birth.bootstrap->fd_actions=actions;
    for (index=0;index<count;index++) {
        actions[index].kind=inputs[index].kind;
        actions[index].source_fd=inputs[index].source_fd;
        actions[index].target_fd=inputs[index].target_fd;
        actions[index].outer_original=inputs[index].outer_original;
        sl_linux_initialize_call(&actions[index].call);
        if (!actions[index].outer_original || actions[index].source_fd<0 ||
            (actions[index].kind!=SL_FD_CLOSE_VIEW && actions[index].kind!=SL_FD_DUPLICATE_VIEW &&
             actions[index].kind!=SL_FD_CLEAR_EXEC)) return -1;
        if (actions[index].kind==SL_FD_DUPLICATE_VIEW &&
            (actions[index].target_fd<0 || actions[index].target_fd==actions[index].source_fd)) return -1;
    }
    entry->birth.bootstrap->fd_action_count=count;
    atomic_store_explicit(&entry->birth.bootstrap->fd_actions_prepared,1,memory_order_release);
    return 0;
}
/* The outside controller must obtain views from its genuine complete observed
 * descriptor roster under migration/duplication exclusion. This builder does
 * not accept a public numeric list as that authority. It performs real state
 * observations and builds each child action against the SAME original row.
 * No descriptor acquisition or closing happens in the parent here. */
long sl_linux_build_child_fd_map(sl_linux_entry *entry,sl_linux_fd_map *map) {
    unsigned long index,other;
    sl_linux_fd_view *payload=0,*custody=0,*pty=0,*gate=0,*standard_sources[3]={0};
    long result;
    if (!entry || !map || map->attempted || entry->birth.attempted) return -1;
    map->attempted=1;
    if (!map->views || !map->count || !map->actions || map->action_count || map->count>((unsigned long)-1)-4 ||
        map->action_capacity<map->count+4 || map->payload_fd<3 || map->custody_fd<3 ||
        map->pty_slave_fd<3 || map->payload_fd==map->custody_fd ||
        map->payload_fd==map->pty_slave_fd || map->custody_fd==map->pty_slave_fd ||
        entry->birth.payload_fd!=map->payload_fd || entry->custody_endpoints[1]!=map->custody_fd) return -1;
    for (index=0;index<map->count;index++) {
        sl_linux_fd_view *view=&map->views[index];
        if (view->fd<0 || !view->original) return -1;
        for (other=0;other<index;other++) if (map->views[other].fd==view->fd) return -1;
        sl_linux_initialize_call(&view->inspect_flags);
        result=sl_linux_call6(&view->inspect_flags,"owner.fd-map.original-flags",SYS_fcntl,
                             view->fd,F_GETFD,0,0,0,0);
        if (result<0) return result;
        if (view->fd==map->payload_fd) payload=view;
        if (view->fd==map->custody_fd) custody=view;
        if (view->fd==map->pty_slave_fd) pty=view;
        if (view->fd==entry->birth.entry_gate_fd) gate=view;
        for (other=0;other<3;other++) if (view->fd==map->standard_fds[other]) standard_sources[other]=view;
    }
    if (!payload || !custody || !pty || !gate || gate->fd<3 ||
        gate==payload || gate==custody || gate==pty) return -1;
    for (index=0;index<3;index++) {
        if (map->standard_fds[index]==-1) {
            if (!entry->public_streams_prepared || !entry->public_streams[index].observed_absent || standard_sources[index]) return -1;
        } else if (entry->public_streams[index].observed_absent || !standard_sources[index] || standard_sources[index]->fd<3 ||
                   standard_sources[index]==payload || standard_sources[index]==custody || standard_sources[index]==gate) return -1;
    }
    /* Verify every low target's actual state before building close/dup order.
     * A closed low target must be observed EBADF; an open target must have its
     * SAME row in the complete roster so its close gets an original cell. */
    for (index=0;index<3;index++) {
        unsigned found=0;
        sl_linux_initialize_call(&map->standard_state[index]);
        result=sl_linux_call6(&map->standard_state[index],"owner.fd-map.standard-target-state",SYS_fcntl,
                             index,F_GETFD,0,0,0,0);
        for (other=0;other<map->count;other++) if (map->views[other].fd==(int)index) found=1;
        if ((result>=0 && !found) || (result<0 && (result!=-EBADF || found))) return -1;
        if (map->standard_fds[index]==-1 && result!=-EBADF) return -1;
    }
    /* Unwanted child views close before duplication. The SAME observed gate
     * row is deliberately retained through the authority transition, consumed
     * after parent inspection, then independently once-closed by bootstrap. */
    for (index=0;index<map->count;index++) {
        sl_linux_fd_view *view=&map->views[index];
        sl_linux_fd_action *action;
        if (view==payload || view==custody || view==gate || view==standard_sources[0] ||
            view==standard_sources[1] || view==standard_sources[2]) continue;
        action=&map->actions[map->action_count++];
        action->kind=SL_FD_CLOSE_VIEW;action->source_fd=view->fd;
        action->target_fd=-1;action->outer_original=view;
    }
    for (index=0;index<3;index++) {
        sl_linux_fd_action *action;
        if (!standard_sources[index]) continue;
        action=&map->actions[map->action_count++];
        action->kind=SL_FD_DUPLICATE_VIEW;action->source_fd=standard_sources[index]->fd;
        action->target_fd=index;action->outer_original=standard_sources[index];
    }
    for (index=0;index<3;index++) {
        unsigned already_closed=0;
        sl_linux_fd_action *action;
        for (other=0;other<index;other++) if (standard_sources[other]==standard_sources[index]) already_closed=1;
        if (!standard_sources[index] || already_closed) continue;
        action=&map->actions[map->action_count++];
        action->kind=SL_FD_CLOSE_VIEW;action->source_fd=standard_sources[index]->fd;
        action->target_fd=-1;action->outer_original=standard_sources[index];
    }
    {
        sl_linux_fd_action *action=&map->actions[map->action_count++];
        action->kind=SL_FD_CLEAR_EXEC;action->source_fd=custody->fd;
        action->target_fd=-1;action->outer_original=custody;
    }
    /* Payload must remain held for execveat and close at actual successful
     * image replacement. The current descriptor's real CLOEXEC bit is required,
     * never inferred from its pathname or nominal acquisition API. */
    if (!(payload->inspect_flags.result & FD_CLOEXEC)) return -1;
    entry->birth.bootstrap->controlling_terminal_fd=pty->fd;
    return sl_linux_prepare_fd_actions(entry,map->actions,map->action_count);
}
/* Complete getdents EOF is required; no fixed row count becomes completeness.
 * The preowned scan backing/profile must cover all frames/rows plus separate
 * failure slots before effects. Never reuse a raw getdents buffer. The native
 * entry must exclude concurrent FD mutations and qualify procfs/mount identity;
 * observing this path alone supplies neither exclusion nor native authority. */
long sl_linux_scan_current_fds(sl_linux_fd_scan *scan) {
    unsigned long frame_index,index;
    long result;
    if (!scan || scan->attempted) return -1;
    scan->attempted=1;
    if (!scan->frames || !scan->frame_capacity || !scan->views || !scan->view_capacity ||
        scan->view_count || scan->frame_count || scan->eof) return -1;
    /* Check all backing before even acquiring the directory. */
    for (frame_index=0;frame_index<scan->frame_capacity;frame_index++) {
        sl_linux_fd_scan_frame *frame=&scan->frames[frame_index];
        if (!frame->bytes || frame->capacity<20 || frame->capacity>(((unsigned long)-1)>>1) ||
            frame->capacity>UINTPTR_MAX-(uintptr_t)frame->bytes) return -1;
        for (index=0;index<frame_index;index++) {
            uintptr_t start=(uintptr_t)frame->bytes,other_start=(uintptr_t)scan->frames[index].bytes;
            if (frame->capacity>UINTPTR_MAX-start || scan->frames[index].capacity>UINTPTR_MAX-other_start ||
                (start<other_start+scan->frames[index].capacity && other_start<start+frame->capacity)) return -1;
        }
        frame->scan_original=scan;
        sl_linux_initialize_call(&frame->read);
    }
    sl_linux_initialize_call(&scan->open_directory);
    sl_linux_initialize_call(&scan->filesystem_query);
    result=sl_linux_call6(&scan->open_directory,"owner.fd-map.proc-directory.open",SYS_openat,
                         AT_FDCWD,(long)"/proc/self/fd",O_RDONLY|O_DIRECTORY|O_CLOEXEC,0,0,0);
    scan->directory_fd=result>=0 && result<=2147483647L?(int)result:-1;
    if (result<0 || scan->directory_fd<0) return result<0?result:-1;
    result=sl_linux_call6(&scan->filesystem_query,"owner.fd-map.proc-directory.filesystem",SYS_fstatfs,
                         scan->directory_fd,(long)&scan->filesystem,0,0,0,0);
    if (result<0 || scan->filesystem.f_type!=PROC_SUPER_MAGIC) return result<0?result:-1;
    for (frame_index=0;frame_index<scan->frame_capacity;frame_index++) {
        sl_linux_fd_scan_frame *frame=&scan->frames[frame_index];
        unsigned long offset=0;
        scan->frame_count=frame_index+1;
        result=sl_linux_call6(&frame->read,"owner.fd-map.proc-directory.getdents64",SYS_getdents64,
                             scan->directory_fd,(long)frame->bytes,frame->capacity,0,0,0);
        frame->raw_count=result;
        if (result<0 || (unsigned long)result>frame->capacity) return result<0?result:-1;
        if (!result) {
            for (index=0;index<scan->view_count;index++) if (scan->views[index].fd==scan->directory_fd) {
                scan->eof=1;return 0;
            }
            return -1;
        }
        while (offset<(unsigned long)result) {
            unsigned long record_length,name_index;
            unsigned long fd=0;
            unsigned digits=0,terminated=0;
            unsigned char *record=frame->bytes+offset;
            if ((unsigned long)result-offset<20) return -1;
            /* Linux dirent64 reclen is the actual native little-endian u16 at
             * offset16; byte reads avoid unaligned typed-pointer projections. */
            record_length=(unsigned long)record[16]|((unsigned long)record[17]<<8);
            if (record_length<20 || record_length>(unsigned long)result-offset) return -1;
            for (name_index=19;name_index<record_length;name_index++) {
                unsigned char ch=record[name_index];
                if (!ch) {terminated=1;break;}
                if (ch<'0' || ch>'9') break;
                if (fd>(2147483647UL-(ch-'0'))/10) return -1;
                fd=fd*10+(ch-'0');digits++;
            }
            if (record[19]=='.') {
                /* Only actual '.' or '..' entries may be skipped. */
                if (!(record_length>20 && ((record[20]==0) || (record_length>21 && record[20]=='.' && record[21]==0)))) return -1;
            } else {
                if (!terminated || !digits || (digits>1 && record[19]=='0') || scan->view_count>=scan->view_capacity) return -1;
                for (index=0;index<scan->view_count;index++) if (scan->views[index].fd==(int)fd) return -1;
                scan->views[scan->view_count].fd=(int)fd;
                /* SAME raw frame and directory/native operation graph remain
                 * held through this actual descriptor observation's cut. */
                scan->views[scan->view_count].original=frame;
                scan->view_count++;
            }
            offset+=record_length;
        }
    }
    /* Exhaustion before real EOF is an unresolved capacity failure; no new
     * allocation, reread, close, birth or successful closed-map claim. */
    return -1;
}
long sl_linux_scan_and_build_child_fd_map(sl_linux_entry *entry,sl_linux_fd_scan *scan,sl_linux_fd_map *map) {
    long result;
    if (!entry || !scan || !map || map->attempted || scan->attempted || entry->birth.attempted) return -1;
    map->scan_original=scan;
    result=sl_linux_scan_current_fds(scan);
    if (result<0 || !scan->eof) return result<0?result:-1;
    map->views=scan->views;
    map->count=scan->view_count;
    return sl_linux_build_child_fd_map(entry,map);
}
/* First native descriptor acquisition in the public entry, before channels or
 * procfs scans can reuse closed stdFD numbers. Query all original views first;
 * F_DUPFD_CLOEXEC never fills low-number holes. Public IO later uses only the
 * SAME retained held view, or the actual observed absence, not numeric0/1/2.
 * Concurrent native FD mutation exclusion remains required qualification. */
long sl_linux_prepare_public_streams(sl_linux_entry *entry) {
    unsigned index;
    long result;
    if (!entry || entry->public_streams_attempted || entry->attempted) return -1;
    entry->public_streams_attempted=1;
    for (index=0;index<3;index++) {
        sl_linux_public_stream *stream=&entry->public_streams[index];
        stream->original_fd=index;stream->held_fd=-1;
        sl_linux_initialize_call(&stream->original_flags);
        sl_linux_initialize_call(&stream->duplicate);
        result=sl_linux_call6(&stream->original_flags,"public.entry.original-stream.flags",SYS_fcntl,
                             index,F_GETFD,0,0,0,0);
        if (result<0) {
            if (result!=-EBADF) return result;
            stream->observed_absent=1;
        }
    }
    for (index=0;index<3;index++) {
        sl_linux_public_stream *stream=&entry->public_streams[index];
        if (stream->observed_absent) continue;
        result=sl_linux_call6(&stream->duplicate,"public.entry.original-stream.duplicate",SYS_fcntl,
                             stream->original_fd,F_DUPFD_CLOEXEC,3,0,0,0);
        if (result<0 || result>2147483647L) return result<0?result:-1;
        stream->held_fd=(int)result;
    }
    entry->public_streams_prepared=1;
    return 0;
}
#endif
