//
// Akron: a 128-bit noncryptographic hash function
// By Bob Jenkins October 2010.  Public domain.  
//
// The internal state is fully overwritten every 96 bytes.
// Every input bit affects at least 128 internal state bits
// before 96 other bytes are combined, when run forward or backward
//   For every input bit,
//   Two inputs differing in just that input bit
//   Where "differ" means xor or subtraction
//   And the base value is random
//   When run forward or backwards one Mix
//   The internal state will differ by an average of 
//     109 bits for one pair
//     146 bits for two pairs
//     169 bits for three pairs
//     185 bits for four pairs
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

#define AkronMix(data,h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11) \
  h0 +=(data)[0];  h2 =rot64(h2, 32)^h0;  h4 +=h0;  h0 +=h3; \
  h1 +=(data)[1];  h3 =rot64(h3, 37)^h1;  h5 +=h1;  h1 +=h4; \
  h2 +=(data)[2];  h4 =rot64(h4, 27)^h2;  h6 +=h2;  h2 +=h5; \
  h3 +=(data)[3];  h5 =rot64(h5, 48)^h3;  h7 +=h3;  h3 +=h6; \
  h4 +=(data)[4];  h6 =rot64(h6, 5) ^h4;  h8 +=h4;  h4 +=h7; \
  h5 +=(data)[5];  h7 =rot64(h7, 7) ^h5;  h9 +=h5;  h5 +=h8; \
  h6 +=(data)[6];  h8 =rot64(h8, 50)^h6;  h10+=h6;  h6 +=h9; \
  h7 +=(data)[7];  h9 =rot64(h9, 18)^h7;  h11+=h7;  h7 +=h10;\
  h8 +=(data)[8];  h10=rot64(h10,9) ^h8;  h0 +=h8;  h8 +=h11;\
  h9 +=(data)[9];  h11=rot64(h11,44)^h9;  h1 +=h9;  h9 +=h0; \
  h10+=(data)[10]; h0 =rot64(h0, 14)^h10; h2 +=h10; h10+=h1; \
  h11+=(data)[11]; h1 =rot64(h1, 30)^h11; h3 +=h11; h11+=h2;

//
// ShortHash: A 128-bit noncryptographic hash function
// AkronHash uses this for short messages (under 96 bytes)
// It is slower than Akron in the long run, but it starts faster
// 
void ShortHash(
  const void *message,  // message to hash
  size_t length,        // length of message
  uint64_t *hash1,      // in/out: in seed 1, out hash value 1
  uint64_t *hash2);     // in/out: in seed 2, out hash value 2


typedef struct Akron
{
  uint64_t data[12];
  uint64_t state[12];
  uint64_t length;
  uint8_t  remainder;
} Akron;

//
// AkronHash: hash a single message in one call
//
void AkronHash(
  const void *message,  // message to hash
  size_t length,        // length of message
  uint64_t *hash1,      // in/out: in seed 1, out hash value 1
  uint64_t *hash2);     // in/out: 


//
// AkronInit: initialize the context of a Akron hash
//
void AkronInit(
  Akron *context, // context to initialize
  uint64_t hash1,       // seed1
  uint64_t hash2);      // seed2

//
// AkronUpdate: add a piece of a message to a Akron state
//
void AkronUpdate(
  Akron *state, 
  const void *message, 
  uint64_t length);


//
// AkronFinal: compute the hash for the current Akron state
//
// This does not modify the state; you can keep updating it afterward
//
// The result is the same as if AkronHash() had been called with
// all the pieces concatenated into one message.
//
void AkronFinal(
  Akron *state, 
  uint64_t *hash1, 
  uint64_t *hash2);
