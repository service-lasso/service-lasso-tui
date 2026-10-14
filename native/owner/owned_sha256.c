/* SHA-256 source recipe: NIST FIPS180-4 sections4.1.2/4.2.2/5/6.2.
 * https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
 * UNRUN/unvalidated; matching digest is byte correspondence, not authority. */
#include "owned_sha256.h"
#include <errno.h>
static const uint32_t initial[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
static const uint32_t constants[64]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
static uint32_t rotate(uint32_t v,unsigned n){return(v>>n)|(v<<(32-n));}
static uint32_t small0(uint32_t v){return rotate(v,7)^rotate(v,18)^(v>>3);}
static uint32_t small1(uint32_t v){return rotate(v,17)^rotate(v,19)^(v>>10);}
static uint32_t big0(uint32_t v){return rotate(v,2)^rotate(v,13)^rotate(v,22);}
static uint32_t big1(uint32_t v){return rotate(v,6)^rotate(v,11)^rotate(v,25);}
static long sha_finish(sl_owned_sha256 *o,long result){o->result=result;o->returned=1;return result;}
long sl_owned_sha256_compute(sl_owned_sha256 *o){
 if(!o||o->attempted)return -EINVAL;o->attempted=1;
 o->actual_body=o->body;o->actual_extent=o->extent;o->actual_workspace=o->workspace;
 sl_owned_sha256_workspace *f=o->actual_workspace;
 if((o->actual_extent&&!o->actual_body)||!f||f->attempted||f->returned||f->owner_original||o->actual_extent>UINT64_MAX/8||o->actual_extent>SIZE_MAX-72)return sha_finish(o,-EINVAL);
 f->owner_original=o;f->attempted=1;f->input_original=o->actual_body;f->input_extent=o->actual_extent;
 o->bit_length=(uint64_t)o->actual_extent*8;
 o->required=o->actual_extent/64+(o->actual_extent%64<56?1:2);
 for(unsigned j=0;j<8;j++)f->chaining[j]=initial[j];
 for(size_t i=0;i<o->required;i++){
  f->block=i;f->block_offset=i*64;
  size_t available=f->block_offset<o->actual_extent?o->actual_extent-f->block_offset:0;f->block_extent=available<64?available:64;
  for(unsigned j=0;j<64;j++){
   size_t at=f->block_offset+j;unsigned char byte=0;
   if(at<o->actual_extent)byte=o->actual_body[at];else if(at==o->actual_extent)byte=0x80;
   if(i==o->required-1&&j>=56)byte=(unsigned char)(o->bit_length>>(8*(63-j)));
   f->padded[j]=byte;
  }
  for(unsigned j=0;j<16;j++)f->schedule[j]=(uint32_t)f->padded[j*4]<<24|(uint32_t)f->padded[j*4+1]<<16|(uint32_t)f->padded[j*4+2]<<8|f->padded[j*4+3];
  for(unsigned j=16;j<64;j++)f->schedule[j]=small1(f->schedule[j-2])+f->schedule[j-7]+small0(f->schedule[j-15])+f->schedule[j-16];
  for(unsigned j=0;j<8;j++)f->working[j]=f->chaining[j];
  for(unsigned j=0;j<64;j++){
   f->round=j;uint32_t *v=f->working;
   f->temporary[0]=v[7]+big1(v[4])+((v[4]&v[5])^(~v[4]&v[6]))+constants[j]+f->schedule[j];
   f->temporary[1]=big0(v[0])+((v[0]&v[1])^(v[0]&v[2])^(v[1]&v[2]));
   v[7]=v[6];v[6]=v[5];v[5]=v[4];v[4]=v[3]+f->temporary[0];v[3]=v[2];v[2]=v[1];v[1]=v[0];v[0]=f->temporary[0]+f->temporary[1];
  }
  for(unsigned j=0;j<8;j++)f->chaining[j]+=f->working[j];f->completed_blocks=i+1;
 }
 for(unsigned j=0;j<32;j++)o->digest[j]=(unsigned char)(f->chaining[j/4]>>(8*(3-j%4)));
 f->returned=1;return sha_finish(o,0);
}