// SpookyHash
// A 128-bit noncryptographic hash, for checksums and table lookup
// By Bob Jenkins.  Public domain.  Alpha quality (function will change)
//   Oct 31 2010: published framework, disclaimer ShortHash isn't right
//   Nov 7 2010: disabled ShortHash

#include "SpookyAlpha.h"

#define BLOCKSIZE 96

static const uint64_t m = 0xba6b2ad56aad55c5LL; 
static const uint64_t m1 = 0xba6b2ad56aad55c5LL; 
static const uint64_t m2 = 0x729ba0e7afa49347LL; 

// short hash ... it could be used on any message, 
// but it's used by Akron just for short messages.
void ShortHash(
  const void *message,
  size_t length,
  uint64_t *hash1,
  uint64_t *hash2)
{
  const uint64_t *data = (const uint64_t *)message;
  const uint64_t *end = data + (length/16)*2;
  size_t remainder = length - (length/16)*16;
  uint64_t h0 = *hash1 * m1;
  uint64_t h1 = *hash2 * m2;
  uint64_t temp;

  h0 = rot64(h0, 32) * m2;
  h1 = rot64(h1, 32) * m1;
  h0 ^= h1;
  h1 += h0;

  // handle all complete sets of 16 bytes
  for (; data < end; data += 2) {
    h0 = rot64(h0, 32) * m1;
    temp = data[0] * m1;
    temp = rot64(temp, 32) * m2;
    h0 += rot64(temp, 32);
    h0 *= m1;
    
    h1 = rot64(h1, 32) * m2;
    temp = data[1] * m2;
    temp = rot64(temp, 32) * m1;
    h1 += rot64(temp, 32);
    h1 *= m2;
    
    h0 ^= h1;
    h1 += h0;
  }

  // handle the last partial 16 bytes
  temp = ((uint64_t)remainder) << 56;
  switch (remainder) {
  case 15:
    temp += ((uint64_t)(((uint8_t *)data)[14])) << 48;
  case 14:
    temp += ((uint64_t)(((uint8_t *)data)[13])) << 40;
  case 13:
    temp += ((uint64_t)(((uint8_t *)data)[12])) << 32;
  case 12:
    temp += ((uint64_t)(((uint8_t *)data)[11])) << 24;
  case 11:
    temp += ((uint64_t)(((uint8_t *)data)[10])) << 16;
  case 10:
    temp += ((uint64_t)(((uint8_t *)data)[9])) << 8;
  case 9:
    temp += (uint64_t)(((uint8_t *)data)[8]);

    temp *= m2;
    temp = rot64(temp, 32) * m1;
    h1 += rot64(temp, 32);
    h1 *= m2;

  case 8:
    temp = data[0] * m1;
    temp = rot64(temp, 32) * m2;
    h0 += rot64(temp, 32);
    h0 *= m1;

    h0 ^= h1;
    h1 += h0;

    break;

  case 7:
    temp += ((uint64_t)(((uint8_t *)data)[6])) << 48;
  case 6:
    temp += ((uint64_t)(((uint8_t *)data)[5])) << 40;
  case 5:
    temp += ((uint64_t)(((uint8_t *)data)[4])) << 32;
  case 4:
    temp += ((uint64_t)(((uint8_t *)data)[3])) << 24;
  case 3:
    temp += ((uint64_t)(((uint8_t *)data)[2])) << 16;
  case 2:
    temp += ((uint64_t)(((uint8_t *)data)[1])) << 8;
  case 1:
    temp += (uint64_t)(((uint8_t *)data)[0]);
  case 0:
    temp *= m1;
    temp = rot64(temp, 32) * m2;
    h1 += temp;
    h0 += rot64(temp, 32);
    h0 *= m1;

    h0 ^= h1;
    h1 += h0;
  }

  *hash1 = rot64(h0, 32) * m2;
  *hash2 = rot64(h1, 32) * m1;
}




