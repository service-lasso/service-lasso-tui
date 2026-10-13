/* UNRUN actual helper source; no parser/kernel/policy authority or native proof. */
#include <assert.h>
#include "../selinux_policy_relations.c"
typedef struct helper_case {
 sl_selinux_te_union root;sl_selinux_helper_visit helpers[3];
 hashtab_val_t table;hashtab_node_t node;hashtab_ptr_t buckets[1];int datum;
} helper_case;
static helper_case cases[6];
static void prepare(helper_case *c,const char *key,unsigned long bound){
 c->node.key=(char*)key;c->node.datum=&c->datum;c->buckets[0]=&c->node;
 c->table.htable=c->buckets;c->table.size=c->table.nel=1;
 c->root.actual_helpers=c->helpers;c->root.actual_helper_capacity=3;c->root.actual_string_capacity=bound;
}
int main(void){
 static const char short_name[]="a",long_name[]="longer",unterminated[3]={'x','y','z'};
 void *result=NULL;helper_case *a=&cases[0];prepare(a,long_name,16);a->root.actual_class_name=short_name;
 assert(relation_lookup(&a->root,&a->table,&a->root.actual_class_name,&result)==0&&!result);
 sl_selinux_helper_visit *v=&a->helpers[1];
 assert(v->name_original==short_name&&v->key_original==long_name&&v->compared==sizeof long_name);
 assert(v->name_read_extent==sizeof short_name&&v->key_read_extent==sizeof long_name&&v->name_terminated&&v->key_terminated);
 helper_case *b=&cases[1];prepare(b,short_name,16);b->root.actual_class_name=long_name;
 assert(relation_lookup(&b->root,&b->table,&b->root.actual_class_name,&result)==0&&!result);
 v=&b->helpers[1];assert(v->name_read_extent==sizeof long_name&&v->key_read_extent==sizeof short_name&&v->compared==sizeof long_name);
 helper_case *c=&cases[2];prepare(c,unterminated,3);c->root.actual_class_name=short_name;
 assert(relation_lookup(&c->root,&c->table,&c->root.actual_class_name,&result)<0&&!result);
 v=&c->helpers[1];assert(v->compared==3&&v->name_read_extent==2&&v->name_terminated&&v->key_read_extent==3&&!v->key_terminated);
 helper_case *d=&cases[3];prepare(d,short_name,16);
 assert(genfs_name(&d->root,&d->helpers[0],"cgroup2")==2);
 assert(d->helpers[0].name_read_extent==8&&d->helpers[0].name_terminated&&!d->helpers[0].key_read_extent);
 helper_case *e=&cases[4];prepare(e,short_name,3);
 assert(genfs_path(&e->root,&e->helpers[0],"bad")<0);
 assert(e->helpers[0].key_read_extent==1&&!e->helpers[0].key_terminated&&!e->helpers[0].compared);
 helper_case *f=&cases[5];prepare(f,short_name,3);static const char path[3]={'/','a','b'};
 assert(genfs_path(&f->root,&f->helpers[0],path)<0);
 assert(f->helpers[0].key_read_extent==3&&f->helpers[0].compared==3&&!f->helpers[0].key_terminated);
 /* All earlier originals remain distinct, including the failed bounded scan. */
 assert(c->helpers[1].key_original==unterminated&&c->helpers[1].key_read_extent==3&&!c->helpers[1].key_terminated);
 assert(a->helpers[1].name_read_extent==2&&a->helpers[1].key_read_extent==7);
 return 0;
}