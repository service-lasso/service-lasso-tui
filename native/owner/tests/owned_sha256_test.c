/* UNRUN known-answer and custody source cases, not cryptographic validation. */
#include "../owned_sha256.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
static unsigned digit(char c){return c<='9'?(unsigned)(c-'0'):(unsigned)(c-'a'+10);}
static void expected(const unsigned char *actual,const char *hex){for(unsigned i=0;i<32;i++)assert(actual[i]==(digit(hex[i*2])*16+digit(hex[i*2+1])));}
int main(void){
 static const unsigned char abc[]="abc",long_message[]="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
 sl_owned_sha256 empty={0},short_input={0},two_blocks={0},unfunded={0},reused={0};
 sl_owned_sha256_workspace empty_work={0},short_work={0},long_work={0};
 empty.workspace=&empty_work;assert(sl_owned_sha256_compute(&empty)==0);
 expected(empty.digest,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 short_input.body=abc;short_input.extent=3;short_input.workspace=&short_work;assert(sl_owned_sha256_compute(&short_input)==0);
 expected(short_input.digest,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 assert(short_work.input_original==abc&&short_work.input_extent==3&&short_work.owner_original==&short_input&&short_work.returned&&short_work.padded[3]==0x80&&short_work.padded[63]==24);
 two_blocks.body=long_message;two_blocks.extent=sizeof(long_message)-1;two_blocks.workspace=&long_work;assert(sl_owned_sha256_compute(&two_blocks)==0&&long_work.completed_blocks==2&&two_blocks.actual_workspace==&long_work);
 expected(two_blocks.digest,"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
 assert(long_work.block==1&&long_work.input_original==long_message&&long_work.input_extent==56&&long_work.padded[62]==1&&long_work.padded[63]==192);
 unfunded.body=long_message;unfunded.extent=sizeof(long_message)-1;
 assert(sl_owned_sha256_compute(&unfunded)==-EINVAL&&unfunded.actual_body==long_message&&!unfunded.actual_workspace&&unfunded.returned);
 reused.body=abc;reused.extent=3;reused.workspace=&short_work;
 assert(sl_owned_sha256_compute(&reused)==-EINVAL&&short_work.owner_original==&short_input&&short_work.completed_blocks==1);
 assert(sl_owned_sha256_compute(&short_input)==-EINVAL&&short_input.result==0);expected(short_input.digest,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 return 0;
}