/* UNRUN controlled source representation cases; no owner authorization grant. */
#include "candidate_profile.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
static void le(unsigned char *p,uint64_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(unsigned char)(v>>(8*i));}
typedef struct {unsigned char bytes[1024];size_t at,role_offset;sl_candidate_profile profile;sl_candidate_record evidence[1],roles[1];} fixture;
static void section(fixture *f,unsigned id,const unsigned char *body,size_t n){le(f->bytes+f->at,id,4);le(f->bytes+f->at+8,n,8);f->at+=16;memcpy(f->bytes+f->at,body,n);f->at+=n;}
static void construct(fixture *f){
 static const unsigned char magic[8]={0x53,0x4c,0x43,0x50,0x52,0x4f,0x46,0x31};
 memcpy(f->bytes,magic,8);le(f->bytes+8,1,4);le(f->bytes+12,9,4);f->at=32;
 unsigned char digest[32]={1},policy[72]={0},q[24]={0},empty[8]={0};
 section(f,1,digest,32);section(f,2,digest,32);le(policy+32,3,8);section(f,3,policy,72);
 le(q,1,8);le(q+8,1,8);le(q+16,1,8);section(f,4,q,24);
 unsigned char evidence[8+56+9]={0};le(evidence,1,8);
 for(unsigned i=0;i<7;i++)le(evidence+8+i*8,i==4?0:i==6?4:1,8);
 memcpy(evidence+64,"abcdeBODY",9);section(f,5,evidence,sizeof(evidence));
 unsigned char role[8+64+48+2]={0};le(role,1,8);
 for(unsigned i=0;i<8;i++)le(role+8+i*8,i<6?8:1,8);
 le(role+72+8,1,8);le(role+72+16,2,8);le(role+72+24,1,8);le(role+72+32,1,8);le(role+72+40,1,8);role[120]='A';role[121]='E';
 f->role_offset=f->at+16+72;section(f,6,role,sizeof(role));section(f,7,empty,8);section(f,8,empty,8);section(f,9,empty,8);le(f->bytes+16,f->at,8);
 f->profile.source=f->bytes;f->profile.source_extent=f->at;
 f->profile.rows[0]=f->evidence;f->profile.capacities[0]=1;f->profile.rows[1]=f->roles;f->profile.capacities[1]=1;
}
int main(void){
 fixture good={0};construct(&good);assert(sl_candidate_decode_profile(&good.profile)==0);
 assert(good.profile.original.bytes==good.bytes&&good.profile.returned&&good.profile.counts[0]==1&&good.profile.counts[1]==1);
 assert(good.roles[0].evidence_original==&good.evidence[0]);
 assert(good.roles[0].statement_original.bytes==good.evidence[0].fields[6].bytes+1&&good.roles[0].statement_original.extent==2);
 assert(!memcmp(good.roles[0].statement_original.bytes,"OD",2));
 assert(sl_candidate_decode_profile(&good.profile)==-EINVAL&&good.profile.result==0);
 fixture missing={0};construct(&missing);missing.profile.capacities[1]=0;
 assert(sl_candidate_decode_profile(&missing.profile)==-EOVERFLOW&&missing.profile.returned&&missing.profile.counts[0]==1);
 fixture truncated={0};construct(&truncated);truncated.profile.source_extent--;
 assert(sl_candidate_decode_profile(&truncated.profile)==-EINVAL&&truncated.profile.returned);
 fixture wrong={0};construct(&wrong);
 /* Invalid evidence index, with all original bytes/row cells independently held. */
 le(wrong.bytes+wrong.role_offset,1,8);

 assert(sl_candidate_decode_profile(&wrong.profile)==-EINVAL&&wrong.roles[0].evidence_index==1&&wrong.profile.returned);
 return 0;
}