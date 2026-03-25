#ifndef SHA256_H_
#define SHA256_H_

#include "mainHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SHA-256 context structure.
 * Maintains state during incremental hashing.
 */
typedef struct {
    UINT8  buf[64];      /* Current 512-bit chunk being processed */
    UINT32 hash[8];      /* Current hash value */
    UINT32 bits[2];      /* Bit count (high, low) */
    UINT32 len;          /* Length of data in current buffer */
    UINT32 rfu__;        /* Reserved for alignment */
    UINT32 W[64];        /* Message schedule (can be removed to save space) */
} sha256_context;

/**
 * Initialize SHA-256 context.
 *
 * @param ctx  Context to initialize
 */
void sha256_init(sha256_context* ctx);

/**
 * Update hash with additional data.
 *
 * @param ctx  SHA-256 context
 * @param data Input data
 * @param len  Length of input data in bytes
 */
void sha256_update(sha256_context* ctx, CONST UINT8* data, UINTN len);

/**
 * Finalize hash computation and output result.
 *
 * @param ctx  SHA-256 context
 * @param hash Output buffer (32 bytes) for final hash
 */
void sha256_final(sha256_context* ctx, UINT8* hash);

/**
 * One-shot hash computation.
 *
 * @param data Input data to hash
 * @param len  Length of input data in bytes
 * @param hash Output buffer (32 bytes) for hash result
 */
void sha256_hash(CONST UINT8* data, UINTN len, UINT8* hash);

#ifdef __cplusplus
}
#endif

#endif /* SHA256_H_ */
