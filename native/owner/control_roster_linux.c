#include "public_startup_linux.h"
#include <errno.h>
#include <limits.h>
long sl_linux_interpret_control_roster(sl_linux_control_roster *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;o->result=-EINVAL;
 sl_linux_public_startup *s=o->startup_original;unsigned long at=0;
 if(!s||o!=&s->control_roster||!s->returned||s->result||s->stage!=SL_STARTUP_ACQUIRED||!s->mounts.returned||s->mounts.result||!s->mounts.eof||(s->mounts.row_count&&!s->mounts.actual_rows)||!s->actual_locations||!s->actual_children||!s->actual_controls||s->actual_location_capacity!=s->actual_children_capacity||s->mounts.row_count>s->actual_location_capacity)goto done;
 for(unsigned long i=0;i<s->actual_children_capacity;i++){
  sl_linux_mount_location *l=&s->actual_locations[i];sl_linux_cgroup_children *c=&s->actual_children[i];o->location_original=l;o->children_original=c;
  if(c->location_original!=l||!c->views||!c->view_capacity||c->view_capacity>ULONG_MAX/2||2*c->view_capacity>s->actual_control_capacity-at)goto done;
  unsigned active=i<s->mounts.row_count&&!l->outside_mount;
  if(i<s->mounts.row_count&&(!l->returned||(l->outside_mount?l->result!=1:l->result!=0)||l->actual_mounts!=&s->mounts||l->row_index!=i||l->actual_row!=&s->mounts.actual_rows[i]))goto done;
  if(active&&(!c->attempted||!c->returned||c->result||!c->eof||c->actual_location!=l||c->actual_views!=c->views||c->actual_view_capacity!=c->view_capacity||c->view_count>c->view_capacity))goto done;
  if(!active&&c->attempted)goto done;
  for(unsigned long j=0;j<c->view_capacity;j++){
   sl_linux_child_view *v=&c->views[j];o->view_original=v;
   unsigned required=active&&j<c->view_count&&!v->dot&&!v->not_directory;
   if(active&&j<c->view_count&&!v->dot&&(!v->attempted||!v->returned||(v->not_directory?v->result!=-ENOTDIR:v->result!=0)))goto done;
   for(unsigned kind=SL_CGROUP_PROCS;kind<=SL_CGROUP_THREADS;kind++){
    sl_linux_cgroup_control_label *control=&s->actual_controls[at++];o->control_original=control;o->kind=kind;
    if(control->children_original!=c||control->view_original!=v||control->kind!=kind)goto done;
    if(required){
     o->required_controls++;
     if(!control->attempted||!control->returned||control->result||!control->framing_complete||control->actual_children!=c||control->actual_view!=v||control->actual_kind!=kind)goto done;
    }else if(control->attempted)goto done;
    o->controls_checked++;
   }
   o->views_checked++;
  }
  o->locations_checked++;
 }
 if(at!=s->actual_control_capacity)goto done;
 o->result=0;
 done:o->returned=1;return o->result;
}
