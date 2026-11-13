/* UNRUN synthetic archive originals, no native mapping/provider authority. */
#include "../shared_range_archive_linux.c"
#include <assert.h>
typedef struct range_case {
 sl_linux_shared_range_archive_roster output;
 sl_linux_final_record records[4];
 unsigned char bytes[448];
} range_case;
static void put(unsigned char *p,uint64_t value){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(value>>(8*i));}
static void prepare(range_case *c){
 c->output.expected=3;
 for(unsigned i=0;i<4;i++){c->records[i].offset=112*i;c->records[i].length=80;}
 for(unsigned role=0;role<2;role++){
  unsigned char *native=c->bytes+224*role,*range=native+112;
  put(native,3);put(native+8,UINT64_MAX-9);put(native+16,20+role);
  put(native+32,SYS_mmap);put(native+48,4096);put(native+56,PROT_READ|PROT_WRITE);
  put(native+64,MAP_SHARED|MAP_ANONYMOUS);put(native+72,UINT64_MAX);
  put(native+88,4096);put(native+96,1);put(native+104,1);
  put(range,36);put(range+8,UINT64_MAX-9);put(range+16,20+role);put(range+24,1);
  put(range+32,128*role);put(range+40,64);put(range+48,4096+128*role);
 }
}
int main(void){
 static range_case cases[12];
 for(unsigned mode=0;mode<12;mode++){
  range_case *c=&cases[mode];prepare(c);
  unsigned char *native=c->bytes+224,*range=native+112;
  if(mode==1){put(native+16,20);put(range+16,20);} /* duplicate role */
  if(mode==2){put(native+88,8192);put(range+48,8320);} /* other origin, valid individual range */
  if(mode==3){put(range+32,32);put(range+48,4128);} /* cross-role backing overlap */
  if(mode==4)put(range+16,22); /* unselected role */
  if(mode==5)put(native+16,19); /* noncorresponding adjacent frame */
  if(mode==6)put(native+104,0); /* no returned native original */
  if(mode==7)put(range+56,1); /* reserved representation byte */
  if(mode==8){put(range+32,4096);put(range+40,1);put(range+48,8192);} /* range past map */
  if(mode==9)c->output.expected=7; /* missing required source role */
  if(mode==10)c->records[3].offset++; /* metadata/byte cursor mismatch */
  if(mode==11)put(range+24,0); /* wrong adjacent range occurrence tag */
  unsigned char original[448];memcpy(original,c->bytes,sizeof(original));
  long result=sl_linux_interpret_shared_range_records(&c->output,c->bytes,sizeof(c->bytes),c->records,4);
  assert(c->output.attempted&&c->output.returned&&!memcmp(original,c->bytes,sizeof(original)));
  if(!mode){assert(!result&&c->output.seen==3&&c->output.roles[1].address==4224);
   assert(c->output.roles[0].native_original==c->bytes+32&&c->output.roles[1].range_original==range+32);
  }else assert(result<0);
  unsigned char saved[sizeof(c->output)];memcpy(saved,&c->output,sizeof(saved));
  assert(sl_linux_interpret_shared_range_records(&c->output,c->bytes,sizeof(c->bytes),c->records,4)<0);
  assert(!memcmp(saved,&c->output,sizeof(saved)));
 }
 return 0;
}
