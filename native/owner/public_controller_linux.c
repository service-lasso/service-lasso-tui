#if defined(__linux__) && defined(__x86_64__)
#include "public_controller_linux.h"
#include "candidate_preexec_linux.h"
#include "public_startup_linux.h"
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <fcntl.h>
static void *sl_linux_controller_collect(void *argument) {
    sl_linux_public_controller *controller=argument;
    atomic_store_explicit(&controller->collector_entered,1,memory_order_release);
    sl_linux_serve_owner_channel(&controller->channel);
    controller->collector_result=&controller->collection;
    atomic_store_explicit(&controller->collector_returned,1,memory_order_release);
    if(controller->collector_wake)sl_linux_schedule_notify(controller->collector_wake);
    return controller->collector_result;
}
/* Construction is an actual connected controller operation, not native
 * admission. The held candidate/cgroup originals and measured source backing
 * belong to this SAME outside owner before entry. Unknown rights, namespace,
 * loaded identities and finite profile cannot be supplied by pointer presence.
 * The entry gate remains closed until all input/custody/control servicing is
 * genuinely established by the public caller; this function never writes it. */
static long construct_contained_body(sl_linux_public_controller *controller) {
    long result;
    unsigned long index,frame_index,expected_readers;
    if (!controller || controller->attempted) return -1;
    controller->attempted=1;
    if(!controller->startup_original||controller->startup_original->controller_original!=controller)return -1;
    result=sl_linux_bind_controller_preexec(controller->startup_original);
    if(result){controller->construction_result=result;sl_linux_close_failed_target_before_birth(controller);sl_linux_close_failed_archive_before_birth(controller);return result;}
    controller->namespace_observation.birth_original=&controller->entry.birth;
    controller->namespace_observation.pidfd_info.birth_original=&controller->entry.birth;
    controller->namespace_observation.pidfd_info.procfs_original=&controller->namespace_observation.views[0];
    controller->namespace_observation.root_before.birth_original=&controller->entry.birth;
    controller->namespace_observation.root_after.birth_original=&controller->entry.birth;
    controller->pty_master=-1;controller->pty_slave=-1;
    if (!controller->readers || !controller->forwarders || !controller->initial_service || !controller->service_roster || !controller->service_capacity ||
        controller->initial_service!=controller->service_roster || controller->service_index || !controller->reader_count ||
        controller->entry.birth.cgroup_fd<0 || controller->entry.birth.payload_fd<0) return -1;
    if(!controller->collector_join_roster||!controller->collector_join_capacity||controller->collector_join_index)return -1;
    for(index=0;index<controller->collector_join_capacity;index++){
        sl_linux_collector_join *join=&controller->collector_join_roster[index];
        if(join->controller_original!=controller||join->attempted||join->returned)return -1;
    }
    /* Public entry must preserve original views before opening candidate or
     * cgroup descriptors. Reuse that SAME successful observation here. */
    if (!controller->entry.public_streams_prepared) return -1;
    controller->terminal.input_original=&controller->entry.public_streams[0];
    controller->terminal.output_original=&controller->entry.public_streams[1];
    result=sl_linux_observe_public_terminal(&controller->terminal);
    controller->construction_result=result;
    if (result<0) return result;
    atomic_init(&controller->pty_open.entered,0);atomic_init(&controller->pty_open.returned,0);
    atomic_init(&controller->pty_unlock.entered,0);atomic_init(&controller->pty_unlock.returned,0);
    atomic_init(&controller->pty_peer.entered,0);atomic_init(&controller->pty_peer.returned,0);
    result=sl_linux_call6(&controller->pty_open,"public.controller.pty.master.open",SYS_openat,
                         AT_FDCWD,(long)"/dev/ptmx",O_RDWR|O_NOCTTY|O_CLOEXEC,0,0,0);
    controller->construction_result=result;
    if (result<0 || result>2147483647L) return result<0?result:-1;
    result=sl_linux_lift_internal_fd(&controller->master_lift,(int)result);
    controller->construction_result=result;
    if (result<0) return result;
    controller->pty_master=(int)result;
    atomic_init(&controller->master_flags.entered,0);atomic_init(&controller->master_flags.returned,0);
    atomic_init(&controller->master_nonblocking.entered,0);atomic_init(&controller->master_nonblocking.returned,0);
    result=sl_linux_call6(&controller->master_flags,"controller.private-master.flags",SYS_fcntl,controller->pty_master,F_GETFL,0,0,0,0);
    controller->construction_result=result;if(result<0)return result;
    result=sl_linux_call6(&controller->master_nonblocking,"controller.private-master.nonblocking",SYS_fcntl,controller->pty_master,F_SETFL,result|O_NONBLOCK,0,0,0);
    controller->construction_result=result;if(result<0)return result;
    controller->unlock_value=0;
    result=sl_linux_call6(&controller->pty_unlock,"public.controller.pty.unlock",SYS_ioctl,
                         controller->pty_master,TIOCSPTLCK,(long)&controller->unlock_value,0,0,0);
    controller->construction_result=result;
    if (result<0) return result;
    /* TIOCGPTPEER opens the peer of the actual master; there is no pathname
     * number lookup or race against a later /dev/pts path substitution. */
    result=sl_linux_call6(&controller->pty_peer,"public.controller.pty.peer.open",SYS_ioctl,
                         controller->pty_master,TIOCGPTPEER,O_RDWR|O_NOCTTY|O_CLOEXEC,0,0,0);
    controller->construction_result=result;
    if (result<0 || result>2147483647L) return result<0?result:-1;
    result=sl_linux_lift_internal_fd(&controller->slave_lift,(int)result);
    controller->construction_result=result;
    if (result<0) return result;
    controller->pty_slave=(int)result;
    result=sl_linux_prepare_private_output(&controller->private_query,&controller->private_set,
             &controller->private_state,&controller->private_output_state,controller->pty_slave);
    controller->construction_result=result;if(result<0)return result;
    for(index=0;index<3;index++){
        controller->routes[index].public_original=&controller->entry.public_streams[index];
        controller->routes[index].stream_index=(int)index;
        result=sl_linux_construct_stream_route(&controller->routes[index],controller->pty_master,controller->pty_slave);
        controller->construction_result=result;if(result<0)return result;
    }
    result=sl_linux_prepare_entry_channels(&controller->entry);
    controller->construction_result=result;
    if (result<0) return result;
    result=sl_linux_interpret_startup_socket_contexts(controller->startup_original);
    controller->construction_result=result;
    if(result<0)return result;
    result=sl_linux_interpret_startup_socket_creates(controller->startup_original);
    controller->construction_result=result;
    if(result<0)return result;
    result=sl_linux_interpret_startup_socket_uses(controller->startup_original);
    controller->construction_result=result;if(result)return result;
    controller->entry.birth.bootstrap->authority.status_bytes=controller->credential_status_bytes;
    controller->entry.birth.bootstrap->authority.status_attempts=controller->credential_status_attempts;
    controller->entry.birth.bootstrap->authority.group_capacity=controller->credential_group_capacity;
    result=sl_linux_prepare_preexec_authority(&controller->entry.birth.bootstrap->authority,
                                             controller->capability_domain_original);
    controller->construction_result=result;
    if(result<0)return result;
    if (!controller->candidate_preexec_original || controller->candidate_preexec_original->attempted ||
        controller->candidate_preexec_original->bootstrap_original || controller->entry.birth.bootstrap->candidate_preexec) {
        controller->construction_result=-1;return -1;
    }
    controller->candidate_preexec_original->bootstrap_original=controller->entry.birth.bootstrap;
    result=sl_linux_prepare_candidate_preexec(controller->candidate_preexec_original);
    controller->construction_result=result;if(result)return result;
    controller->entry.birth.bootstrap->candidate_preexec=controller->candidate_preexec_original->shared;
    if(!controller->startup_original->archive_files.returned||controller->startup_original->archive_files.result||!controller->startup_original->archive_files.local.returned||controller->startup_original->archive_files.local.result)return -1;
    controller->collection.directory_original=&controller->startup_original->archive_directory;
    controller->collection.directory_fd=controller->startup_original->archive_directory.fd;
    controller->collection.storage_original=&controller->startup_original->archive_files.local;
    controller->collection.storage_fd=controller->startup_original->archive_files.local.fd;
    controller->collection.birth_original=&controller->entry.birth;
    controller->collection.endpoint_fd=controller->entry.custody_endpoints[0];
    result=sl_linux_prepare_local_collection(&controller->collection);
    controller->construction_result=result;
    if (result<0) return result;
    result=sl_linux_prepare_final_capture(&controller->final_capture);
    controller->construction_result=result;
    if(result<0)return result;
    result=sl_linux_prepare_archive_readback(&controller->final_reader_task);
    controller->construction_result=result;
    if(result<0)return result;
    controller->channel.collection_original=&controller->collection;
    controller->channel.terminal_original=&controller->terminal;
    controller->channel.pty_fd=controller->pty_master;
    controller->channel.master_nonblocking_original=&controller->master_nonblocking;
    result=sl_linux_prepare_owner_channel(&controller->channel);
    controller->construction_result=result;
    if (result<0) return result;
    /* The controlling master is captured once. Independent stdout-pipe and
     * stderr sources each get their own reader; no duplicate master readers. */
    expected_readers=1;
    if(!controller->routes[1].observed_absent&&!controller->routes[1].shares_controlling)expected_readers++;
    if(!controller->routes[2].observed_absent)expected_readers++;
    if(controller->reader_count!=expected_readers)return -1;
    for (index=0;index<controller->reader_count;index++) {
        sl_linux_capture_worker *reader=&controller->readers[index];
        sl_linux_stream_route *route=0;
        if(index>0){
            if(index==1&&!controller->routes[1].observed_absent&&!controller->routes[1].shares_controlling)route=&controller->routes[1];
            else route=&controller->routes[2];
        }
        reader->controller_original=controller;
        if(!reader->frames||!reader->capacity)return -1;
        for (frame_index=0;frame_index<reader->capacity;frame_index++) {
            sl_linux_capture_frame *frame=&reader->frames[frame_index];
            frame->writer_original=route?(void*)route:(void*)&controller->pty_open;
            frame->input_fd=route?route->owner_fd:controller->pty_master;
            frame->public_original=route?route->public_original:&controller->entry.public_streams[1];
            frame->raw_only=!route&&!controller->routes[1].shares_controlling;
            result=sl_linux_capture_prepare(frame);
            controller->construction_result=result;
            if (result<0) return result;
        }
        controller->forwarders[index].reader_original=reader;
        result=sl_linux_prepare_forward_worker(&controller->forwarders[index]);
        controller->construction_result=result;
        if(result)return result;
    }
    controller->reader_start_result=sl_linux_prepare_raw_readers(controller->readers,controller->reader_count);
    if(controller->reader_start_result)return -1;
    for(unsigned long service_index=0;service_index<controller->service_capacity;service_index++){
        sl_linux_service_observation *observation=&controller->service_roster[service_index];
        observation->capacity=controller->reader_count;
        result=sl_linux_prepare_service_observation(observation,controller);
        if(result)return result;
    }
    controller->private_arguments.custody_fd=controller->entry.custody_endpoints[1];
    result=sl_prepare_private_arguments(&controller->private_arguments);
    controller->construction_result=result;if(result)return result;
    controller->entry.birth.argv=controller->private_arguments.actual_private_argv;
    controller->child_map.payload_fd=controller->entry.birth.payload_fd;
    controller->child_map.custody_fd=controller->entry.custody_endpoints[1];
    controller->child_map.pty_slave_fd=controller->pty_slave;
    for (index=0;index<3;index++) controller->child_map.standard_fds[index]=controller->routes[index].child_fd;
    result=sl_linux_scan_and_build_child_fd_map(&controller->entry,&controller->scan,&controller->child_map);
    controller->construction_result=result;
    if (result<0) return result;
    /* Scan and closed child mapping precede any reader pthread dispatch. The
     * actual newborn remains blocked in its native gate, before Go startup. */
    result=sl_linux_birth_into_cgroup(&controller->entry.birth);
    controller->birth_result=result;
    if (result<=0) return result<0?result:-1;
    result=sl_linux_release_parent_child_views(controller);
    if(result!=0)return result;
    for(index=0;index<controller->reader_count;index++){
        result=sl_linux_start_forward_worker(&controller->forwarders[index]);
        controller->construction_result=result;
        if(result)return result;
    }
    controller->reader_start_result=sl_linux_start_raw_readers(controller->readers,controller->reader_count);
    if (controller->reader_start_result) return -1;
    atomic_init(&controller->collector_entered,0);atomic_init(&controller->collector_returned,0);
    controller->collector_create_entered=1;
    controller->collector_create_result=pthread_create(&controller->collector_thread,0,sl_linux_controller_collect,controller);
    controller->collector_create_returned=1;
    if (controller->collector_create_result) return -1;
    result=sl_linux_service_until_authority(controller);
    controller->construction_result=result;if(result<0)return result;
    controller->namespace_observation.transition_original=&controller->authority_observed_by->authority;
    result=sl_linux_observe_parent_namespace(&controller->namespace_observation);
    controller->construction_result=result;if(result<0)return result;
    result=sl_linux_interpret_namespace_target(controller);
    controller->construction_result=result;if(result<0)return result;
    controller->construction_body_completed=1;
    return controller->birth_result;
}
long sl_linux_construct_contained_producer(sl_linux_public_controller *controller){
    if(!controller||controller->attempted)return -1;
    long result=construct_contained_body(controller);
    controller->construction_return_result=result;
    controller->construction_result=controller->construction_body_completed?0:(result?result:-1);
    controller->construction_body_returned=1;
    if(controller->construction_result&&controller->birth_result>0&&controller->service_roster&&controller->schedule.prepared)
        (void)sl_linux_continue_failed_sources(controller);
    controller->construction_returned=1;
    return result;
}
#endif
