//
// Spooky: a 128-bit noncryptographic hash function
// By Bob Jenkins, public domain
//   Oct 31 2010: alpha, framework + SpookyMix appears right
//   Nov 7 2010: corrected count of bits flipped
//
// The internal state is fully overwritten every 96 bytes.
// Every input bit appears to cause at least 128 bits of entropy
// before 96 other bytes are combined, when run forward or backward
//   For every input bit,
//   Two inputs differing in just that input bit
//   Where "differ" means xor or subtraction
//   And the base value is random
//   When run forward or backwards one Mix
//   The internal state will differ by an average of 
//     72 bits for one pair (vs 64, for 128 bits of entropy)
//     103 bits for two pairs (vs 96)
//     125 bits for three pairs (vs 112)
//     141 bits for four pairs (vs 120)
//     152 bits for five pairs (vs 124)
//     160 bits for six pairs (vs 126)
//
//   For two inputs differing in just the input bits
//   Where "differ" means xor or subtraction
//   And the base value is random, or a counting value starting at that bit
//   The final result will have each bit flip
//   For every input bit,
//   with probabiliy 50 +- .8% (that's all the more accurately I measured it)
//   For every pair of input bits,
//   with probabiliy 50 +- 3% (that's all the more accurately I measured it)
// 
// 4 bytes/cycle for long messages.
//
// This was developed for and tested on 64-bit x86-compatible processors.
// It assumes the processor is little-endian, that multiplication is fast,
// that unaligned 8-byte reads are allowed, and a rotate operation exists.
//
// An unrelated hash (the short hash) is used for inputs less than 96 
// bytes long.
//
// The target use is checksums for large (1KB+) messages.  The main 
// competitor is CRCs, which are several times slower, but have nice math 
// for combining the CRCs of pieces to form new pieces.  There are also
// cryptographic hashes, but those are orders of magnitude slower.
//
#include <stddef.h>
#include <memory.h>

typedef  unsigned long long uint64_t;
typedef  unsigned char      uint8_t;

#define rot64(x,k) (((x)<<(k)) | ((x)>>(64-(k)))) 

#define SpookyMix(data,h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11) \
  h0 +=(data)[0];  h11=rot64(h11,32); h9 ^=h1;  h11+=h10; h1 +=h10;\
  h1 +=(data)[1];  h0 =rot64(h0, 41); h10^=h2;  h0 +=h11; h2 +=h11;\
  h2 +=(data)[2];  h1 =rot64(h1, 12); h11^=h3;  h1 +=h0;  h3 +=h0; \
  h3 +=(data)[3];  h2 =rot64(h2, 24); h0 ^=h4;  h2 +=h1;  h4 +=h1; \
  h4 +=(data)[4];  h3 =rot64(h3, 8);  h1 ^=h5;  h3 +=h2;  h5 +=h2; \
  h5 +=(data)[5];  h4 =rot64(h4, 42); h2 ^=h6;  h4 +=h3;  h6 +=h3; \
  h6 +=(data)[6];  h5 =rot64(h5, 32); h3 ^=h7;  h5 +=h4;  h7 +=h4; \
  h7 +=(data)[7];  h6 =rot64(h6, 13); h4 ^=h8;  h6 +=h5;  h8 +=h5; \
  h8 +=(data)[8];  h7 =rot64(h7, 30); h5 ^=h9;  h7 +=h6;  h9 +=h6; \
  h9 +=(data)[9];  h8 =rot64(h8, 20); h6 ^=h10; h8 +=h7;  h10+=h7; \
  h10+=(data)[10]; h9 =rot64(h9, 47); h7 ^=h11; h9 +=h8;  h11+=h8; \
  h11+=(data)[11]; h10=rot64(h10,16); h8 ^=h0;  h10+=h9;  h0 +=h9; \

//
// ShortHash: A 128-bit noncryptographic hash function
// SpookyHash uses this for short messages (under 96 bytes)
// It is slower than Spooky in the long run, but it starts faster
// 
void ShortHash(
  const void *message,  // message to hash
  size_t length,        // length of message
  uint64_t *hash1,      // in/out: in seed 1, out hash value 1
  uint64_t *hash2);     // in/out: in seed 2, out hash value 2


typedef struct Spooky
{
  uint64_t data[12];
  uint64_t state[12];
  uint64_t length;
  uint8_t  remainder;
} Spooky;

//
// SpookyHash: hash a single message in one call
//
void SpookyHash(
  const void *message,  // message to hash
  size_t length,        // length of message
  uint64_t *hash1,      // in/out: in seed 1, out hash value 1
  uint64_t *hash2);     // in/out: 


//
// SpookyInit: initialize the context of a Spooky hash
//
void SpookyInit(
  Spooky *context, // context to initialize
  uint64_t hash1,       // seed1
  uint64_t hash2);      // seed2

//
// SpookyUpdate: add a piece of a message to a Spooky state
//
void SpookyUpdate(
  Spooky *state, 
  const void *message, 
  uint64_t length);


//
// SpookyFinal: compute the hash for the current Spooky state
//
// This does not modify the state; you can keep updating it afterward
//
// The result is the same as if SpookyHash() had been called with
// all the pieces concatenated into one message.
//
void SpookyFinal(
  Spooky *state, 
  uint64_t *hash1, 
  uint64_t *hash2);
