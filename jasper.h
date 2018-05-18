//
// Jasper: a 128-bit noncryptographic hash function
// By Bob Jenkins, September 2010, public domain.  Alpha-quality, beware.
//
// The internal state is fully overwritten every 96 bytes.
// Every input bit affects at least 128 internal state bits
// before 96 other bytes are combined, when run forward or backward
//
// Flipping any input bit flips each output bit with probability 50% += .1%
// 
// 3 bytes/cycle for long messages.
//
// This was developed for and tested on 64-bit x86-compatible processors.
// It assumes the processor is little-endian, that multiplication is fast,
// that unaligned 8-byte reads are allowed, and a rotate operation exists.
//
// The target use is checksums for large (1KB+) messages.  The main 
// competitor is CRCs, which are several times slower, but have nice math 
// for combining the CRCs of pieces to form new pieces.  There are also
// cryptographic hashes, but those are orders of magnitude slower.
//
#include <stddef.h>

typedef  unsigned long long uint64_t;
typedef  unsigned char      uint8_t;

#define rot64(x,k) (((x)<<(k)) | ((x)>>(64-(k)))) 

#define JasperMix1(temp, data, i, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11) \
  temp = (data)[i] * 0xba6b2ad56aad55c5LL; \
  h0  -= temp; \
  h1 -= h3; \
  h3 -= h5; \
  h3 = rot64(h3, 43); \
  h3 ^= h0;

#define JasperMix(temp,data,h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11) \
  JasperMix1(temp, data, 0, h0,h11,h10,h9,h8,h7,h6,h5,h4,h3,h2,h1) \
  JasperMix1(temp, data, 1, h11,h10,h9,h8,h7,h6,h5,h4,h3,h2,h1,h0) \
  JasperMix1(temp, data, 2, h10,h9,h8,h7,h6,h5,h4,h3,h2,h1,h0,h11) \
  JasperMix1(temp, data, 3, h9,h8,h7,h6,h5,h4,h3,h2,h1,h0,h11,h10) \
  JasperMix1(temp, data, 4, h8,h7,h6,h5,h4,h3,h2,h1,h0,h11,h10,h9) \
  JasperMix1(temp, data, 5, h7,h6,h5,h4,h3,h2,h1,h0,h11,h10,h9,h8) \
  JasperMix1(temp, data, 6, h6,h5,h4,h3,h2,h1,h0,h11,h10,h9,h8,h7) \
  JasperMix1(temp, data, 7, h5,h4,h3,h2,h1,h0,h11,h10,h9,h8,h7,h6) \
  JasperMix1(temp, data, 8, h4,h3,h2,h1,h0,h11,h10,h9,h8,h7,h6,h5) \
  JasperMix1(temp, data, 9, h3,h2,h1,h0,h11,h10,h9,h8,h7,h6,h5,h4) \
  JasperMix1(temp, data, 10,h2,h1,h0,h11,h10,h9,h8,h7,h6,h5,h4,h3) \
  JasperMix1(temp, data, 11,h1,h0,h11,h10,h9,h8,h7,h6,h5,h4,h3,h2)


//
// ShortHash: A 128-bit noncryptographic hash function
// JasperHash uses this for short messages (under 96 bytes)
// It is slower than Japser in the long run, but it starts faster
// 
void ShortHash(
  const void *message,  // message to hash
  size_t length,        // length of message
  uint64_t *hash1,      // in/out: in seed 1, out hash value 1
  uint64_t *hash2);     // in/out: in seed 2, out hash value 2


typedef struct Jasper
{
  uint64_t data[12];
  uint64_t state[12];
  uint64_t length;
  uint8_t  remainder;
} Jasper;

//
// JasperHash: hash a single message in one call
//
void JasperHash(
  const void *message,  // message to hash
  size_t length,        // length of message
  uint64_t *hash1,      // in/out: in seed 1, out hash value 1
  uint64_t *hash2);     // in/out: in seed 2, out hash value 2


//
// JasperInit: initialize the context of a Jasper hash
//
void JasperInit(
  Jasper *context, // context to initialize
  uint64_t hash1,       // seed1
  uint64_t hash2);      // seed2

//
// JasperUpdate: add a piece of a message to a Jasper state
//
void JasperUpdate(
  Jasper *state, 
  const void *message, 
  uint64_t length);


//
// JasperFinal: compute the hash for the current Jasper state
//
// This does not modify the state; you can keep updating it afterward
//
// The result is the same as if JasperHash() had been called with
// all the pieces concatenated into one message.
//
void JasperFinal(
  Jasper *state, 
  uint64_t *hash1, 
  uint64_t *hash2);
