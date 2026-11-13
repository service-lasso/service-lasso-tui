#if defined(__linux__) && defined(__x86_64__)
#include "owner_linux.h"
#include "capability_domain_linux.h"
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <linux/securebits.h>
#include <fcntl.h>
#include <stddef.h>

_Static_assert(_LINUX_CAPABILITY_U32S_3==2,"selected capability representation");
#define SL_SECUREBITS (SECBIT_KEEP_CAPS_LOCKED|SECBIT_NO_SETUID_FIXUP|SECBIT_NO_SETUID_FIXUP_LOCKED|SECBIT_NOROOT|SECBIT_NOROOT_LOCKED|SECBIT_NO_CAP_AMBIENT_RAISE|SECBIT_NO_CAP_AMBIENT_RAISE_LOCKED)
static int fresh(sl_linux_call *call) {
    if (call->source || call->number || atomic_load_explicit(&call->entered,memory_order_acquire) ||
        atomic_load_explicit(&call->returned,memory_order_acquire)) return 0;
    atomic_init(&call->entered,0);atomic_init(&call->returned,0);return 1;
}
long sl_linux_prepare_preexec_authority(sl_linux_preexec_authority *a,const sl_linux_capability_domain *domain) {
    if (!a || a->prepared || a->attempted || a->domain_original || !domain || !domain->returned ||
        domain->result || !domain->eof || domain->last_cap>=64 || !domain->entry_original ||
        !domain->entry_original->public_streams_prepared || !domain->entry_original->birth.bootstrap ||
        &domain->entry_original->birth.bootstrap->authority!=a || domain->entry_original->birth.attempted) return -1;
    /* Reserve/check all actual cells before a child can borrow this object.
     * A partially initialized failed preparation cannot be retried. */
    a->domain_original=domain;a->last_cap=domain->last_cap;
    for (unsigned i=0;i<2;i++) if (!fresh(&a->namespace_before[i]) || !fresh(&a->namespace_after[i])) return -1;
    if (!fresh(&a->unshare) || !fresh(&a->uid_before) || !fresh(&a->uid_after) ||
        !fresh(&a->gid_before) || !fresh(&a->gid_after) || !fresh(&a->securebits_set) ||
        !fresh(&a->securebits_get) || !fresh(&a->ambient_clear) || !fresh(&a->cap_before) ||
        !fresh(&a->cap_clear) || !fresh(&a->cap_after) || !fresh(&a->nnp_set) || !fresh(&a->nnp_get)) return -1;
    for (unsigned i=0;i<64;i++) if (!fresh(&a->bounding_drop[i]) || !fresh(&a->bounding_read[i]) || !fresh(&a->ambient_read[i])) return -1;
    a->header_before.version=_LINUX_CAPABILITY_VERSION_3;
    a->header_clear.version=_LINUX_CAPABILITY_VERSION_3;
    a->header_after.version=_LINUX_CAPABILITY_VERSION_3;
    a->header_before.pid=0;a->header_clear.pid=0;a->header_after.pid=0;
    for (unsigned i=0;i<2;i++) a->caps_clear[i]=(struct __user_cap_data_struct){0};
    atomic_init(&a->inspected_ready,0);atomic_init(&a->returned,0);
    long backing=sl_linux_prepare_credential_backing(a);if(backing<0)return backing;
    a->prepared=1;return 0;
}
static long query_namespace(sl_linux_call *call,struct statx *output,unsigned index) {
    static const char *paths[2]={"/proc/self/ns/user","/proc/self/ns/cgroup"};
    return sl_linux_call6(call,"bootstrap.namespace.statx",SYS_statx,AT_FDCWD,
        (long)paths[index],0,STATX_INO|STATX_TYPE|STATX_MNT_ID,(long)output,0);
}
#define SL_APPLY(call) do { result=(call);if(result<0)goto done; } while(0)
#define SL_ZERO(call) do { SL_APPLY(call);if(result){result=-1;goto done;} } while(0)
long sl_linux_apply_preexec_authority(sl_linux_preexec_authority *a) {
    long result=-1;
    if (!a || !a->prepared || a->attempted || !a->domain_original || a->last_cap>=64) return -1;
    a->attempted=1;
    SL_ZERO(sl_linux_call6(&a->uid_before,"bootstrap.credentials.getresuid.before",SYS_getresuid,
        (long)&a->uids_before[0],(long)&a->uids_before[1],(long)&a->uids_before[2],0,0,0));
    SL_ZERO(sl_linux_call6(&a->gid_before,"bootstrap.credentials.getresgid.before",SYS_getresgid,
        (long)&a->gids_before[0],(long)&a->gids_before[1],(long)&a->gids_before[2],0,0,0));
    if (!a->uids_before[0] || !a->uids_before[1] || !a->uids_before[2]) {result=-1;goto done;}
    SL_ZERO(sl_linux_observe_child_credentials(&a->credentials_before));
    for(unsigned i=0;i<3;i++) if(a->credentials_before.uids[i]!=a->uids_before[i] || a->credentials_before.gids[i]!=a->gids_before[i]) {result=-1;goto done;}
    if(!a->credentials_before.uids[3]) {result=-1;goto done;}
    SL_ZERO(sl_linux_call6(&a->cap_before,"bootstrap.capabilities.before",SYS_capget,
        (long)&a->header_before,(long)a->caps_before,0,0,0,0));
    for (unsigned i=0;i<2;i++) SL_ZERO(query_namespace(&a->namespace_before[i],&a->namespaces_before[i],i));
    SL_ZERO(sl_linux_call6(&a->unshare,"bootstrap.cgroup-namespace.unshare",SYS_unshare,CLONE_NEWCGROUP,0,0,0,0,0));
    for (unsigned i=0;i<2;i++) SL_ZERO(query_namespace(&a->namespace_after[i],&a->namespaces_after[i],i));
    /* These are raw observations, not owning-userns/root/mount/profile proof.
     * No new user namespace, UID/GID/group, mount or host policy is installed. */
    SL_ZERO(sl_linux_call6(&a->securebits_set,"bootstrap.securebits.lock",SYS_prctl,PR_SET_SECUREBITS,SL_SECUREBITS,0,0,0,0));
    SL_APPLY(sl_linux_call6(&a->securebits_get,"bootstrap.securebits.readback",SYS_prctl,PR_GET_SECUREBITS,0,0,0,0,0));
    if (result!=SL_SECUREBITS) {result=-1;goto done;}
    for (unsigned i=0;i<=a->last_cap;i++) {
        SL_ZERO(sl_linux_call6(&a->bounding_drop[i],"bootstrap.bounding.drop",SYS_prctl,PR_CAPBSET_DROP,i,0,0,0,0));
        SL_APPLY(sl_linux_call6(&a->bounding_read[i],"bootstrap.bounding.readback",SYS_prctl,PR_CAPBSET_READ,i,0,0,0,0));
        if(result) {result=-1;goto done;}
    }
    SL_ZERO(sl_linux_call6(&a->ambient_clear,"bootstrap.ambient.clear",SYS_prctl,PR_CAP_AMBIENT,PR_CAP_AMBIENT_CLEAR_ALL,0,0,0,0));
    for (unsigned i=0;i<=a->last_cap;i++) {
        SL_APPLY(sl_linux_call6(&a->ambient_read[i],"bootstrap.ambient.readback",SYS_prctl,PR_CAP_AMBIENT,PR_CAP_AMBIENT_IS_SET,i,0,0,0));
        if(result) {result=-1;goto done;}
    }
    SL_ZERO(sl_linux_call6(&a->cap_clear,"bootstrap.capabilities.clear",SYS_capset,
        (long)&a->header_clear,(long)a->caps_clear,0,0,0,0));
    SL_ZERO(sl_linux_call6(&a->cap_after,"bootstrap.capabilities.readback",SYS_capget,
        (long)&a->header_after,(long)a->caps_after,0,0,0,0));
    for (unsigned i=0;i<2;i++) if(a->caps_after[i].effective || a->caps_after[i].permitted || a->caps_after[i].inheritable) {result=-1;goto done;}
    SL_ZERO(sl_linux_call6(&a->nnp_set,"bootstrap.no-new-privileges.set",SYS_prctl,PR_SET_NO_NEW_PRIVS,1,0,0,0,0));
    SL_APPLY(sl_linux_call6(&a->nnp_get,"bootstrap.no-new-privileges.readback",SYS_prctl,PR_GET_NO_NEW_PRIVS,0,0,0,0,0));
    if(result!=1) {result=-1;goto done;}
    SL_ZERO(sl_linux_call6(&a->uid_after,"bootstrap.credentials.getresuid.after",SYS_getresuid,
        (long)&a->uids_after[0],(long)&a->uids_after[1],(long)&a->uids_after[2],0,0,0));
    SL_ZERO(sl_linux_call6(&a->gid_after,"bootstrap.credentials.getresgid.after",SYS_getresgid,
        (long)&a->gids_after[0],(long)&a->gids_after[1],(long)&a->gids_after[2],0,0,0));
    for (unsigned i=0;i<3;i++) if(a->uids_before[i]!=a->uids_after[i] || a->gids_before[i]!=a->gids_after[i]) {result=-1;goto done;}
    SL_ZERO(sl_linux_observe_child_credentials(&a->credentials_after));
    for(unsigned i=0;i<4;i++) if(a->credentials_before.uids[i]!=a->credentials_after.uids[i] || a->credentials_before.gids[i]!=a->credentials_after.gids[i]) {result=-1;goto done;}
    if(a->credentials_before.group_count!=a->credentials_after.group_count) {result=-1;goto done;}
    for(unsigned i=0;i<a->credentials_before.group_count;i++) if(a->credentials_before.groups[i]!=a->credentials_after.groups[i]) {result=-1;goto done;}
    result=0;
done:
    a->result=result;atomic_store_explicit(&a->returned,1,memory_order_release);
    if(!result) atomic_store_explicit(&a->inspected_ready,1,memory_order_release);
    /* Ready only publishes immutable raw child cells for actual parent
     * inspection. It never authorizes a gate token or supplies profile proof. */
    return result;
}
#undef SL_ZERO
#undef SL_APPLY
#endif