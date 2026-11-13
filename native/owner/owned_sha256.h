#ifndef SERVICE_LASSO_OWNED_SHA256_H
#define SERVICE_LASSO_OWNED_SHA256_H
#include <stdint.h>
#include <stddef.h>
struct sl_owned_sha256;
typedef struct {
 struct sl_owned_sha256 *owner_original;
 const unsigned char *input_original;size_t input_extent,block_offset,block_extent;
 unsigned char padded[64];uint32_t schedule[64],working[8],temporary[2],chaining[8];
 size_t block,completed_blocks;unsigned round,attempted,returned;
} sl_owned_sha256_workspace;
typedef struct sl_owned_sha256 {
 const unsigned char *body,*actual_body;size_t extent,actual_extent;
 sl_owned_sha256_workspace *workspace,*actual_workspace;
 size_t required;uint64_t bit_length;unsigned char digest[32];
 long result;unsigned attempted,returned;
} sl_owned_sha256;
/* One source-owned exclusive workspace per operation. Input remains immutable
 * and borrowed through real return. Mutable arithmetic state is not an acquired
 * resource/error graph or a required full arithmetic transcript. */
long sl_owned_sha256_compute(sl_owned_sha256 *owner);
#endif