/* Source-only controlled native transition cases. UNRUN. No syscall here
 * changes test credentials/namespaces; this is not genuine native proof. */
#if defined(__linux__) && defined(__x86_64__)
#include "../owner_linux.h"
#include "../capability_domain_linux.h"
#include <assert.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <linux/securebits.h>
#include <errno.h>
static unsigned calls,fail_at;static long failure=-EPERM;
static sl_linux_call *failed_original;
static unsigned credential_visits,fixture_fsuid=1000,changed_group;
long sl_linux_call6(sl_linux_call *c,const char *source,long number,long x0,long x1,long x2,long x3,long x4,long x5) {
    c->source=source;c->number=number;
    long args[6]={x0,x1,x2,x3,x4,x5};memcpy(c->arguments,args,sizeof args);
    atomic_store(&c->entered,1);calls++;
    long result=0;
    if(calls==fail_at) {result=failure;failed_original=c;}
    else if(number==SYS_getresuid || number==SYS_getresgid) {
        *(unsigned *)x0=1000;*(unsigned *)x1=1000;*(unsigned *)x2=1000;
    } else if(number==SYS_capget) {
        struct __user_cap_data_struct *data=(void *)x1;
        data[0]=(struct __user_cap_data_struct){0};data[1]=(struct __user_cap_data_struct){0};
    } else if(number==SYS_prctl && x0==PR_GET_SECUREBITS) {
        result=SECBIT_KEEP_CAPS_LOCKED|SECBIT_NO_SETUID_FIXUP|SECBIT_NO_SETUID_FIXUP_LOCKED|SECBIT_NOROOT|SECBIT_NOROOT_LOCKED|SECBIT_NO_CAP_AMBIENT_RAISE|SECBIT_NO_CAP_AMBIENT_RAISE_LOCKED;
    } else if(number==SYS_prctl && x0==PR_GET_NO_NEW_PRIVS) result=1;
    c->result=result;atomic_store(&c->returned,1);return result;
}
/* This transition fixture substitutes credential acquisition; its complete
 * read-only IO/parser/extent cases live in credentials_linux_test.c. */
long sl_linux_prepare_credential_backing(sl_linux_preexec_authority *a){(void)a;return 0;}
long sl_linux_observe_child_credentials(sl_linux_credential_snapshot *s){
 static unsigned groups[2][1];unsigned which=credential_visits++;
 assert(which<2);groups[which][0]=44+(which&&changed_group);
 for(unsigned i=0;i<4;i++)s->uids[i]=s->gids[i]=1000;
 s->uids[3]=fixture_fsuid;s->groups=groups[which];s->group_count=1;s->returned=1;s->result=0;return 0;
}
static void prepare(sl_linux_bootstrap *b,sl_linux_entry *entry,sl_linux_capability_domain *domain) {
    credential_visits=changed_group=0;fixture_fsuid=1000;
    memset(b,0,sizeof *b);memset(entry,0,sizeof *entry);memset(domain,0,sizeof *domain);
    entry->public_streams_prepared=1;entry->birth.bootstrap=b;
    domain->entry_original=entry;domain->returned=1;domain->eof=1;domain->last_cap=3;
    assert(sl_linux_prepare_preexec_authority(&b->authority,domain)==0);
}
int main(void) {
    sl_linux_bootstrap b;sl_linux_entry entry;sl_linux_capability_domain domain;
    calls=fail_at=0;prepare(&b,&entry,&domain);
    assert(sl_linux_apply_preexec_authority(&b.authority)==0);
    unsigned full=calls;assert(full>20);
    assert(b.authority.unshare.number==SYS_unshare);
    assert(b.authority.unshare.arguments[0]==CLONE_NEWCGROUP);
    assert(b.authority.bounding_drop[3].arguments[1]==3);
    assert(!atomic_load(&b.authority.bounding_drop[4].entered));
    assert(atomic_load(&b.authority.returned) && atomic_load(&b.authority.inspected_ready));
    assert(sl_linux_apply_preexec_authority(&b.authority)<0 && calls==full);
    /* Every actual operation failure stops subsequent effects and leaves the
     * SAME signed raw failure cell untouched. No rollback/retry is invented. */
    for(unsigned at=1;at<=full;at++) {
        calls=0;fail_at=at;failed_original=0;prepare(&b,&entry,&domain);
        assert(sl_linux_apply_preexec_authority(&b.authority)==failure);
        assert(calls==at && atomic_load(&b.authority.returned));
        assert(failed_original && failed_original->result==failure && atomic_load(&failed_original->returned));
        assert(!atomic_load(&b.authority.inspected_ready));
        assert(sl_linux_apply_preexec_authority(&b.authority)<0 && calls==at);
    }
    calls=fail_at=0;prepare(&b,&entry,&domain);
    fixture_fsuid=0;
    assert(sl_linux_apply_preexec_authority(&b.authority)<0&&b.authority.credentials_before.uids[3]==0);
    assert(!atomic_load(&b.authority.unshare.entered)&&!atomic_load(&b.authority.inspected_ready));
    calls=fail_at=0;prepare(&b,&entry,&domain);changed_group=1;
    assert(sl_linux_apply_preexec_authority(&b.authority)<0&&credential_visits==2);
    assert(b.authority.credentials_before.groups[0]==44&&b.authority.credentials_after.groups[0]==45);
    assert(!atomic_load(&b.authority.inspected_ready));
    calls=fail_at=0;prepare(&b,&entry,&domain);
    /* Stale SAME native cell denies preparation; its original is preserved. */
    sl_linux_preexec_authority other={0};
    other.bounding_drop[2].source="prior";entry.birth.bootstrap->authority=other;
    assert(sl_linux_prepare_preexec_authority(&entry.birth.bootstrap->authority,&domain)<0);
    assert(entry.birth.bootstrap->authority.bounding_drop[2].source==other.bounding_drop[2].source && calls==0);
    return 0;
}
#endif