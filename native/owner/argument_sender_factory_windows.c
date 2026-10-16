#ifdef _WIN32
#include "argument_sender_factory_windows.h"
#include <stdint.h>
#ifndef SL_ARGUMENT_WRITER_STACK_RESERVATION_BYTES
#error exact reviewed generated argument-writer stack input required
#endif
#ifndef SL_ARGUMENT_WRITER_TURN_CAPACITY
#error exact reviewed generated argument-writer turn input required
#endif
_Static_assert(sizeof(SIZE_T)==8,"selected Windows amd64 SIZE_T");
_Static_assert(SL_ARGUMENT_WRITER_STACK_RESERVATION_BYTES>0,"positive source stack operand");
_Static_assert(SL_ARGUMENT_WRITER_TURN_CAPACITY>0,"positive source turn roster");
_Static_assert(SL_ARGUMENT_WRITER_TURN_CAPACITY<=SIZE_MAX/sizeof(sl_win_argument_send_turn),"source turn roster representability");

static int factory_disjoint(const void *a,SIZE_T an,const void *b,SIZE_T bn){
 ULONG_PTR x=(ULONG_PTR)a,y=(ULONG_PTR)b;
 return x&&y&&an&&bn&&x<=~(ULONG_PTR)0-an&&y<=~(ULONG_PTR)0-bn&&!(x<y+bn&&y<x+an);
}
static int factory_zero(const void *p,SIZE_T n){const unsigned char *b=p;for(SIZE_T i=0;i<n;i++)if(b[i])return 0;return 1;}
int sl_win_bind_argument_sender_factory(sl_win_argument_sender_factory *f,sl_win_public_startup *o,
 sl_win_argument_send_turn *turns,SIZE_T count,SIZE_T stack){
 if(!f||!o||!turns||count!=(SIZE_T)SL_ARGUMENT_WRITER_TURN_CAPACITY||stack!=(SIZE_T)SL_ARGUMENT_WRITER_STACK_RESERVATION_BYTES||
    count>SIZE_MAX/sizeof(*turns)||(ULONG_PTR)turns%__alignof(sl_win_argument_send_turn)||
    !factory_zero(f,sizeof(*f))||!factory_zero(turns,count*sizeof(*turns))||!factory_disjoint(o,sizeof(*o),f,sizeof(*f))||
    !factory_disjoint(f,sizeof(*f),turns,count*sizeof(*turns))||!factory_disjoint(o,sizeof(*o),turns,count*sizeof(*turns)))return 0;
 if(o->attempted||o->returned||!factory_zero(&o->argument_sequence,sizeof(o->argument_sequence))||
    !sl_win_public_startup_factory_disjoint(o,f,sizeof(*f))||!sl_win_public_startup_factory_disjoint(o,turns,count*sizeof(*turns)))return 0;
 // Complete private factory must already have genuine containing reservation
 // and exact admitted profile/provider/source custody. This performs no native
 // effect, measures no stack, grants no funding, and dispatches no worker.
 f->attempted=1;f->owner_original=o;f->turns_original=turns;f->turn_capacity=count;f->stack_reservation=stack;
 o->argument_sequence.stack_reservation=stack;
 o->argument_sequence.turns=turns;o->argument_sequence.turn_capacity=count;
 for(SIZE_T i=0;i<count;i++)turns[i].sequence_original=&o->argument_sequence;
 f->returned=1;return 1;
}
#endif
