/**
 * SHA-256 Implementation for UEFI
 *
 * Based on NIST FIPS 180-4 specification.
 * Optimized for EFI environment without standard C library dependencies.
 */

#include "mainHeader.h"
#include "SHA256header.h"

/* SHA-256 Constants (first 32 bits of fractional parts of cube roots of first 64 primes) */
static CONST UINT32 K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* Rotate right */
static inline UINT32 rotr32(UINT32 x, UINT32 n) {
    return (x >> n) | (x << (32 - n));
}

/* SHA-256 specific functions */
static inline UINT32 ch(UINT32 x, UINT32 y, UINT32 z) {
    return (x & y) ^ (~x & z);
}

static inline UINT32 maj(UINT32 x, UINT32 y, UINT32 z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static inline UINT32 ep0(UINT32 x) {
    return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22);
}

static inline UINT32 ep1(UINT32 x) {
    return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25);
}

static inline UINT32 sig0(UINT32 x) {
    return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3);
}

static inline UINT32 sig1(UINT32 x) {
    return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10);
}

/* Pack 4 bytes into big-endian 32-bit word */
static inline UINT32 pack32(const UINT8* c) {
    return ((UINT32)c[0] << 24) | ((UINT32)c[1] << 16) |
           ((UINT32)c[2] << 8) | (UINT32)c[3];
}

/* Unpack 32-bit word into bytes */
static inline UINT8 unpack32(UINT32 x, UINT32 n) {
    return (UINT8)((x >> n) & 0xff);
}

static void sha256_transform(sha256_context* ctx) {
    UINT32 a, b, c, d, e, f, g, h;
    UINT32 t1, t2;
    UINT32 m[64];

    /* Prepare message schedule */
    for (UINT32 i = 0; i < 64; i++) {
        if (i < 16) {
            m[i] = pack32(&ctx->buf[i * 4]);
        } else {
            m[i] = sig1(m[i - 2]) + m[i - 7] + sig0(m[i - 15]) + m[i - 16];
        }
    }

    /* Initialize working variables */
    a = ctx->hash[0];
    b = ctx->hash[1];
    c = ctx->hash[2];
    d = ctx->hash[3];
    e = ctx->hash[4];
    f = ctx->hash[5];
    g = ctx->hash[6];
    h = ctx->hash[7];

    /* Main loop */
    for (UINT32 i = 0; i < 64; i++) {
        t1 = h + ep1(e) + ch(e, f, g) + K[i] + m[i];
        t2 = ep0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    /* Update hash values */
    ctx->hash[0] += a;
    ctx->hash[1] += b;
    ctx->hash[2] += c;
    ctx->hash[3] += d;
    ctx->hash[4] += e;
    ctx->hash[5] += f;
    ctx->hash[6] += g;
    ctx->hash[7] += h;
}

static void sha256_addbits(sha256_context* ctx, UINT32 n) {
    if (ctx->bits[0] > (0xffffffff - n)) {
        ctx->bits[1]++;
    }
    ctx->bits[0] += n;
}

void sha256_init(sha256_context* ctx) {
    if (ctx == NULL) {
        return;
    }

    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
    ctx->len = 0;

    /* Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes) */
    ctx->hash[0] = 0x6a09e667;
    ctx->hash[1] = 0xbb67ae85;
    ctx->hash[2] = 0x3c6ef372;
    ctx->hash[3] = 0xa54ff53a;
    ctx->hash[4] = 0x510e527f;
    ctx->hash[5] = 0x9b05688c;
    ctx->hash[6] = 0x1f83d9ab;
    ctx->hash[7] = 0x5be0cd19;
}

void sha256_update(sha256_context* ctx, CONST UINT8* data, UINTN len) {
    if (ctx == NULL || data == NULL) {
        return;
    }

    for (UINTN i = 0; i < len; i++) {
        ctx->buf[ctx->len++] = data[i];
        if (ctx->len == sizeof(ctx->buf)) {
            sha256_transform(ctx);
            sha256_addbits(ctx, 512);
            ctx->len = 0;
        }
    }
}

void sha256_final(sha256_context* ctx, UINT8* hash) {
    if (ctx == NULL || hash == NULL) {
        return;
    }

    UINT32 i = ctx->len;

    /* Padding */
    ctx->buf[i++] = 0x80;

    if (ctx->len >= 56) {
        while (i < 64) {
            ctx->buf[i++] = 0x00;
        }
        sha256_transform(ctx);
        i = 0;
    }

    while (i < 56) {
        ctx->buf[i++] = 0x00;
    }

    /* Append length in bits */
    sha256_addbits(ctx, (UINT32)(ctx->len * 8));

    ctx->buf[63] = (UINT8)(ctx->bits[0]);
    ctx->buf[62] = (UINT8)(ctx->bits[0] >> 8);
    ctx->buf[61] = (UINT8)(ctx->bits[0] >> 16);
    ctx->buf[60] = (UINT8)(ctx->bits[0] >> 24);
    ctx->buf[59] = (UINT8)(ctx->bits[1]);
    ctx->buf[58] = (UINT8)(ctx->bits[1] >> 8);
    ctx->buf[57] = (UINT8)(ctx->bits[1] >> 16);
    ctx->buf[56] = (UINT8)(ctx->bits[1] >> 24);

    sha256_transform(ctx);

    /* Write output hash */
    for (i = 0; i < 4; i++) {
        hash[i + 0]  = (UINT8)(ctx->hash[0] >> (24 - i * 8));
        hash[i + 4]  = (UINT8)(ctx->hash[1] >> (24 - i * 8));
        hash[i + 8]  = (UINT8)(ctx->hash[2] >> (24 - i * 8));
        hash[i + 12] = (UINT8)(ctx->hash[3] >> (24 - i * 8));
        hash[i + 16] = (UINT8)(ctx->hash[4] >> (24 - i * 8));
        hash[i + 20] = (UINT8)(ctx->hash[5] >> (24 - i * 8));
        hash[i + 24] = (UINT8)(ctx->hash[6] >> (24 - i * 8));
        hash[i + 28] = (UINT8)(ctx->hash[7] >> (24 - i * 8));
    }
}

void sha256_hash(CONST UINT8* data, UINTN len, UINT8* hash) {
    sha256_context ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}
