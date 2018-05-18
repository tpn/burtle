/*
------------------------------------------------------------------------------
mix.h: block cipher and related functions
By Bob Jenkins, 1997.  Public Domain.
------------------------------------------------------------------------------
*/
#ifndef STANDARD
#include "standard.h"
#endif

#ifndef MIX
#define MIX

#define MIXSIZE 128                /* number of bits in each block */

/* the block cipher: mix the seed */
void mix(/*_ ub4 *x _*/);

/* decryption: the reverse of mix */
void unmix(/*_ ub4 *x _*/);

/* given the seed at some point in the mix, roll back to the start */
void preimage(/*_ ub4 *x _*/);

/* given the seed at some point in the mix, roll forward to the end */
void postimage(/*_ ub4 *x _*/);

#endif /* MIX */
