/* UNRUN actual generated-helper refusal fixture. Compile only with an exact
 * generated synthetic source fixture supplied through the macro below and the
 * existing native owner link inputs. Synthetic source/domain/cost rows are NOT
 * admitted issuer/reservation inputs. No allocation/clone/activation is allowed
 * by these cases; authentic ROOT admission is still required to execute them.
 * This does not replace the genuine constructed-root/borrower fault matrix. */
#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#define SERVICE_LASSO_OWNED_SEPOL 1
#ifndef SL_CANDIDATE_GENERATED_SOURCE
#error exact generated fixture source input required
#endif
#include SL_CANDIDATE_GENERATED_SOURCE
#include <assert.h>

enum { fixture_items=sizeof(sl_candidate_static_charge_roster)/sizeof(sl_candidate_static_charge_roster[0])+1 };
typedef struct {
 sl_linux_containing_charge charge;
 sl_linux_containing_source source;
 sl_linux_charge_item items[fixture_items],copies[fixture_items];
 unsigned char graph[8],domain[8],actor[8],basis[8];
} entry_fixture;

static void prepare_fixture(entry_fixture *f){
 memset(f,0,sizeof(*f));
 for(unsigned long i=0;i<fixture_items-1;i++)f->items[i]=sl_candidate_static_charge_roster[i];
 f->items[fixture_items-1]=(sl_linux_charge_item){f->actor,sizeof(f->actor),f->basis,sizeof(f->basis),1,SL_CHARGE_RETAINED_OVERLAP};
 f->source=(sl_linux_containing_source){f->graph,sizeof(f->graph),f->domain,sizeof(f->domain),ULONG_MAX,4096,f->items,fixture_items};
 f->charge.source_original=&f->source;f->charge.copies_original=f->copies;f->charge.capacity=fixture_items;
}
static void assert_no_constructor_write(void){
 const void *objects[]={&sl_candidate_native_arena_original,&sl_candidate_layout_original,&sl_candidate_profile_original,
  &sl_candidate_preparation_original,&sl_candidate_rows_0,&sl_candidate_rows_1,&sl_candidate_rows_2,&sl_candidate_rows_3,&sl_candidate_rows_4};
 const unsigned long extents[]={sizeof(sl_candidate_native_arena_original),sizeof(sl_candidate_layout_original),sizeof(sl_candidate_profile_original),
  sizeof(sl_candidate_preparation_original),sizeof(sl_candidate_rows_0),sizeof(sl_candidate_rows_1),sizeof(sl_candidate_rows_2),sizeof(sl_candidate_rows_3),sizeof(sl_candidate_rows_4)};
 for(unsigned i=0;i<sizeof(objects)/sizeof(objects[0]);i++){
  const unsigned char *bytes=objects[i];for(unsigned long j=0;j<extents[i];j++)assert(!bytes[j]);
 }
}
int main(void){
 entry_fixture f;char empty[]={0};char *arguments[]={empty,0},*environment[]={0};
 prepare_fixture(&f);
 /* The actual pure exclusion predicate accepts disjoint source backing. This
  * is not a containing reservation or permission to call an effectful seam. */
 assert(sl_candidate_source_backing_excludes_output(&f.charge,&sl_candidate_layout_original,sizeof(sl_candidate_layout_original)));
 assert_no_constructor_write();
 assert(!sl_candidate_prepare_owned_entry(0,1,arguments,environment));
 for(unsigned episode=0;episode<18;episode++){
  prepare_fixture(&f);char **argv=arguments;
  sl_linux_candidate_layout_plan foreign_plan={0};
  switch(episode){
  case 0:f.charge.source_original=0;break;
  case 1:f.charge.capacity=ULONG_MAX;break;
  case 2:f.source.source_graph_original=&sl_candidate_layout_original;f.source.source_graph_extent=sizeof(sl_candidate_layout_original);break;
  case 3:f.source.domain_original=&sl_candidate_profile_original;f.source.domain_extent=sizeof(sl_candidate_profile_original);break;
  case 4:f.items[fixture_items-1].actor_original=&sl_candidate_preparation_original;f.items[fixture_items-1].actor_extent=sizeof(sl_candidate_preparation_original);break;
  case 5:f.items[fixture_items-1].basis_original=&sl_candidate_rows_0;f.items[fixture_items-1].basis_extent=sizeof(sl_candidate_rows_0);break;
  case 6:f.charge.copies_original=(sl_linux_charge_item *)&sl_candidate_native_arena_original;break;
  case 7:f.charge.plan_original=&foreign_plan;break;
  case 8:f.charge.attempted=1;break;
  case 9:arguments[0]=(char *)&sl_candidate_rows_0;break;
  case 10:argv=(char **)&sl_candidate_rows_0;break;
  case 11:f.source.count--;break;
  case 12:f.source.available_bytes=0;break;
  case 13:f.source.page_size=3;break;
  case 14:f.items[fixture_items-1].bytes=0;break;
  case 15:f.items[fixture_items-1].category=SL_CHARGE_CATEGORY_COUNT;break;
  case 16:f.charge.actual_source=&f.source;break;
  case 17:f.copies[0].bytes=1;break;
  }
  unsigned attempted=f.charge.attempted;
  assert(!sl_candidate_prepare_owned_entry(&f.charge,1,argv,environment));
  assert(f.charge.attempted==attempted&&!f.charge.returned);
  assert_no_constructor_write();arguments[0]=empty;
 }
 /* Missing genuine prepared owner/charge/source/root/sequence cannot dispatch
  * or fabricate clock enrollment. Each actual generated helper guard runs. */
 sl_linux_deadline_source source={0};sl_linux_deadline root={0};sl_linux_deadline_sequence sequence={0};
 assert(sl_candidate_enter_owned_outer(0,0,&source,&root,&sequence)==-EINVAL);
 assert(!root.attempted&&!sequence.attempted);assert_no_constructor_write();
 return 0;
}
#endif
