/* Actual source consumer, controlled completed roots; UNRUN/native unqualified. */
#include <assert.h>
#include <errno.h>
#include "../control_roster_linux.c"
typedef struct {
 sl_linux_public_startup startup;sl_linux_mount_location location;
 sl_linux_cgroup_children children;sl_linux_child_view views[3];
 sl_linux_cgroup_control_label controls[6];sl_linux_mount_row row;
} episode;
static void prepare(episode *e){
 sl_linux_public_startup *s=&e->startup;s->returned=1;s->stage=SL_STARTUP_ACQUIRED;s->mounts.returned=s->mounts.eof=1;s->mounts.row_count=1;s->mounts.actual_rows=&e->row;
 s->actual_locations=&e->location;s->actual_location_capacity=1;s->actual_children=&e->children;s->actual_children_capacity=1;s->actual_controls=e->controls;s->actual_control_capacity=6;s->control_roster.startup_original=s;
 e->location.returned=1;e->location.actual_mounts=&s->mounts;e->location.actual_row=&e->row;
 e->children.location_original=e->children.actual_location=&e->location;e->children.views=e->children.actual_views=e->views;e->children.view_capacity=e->children.actual_view_capacity=3;e->children.view_count=3;e->children.attempted=e->children.returned=e->children.eof=1;
 e->views[0].dot=1;e->views[1].attempted=e->views[1].returned=1;e->views[2].attempted=e->views[2].returned=e->views[2].not_directory=1;e->views[2].result=-ENOTDIR;
 for(unsigned i=0;i<6;i++){sl_linux_cgroup_control_label *c=&e->controls[i];c->children_original=&e->children;c->view_original=&e->views[i/2];c->kind=(i%2)?SL_CGROUP_THREADS:SL_CGROUP_PROCS;if(i/2==1){c->attempted=c->returned=c->framing_complete=1;c->actual_children=&e->children;c->actual_view=&e->views[1];c->actual_kind=c->kind;}}
}
int main(void){
 episode cases[6]={0};for(unsigned i=0;i<6;i++)prepare(&cases[i]);
 assert(sl_linux_interpret_control_roster(&cases[0].startup.control_roster)==0);assert(cases[0].startup.control_roster.required_controls==2&&cases[0].startup.control_roster.controls_checked==6);
 cases[1].controls[3].attempted=0;assert(sl_linux_interpret_control_roster(&cases[1].startup.control_roster)==-EINVAL);assert(cases[1].startup.control_roster.control_original==&cases[1].controls[3]);
 cases[2].controls[4].attempted=1;assert(sl_linux_interpret_control_roster(&cases[2].startup.control_roster)==-EINVAL);
 cases[3].children.eof=0;assert(sl_linux_interpret_control_roster(&cases[3].startup.control_roster)==-EINVAL);
 cases[4].controls[2].actual_view=&cases[4].views[2];assert(sl_linux_interpret_control_roster(&cases[4].startup.control_roster)==-EINVAL);
 cases[5].startup.actual_control_capacity=5;assert(sl_linux_interpret_control_roster(&cases[5].startup.control_roster)==-EINVAL);
 assert(sl_linux_interpret_control_roster(&cases[1].startup.control_roster)==-EINVAL);assert(cases[1].startup.control_roster.control_original==&cases[1].controls[3]);
 return 0;
}
