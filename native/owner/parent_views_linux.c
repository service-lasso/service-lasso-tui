#if defined(__linux__) && defined(__x86_64__)
#include "public_controller_linux.h"
#include <sys/syscall.h>
long sl_linux_release_parent_child_views(sl_linux_public_controller *controller){
    long result;unsigned long index;
    if(!controller||controller->parent_views_attempted||!controller->entry.birth.bootstrap)return -1;
    controller->parent_views_attempted=1;
    if(!atomic_load_explicit(&controller->entry.birth.bootstrap->clone_parent.returned,memory_order_acquire)||
       controller->entry.birth.bootstrap->clone_parent.result<=0||
       controller->birth_result!=controller->entry.birth.bootstrap->clone_parent.result)return -1;
    if(controller->entry.custody_endpoints[1]<3||controller->entry.entry_endpoints[0]<3||controller->pty_slave<3||
       controller->entry.custody_endpoints[1]==controller->entry.entry_endpoints[0]||
       controller->entry.custody_endpoints[1]==controller->pty_slave||controller->entry.entry_endpoints[0]==controller->pty_slave)return -1;
    for(index=0;index<3;index++){
        sl_linux_stream_route *route=&controller->routes[index];unsigned long earlier;
        if(!route->attempted||route->public_original!=&controller->entry.public_streams[index])return -1;
        if(route->observed_absent){if(route->child_fd!=-1)return -1;continue;}
        if(route->shares_controlling){if(route->child_fd!=controller->pty_slave||route->owner_fd!=controller->pty_master)return -1;continue;}
        if(route->child_fd<3||route->child_fd==controller->pty_slave||route->child_fd==controller->entry.custody_endpoints[1]||route->child_fd==controller->entry.entry_endpoints[0])return -1;
        for(earlier=0;earlier<index;earlier++)if(!controller->routes[earlier].observed_absent&&!controller->routes[earlier].shares_controlling&&controller->routes[earlier].child_fd==route->child_fd)return -1;
    }
    atomic_init(&controller->parent_custody_child_close.entered,0);atomic_init(&controller->parent_custody_child_close.returned,0);
    controller->parent_custody_child_close_attempted=1;
    result=sl_linux_call6(&controller->parent_custody_child_close,"controller.parent.custody-child-view.close-once",SYS_close,controller->entry.custody_endpoints[1],0,0,0,0,0);
    if(result!=0&&!controller->parent_close_primary)controller->parent_close_primary=&controller->parent_custody_child_close;
    atomic_init(&controller->parent_gate_child_close.entered,0);atomic_init(&controller->parent_gate_child_close.returned,0);
    controller->parent_gate_child_close_attempted=1;
    result=sl_linux_call6(&controller->parent_gate_child_close,"controller.parent.gate-child-view.close-once",SYS_close,controller->entry.entry_endpoints[0],0,0,0,0,0);
    if(result!=0&&!controller->parent_close_primary)controller->parent_close_primary=&controller->parent_gate_child_close;
    /* Clone has its own actual descriptor table. Parent-side child endpoints
     * must close once or the owner itself would keep capture writers alive. */
    atomic_init(&controller->parent_slave_close.entered,0);atomic_init(&controller->parent_slave_close.returned,0);
    controller->parent_slave_close_attempted=1;
    result=sl_linux_call6(&controller->parent_slave_close,"controller.parent.pty-slave.close-once",SYS_close,controller->pty_slave,0,0,0,0,0);
    if(result!=0&&!controller->parent_close_primary)controller->parent_close_primary=&controller->parent_slave_close;
    for(index=0;index<3;index++){
        sl_linux_stream_route *route=&controller->routes[index];
        if(route->observed_absent||route->shares_controlling)continue;
        result=sl_linux_close_parent_child_view(route);
        if(result!=0&&!controller->parent_close_primary)controller->parent_close_primary=&route->close_child;
    }
    // Visit every independent acquired child view once; never launch after any failure.
    if(controller->parent_close_primary){controller->construction_result=controller->parent_close_primary->result;return controller->construction_result<0?controller->construction_result:-1;}

    controller->construction_result=0;return 0;
}
#endif
