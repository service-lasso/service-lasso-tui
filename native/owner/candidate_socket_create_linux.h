#ifndef SERVICE_LASSO_CANDIDATE_SOCKET_CREATE_LINUX_H
#define SERVICE_LASSO_CANDIDATE_SOCKET_CREATE_LINUX_H
#include "candidate_socket_context_linux.h"
#include "candidate_routes_linux.h"
typedef struct {
 sl_linux_candidate_roles *roles_original,*actual_roles;
 sl_linux_selinux_context *owner_original,*actual_owner;
 sl_linux_candidate_socket_context *socket_original,*actual_socket;
 sl_linux_context_reachability *payload_original,*actual_payload;
 sl_selinux_access_sources inventory;
 sl_candidate_route_source *sources,*actual_sources;size_t capacity,actual_capacity,count;
 sl_linux_selinux_status_sample after_create;
 unsigned purpose; /* 0 CREATE, 1 READ, 2 WRITE; fixed constructor source. */
 long inventory_result,primary_result,status_result,result;unsigned attempted,returned;
} sl_linux_candidate_socket_create;
/* Separate CREATE exclusion or useful READ/WRITE internal-source classification;
 * actual owner/payload exceptions require independent closed loaded entry.
 * Owner exception requires independent
 * closed loaded entry. Neither socket label nor this matrix grants authority. */
long sl_linux_interpret_candidate_socket_create(sl_linux_candidate_socket_create *root);
long sl_linux_interpret_candidate_socket_use(sl_linux_candidate_socket_create *root);
#endif