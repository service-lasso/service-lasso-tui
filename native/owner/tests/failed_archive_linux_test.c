/* Synthetic production-body framing coverage, UNRUN. No native custody. */
#include "../failed_archive_linux.h"
#include <assert.h>
#include <string.h>
static void put64(unsigned char *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(8*i));}
static void valid(unsigned char *p){memset(p,0,96);memcpy(p,"SLFTAR1",7);p[8]=1;put64(p+16,96);put64(p+24,1);put64(p+32,32);}
int main(void){
 unsigned char bytes[96];valid(bytes);sl_linux_failed_archive_header o={0};
 assert(!sl_linux_interpret_failed_archive_header(&o,bytes,sizeof(bytes)));
 assert(o.bytes_original==bytes&&o.extent_original==96&&o.record_count==1&&o.returned);
 sl_linux_failed_archive_header saved=o;
 assert(sl_linux_interpret_failed_archive_header(&o,bytes,sizeof(bytes))<0);
 assert(!memcmp(&o,&saved,sizeof(o)));
 for(unsigned mode=0;mode<8;mode++){
  valid(bytes);memset(&o,0,sizeof(o));
  if(mode==0)bytes[7]=1;
  if(mode==1)bytes[8]=2;
  if(mode==2)bytes[12]=1;
  if(mode==3)bytes[48]=1;
  if(mode==4)put64(bytes+32,UINT64_MAX);
  if(mode==5)put64(bytes+24,0);
  if(mode==6)put64(bytes+24,2);
  if(mode==7)put64(bytes+40,1);
  assert(sl_linux_interpret_failed_archive_header(&o,bytes,sizeof(bytes))<0);
  assert(o.attempted&&o.returned&&o.bytes_original==bytes);
 }
 /* Actual output/input overlap rejects before changing either occurrence. */
 union {sl_linux_failed_archive_header output;unsigned char bytes[128];} alias={0};
 unsigned char snapshot[128];memcpy(snapshot,alias.bytes,sizeof(snapshot));
 assert(sl_linux_interpret_failed_archive_header(&alias.output,alias.bytes,96)<0);
 assert(!memcmp(snapshot,alias.bytes,sizeof(snapshot)));
 memset(&o,0,sizeof(o));valid(bytes);
 assert(sl_linux_interpret_failed_archive_header(&o,bytes,63)<0&&!o.attempted);
 return 0;
}
