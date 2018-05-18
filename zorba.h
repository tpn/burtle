/*
--------------------------------------------------------------------------
A x86-specific noncryptographic hash, about 3 bytes per cycle

I might be finished with the function, but I need to work on the 
performance of small or unaligned values.

By Bob Jenkins, Jan 1 2009, public domain
--------------------------------------------------------------------------
*/

#ifndef ZORBA
#define ZORBA

#include <emmintrin.h>

typedef  unsigned long long  u8;
typedef  unsigned long int   u4;
typedef  unsigned short int  u2;
typedef  unsigned char       u1;

typedef union z128 {
  __m128i h;
  u8      x[2];
} z128;

#define BYTES_PER_VAL  16
#define BLOCK          48
#define BUFFERED       (BLOCK * BYTES_PER_VAL)

/*------------------------------------------ public structures and routines */

/* an internal state for hashing a message in pieces */
typedef  struct zorba {
  z128 accum[BLOCK];  /* accumulators: l0..3 .. c0..3 */
  z128 data[BLOCK+4]; /* unconsumed data, plus space for key+length+padding */
  z128 s[4];          /* the internal states */
  u8  messagelen;     /* cumulative length of the message */
  int datalen;        /* length in data[] (in bytes) */
} zorba;

/* init: initialize a hash */
void init(zorba *z);

/* update: hash a piece of a message */
void update(zorba *z, const void *data, size_t len);

/* final: report the hash value for the entire message */
z128 final(zorba *z, const void *key, size_t klen);
#define final64(z,key,len) (final8(z,key,len).x[0])
#define final32(z,key,len) ((u4)(final8(z,key,len).x[0]))

/* keyhash: given the whole message and a key, return the hash value */
z128 keyhash(const void *message, size_t mlen, const void *key, size_t klen);
#define keyhash64(m,ml,k,kl) (keyhash(m,ml,k,kl).x[0])
#define keyhash32(m,ml,k,kl) ((u4)(keyhash(m,ml,k,kl).x[0]))

/* hash: given the whole message, return the hash value */
z128 hash(const void *message, size_t mlen);
#define hash64(m,ml) (hash(m,ml).x[0])
#define hash32(m,ml) ((u4)(hash(m,ml).x[0]))

#endif