// do the whole hash in one call
void SpookyHash(
  const void *message, 
  size_t length, 
  uint64_t *hash1, 
  uint64_t *hash2)
{
  if (0 && length < BLOCKSIZE) {

    ShortHash(message, length, hash1, hash2);
    return;

  } else {

    uint64_t h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11;
    uint64_t buf[12];
    const uint64_t *data = (uint64_t *)message;
    const uint64_t *end = data + (length/(12*8))*12;
    size_t remainder;

    h0=h3=h6=h9 = *hash1 * m;
    h1=h4=h7=h10 = *hash2 * m;
    h2=h5=h8=h11 = m;

    // handle all whole blocks of BLOCKSIZE bytes
    for (; data < end; data+=12) { 
      SpookyMix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    }

    // handle the last partial block of BLOCKSIZE bytes
    remainder = (length - ((const uint8_t *)end - (const uint8_t *)message));
    memcpy(buf, end, remainder);
    memset(((uint8_t *)buf)+remainder, 0, BLOCKSIZE-remainder);
    ((uint8_t *)buf)[BLOCKSIZE-1] = (uint8_t)remainder;
    SpookyMix(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);

    // do some final mixing 
    SpookyMix(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    SpookyMix(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    SpookyMix(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    *hash1 = h11;
    *hash2 = h0;
 }
}



// init spooky state
void SpookyInit(Spooky *state, uint64_t seed1, uint64_t seed2)
{
  state->length = 0;
  state->remainder = 0;
  state->state[0] = seed1;
  state->state[1] = seed2;
}


// add a message fragment to the state
void SpookyUpdate(Spooky *state, const void *message, uint64_t length)
{
  uint64_t h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11;
  uint64_t newLength = length + state->remainder;
  uint8_t  remainder;
  const uint64_t *data;
  const uint64_t *end;

  // Is this message fragment too short?  If it is, stuff it away.
  if (newLength < BLOCKSIZE) {
    memcpy(&((uint8_t *)state->data)[state->remainder], message, length);
    state->length = length + state->length;
    state->remainder = (uint8_t)newLength;
    return;
  }

  // init the variables
  if (state->length < BLOCKSIZE) {
    h0=h3=h6=h9 = state->state[0] * m;
    h1=h4=h7=h10 = state->state[1] * m;
    h2=h5=h8=h11 = m;
  } else {
    h0 = state->state[0];
    h1 = state->state[1];
    h2 = state->state[2];
    h3 = state->state[3];
    h4 = state->state[4];
    h5 = state->state[5];
    h6 = state->state[6];
    h7 = state->state[7];
    h8 = state->state[8];
    h9 = state->state[9];
    h10 = state->state[10];
    h11 = state->state[11];
  }
  state->length = length + state->length;

  // if we've got anything stuffed away, use it now
  if (state->remainder) {
    uint8_t prefix = BLOCKSIZE-state->remainder;
    memcpy(&((uint8_t *)state->data)[state->remainder], 
	   message,
	   prefix);
    data = (const uint64_t *)state->data;
    SpookyMix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    data = (const uint64_t *)&((const uint8_t *)message)[prefix];
    length -= prefix;
  } else {
    data = (const uint64_t *)message;
  }

  // handle all whole blocks of BLOCKSIZE bytes
  end = data + (length/BLOCKSIZE)*12;
  remainder = (uint8_t)(length - ((const uint8_t *)end-(const uint8_t *)data));
  for (; data < end; data+=12) { 
    SpookyMix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
  }

  // stuff away the last few bytes
  state->remainder = remainder;
  memcpy(state->data, end, remainder);

  // stuff away the variables
  state->state[0] = h0;
  state->state[1] = h1;
  state->state[2] = h2;
  state->state[3] = h3;
  state->state[4] = h4;
  state->state[5] = h5;
  state->state[6] = h6;
  state->state[7] = h7;
  state->state[8] = h8;
  state->state[9] = h9;
  state->state[10] = h10;
  state->state[11] = h11;
}


// report the hash for the concatenation of all message fragments so far
void SpookyFinal(Spooky *state, uint64_t *hash1, uint64_t *hash2)
{
  uint64_t h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11;
  const uint64_t *data;
  const uint64_t *end;
  uint8_t remainder, prefix;

  // init the variables
  if (state->length < BLOCKSIZE) {

    if (0) {
      ShortHash( state->data, (size_t)state->remainder, hash1, hash2);
      return;
    }

    h0=h3=h6=h9 = state->state[0] * m;
    h1=h4=h7=h10 = state->state[1] * m;
    h2=h5=h8=h11 = m;

  } else {

    h0 = state->state[0];
    h1 = state->state[1];
    h2 = state->state[2];
    h3 = state->state[3];
    h4 = state->state[4];
    h5 = state->state[5];
    h6 = state->state[6];
    h7 = state->state[7];
    h8 = state->state[8];
    h9 = state->state[9];
    h10 = state->state[10];
    h11 = state->state[11];

  }

  // mix in the last partial block, and the length mod BLOCKSIZE
  remainder = state->remainder;
  prefix = (uint8_t)(BLOCKSIZE-remainder);
  memset(&((uint8_t *)state->data)[remainder], 0, prefix);
  ((uint8_t *)state->data)[BLOCKSIZE-1] = remainder;
  data = (const uint64_t *)state->data;
  SpookyMix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);

  // do some final mixing
  SpookyMix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
  SpookyMix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
  SpookyMix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
  *hash1 = h11;
  *hash2 = h0;
}

