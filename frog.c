#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

typedef  unsigned char      ub1;
typedef  unsigned short     ub2;
typedef  unsigned long      ub4;
typedef  unsigned long long ub8;

#define hashsize(n) ((ub8)1<<(n))
#define hashmask(n) (hashsize(n)-1)

/*
--------------------------------------------------------------------
mix -- mix 3 64-bit values reversibly.
mix() takes 48 machine instructions, but only 24 cycles on a superscalar
  machine (like Intel's new MMX architecture).  It requires 4 64-bit
  registers for 4::2 parallelism.
All 1-bit deltas, all 2-bit deltas, all deltas composed of top bits of
  (a,b,c), and all deltas of bottom bits were tested.  All deltas were
  tested both on random keys and on keys that were nearly all zero.
  These deltas all cause every bit of c to change between 1/3 and 2/3
  of the time (well, only 113/400 to 287/400 of the time for some
  2-bit delta).  These deltas all cause at least 80 bits to change
  among (a,b,c) when the mix is run either forward or backward (yes it
  is reversible).
This implies that a hash using mix64 has no funnels.  There may be
  characteristics with 3-bit deltas or bigger, I didn't test for
  those.
--------------------------------------------------------------------
*/
#define mix64(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>43); \
  b -= c; b -= a; b ^= (a<<9); \
  c -= a; c -= b; c ^= (b>>8); \
  a -= b; a -= c; a ^= (c>>38); \
  b -= c; b -= a; b ^= (a<<23); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>35); \
  b -= c; b -= a; b ^= (a<<49); \
  c -= a; c -= b; c ^= (b>>11); \
  a -= b; a -= c; a ^= (c>>12); \
  b -= c; b -= a; b ^= (a<<18); \
  c -= a; c -= b; c ^= (b>>22); \
}


/*
--------------------------------------------------------------------
 This works on all machines, is identical to hash() on little-endian 
 machines, and it is much faster than hash(), but it requires
 -- that the key be an array of ub8's, and
 -- that all your machines have the same endianness, and
 -- that the length be the number of ub8's in the key
--------------------------------------------------------------------
*/
ub8 hash2( k, length, level)
ub8 *k;        /* the key */
ub8  length;   /* the length of the key */
ub8  level;    /* the previous hash, or an arbitrary value */
{
  ub8 a,b,c,len;

  /* Set up the internal state */
  len = length;
  a = b = level;                         /* the previous hash value */
  c = 0x9e3779b97f4a7c13LL; /* the golden ratio; an arbitrary value */

  /*---------------------------------------- handle most of the key */
  while (len >= 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix64(a,b,c);
    k += 3; len -= 3;
  }

  /*-------------------------------------- handle the last 2 ub8's */
  c += (length<<3);
  switch(len)              /* all the case statements fall through */
  {
    /* c is reserved for the length */
  case  2: b+=k[1];
  case  1: a+=k[0];
    /* case 0: nothing left to add */
  }
  mix64(a,b,c);
  /*-------------------------------------------- report the result */
  return c;
}


#define BITS   8                 /* maximum number of bits set in the string */
#define BYTES  24                          /* length of the string, in bytes */
#define LARRAY 16                          /* 1<<LARRAY HLEN*ub4s per malloc */
#define LMMM   8                                    /* 1<<LMMM mallocs total */
#define HLEN   2                          /* length of a hash value, in ub4s */
#define ARRAY (((ub4)1)<<LARRAY)              /* length of a malloc, in ub4s */
#define MMM   (((ub4)1)<<LMMM)           /* number of segments in hash table */

struct mystate
{
 ub8    count;                            /* total number of strings tested */
 ub4    a[BITS];           /* a[i] says that q[a[i]] should be the ith word */
 ub4   *hash[MMM];                                  /* last such hash value */
 ub1   *string;                                      /* string to be hashed */
 ub4    x[4];                                      /* results from the hash */
 ub4    mask;                          /* what section of values to look at */
};
typedef  struct mystate  mystate;

void test(mystate *state)
{
 ub4  val;
 ub4  zip;
 ub4  off;
 ub4 *where;
 ub4 *x = state->x;
 ub8  hashval;
 ++state->count;

 /* report how much progress we have made */
 if (!(state->count & (state->count-1)))
 {
   int j;
   for (j=0; (((ub8)1)<<j) < state->count; ++j)
     ;
   printf("count 2^^%d, covered 2^^%d key pairs\n", 
	  j, 
	  j <= LARRAY+LMMM ? 2*j-1 : (LARRAY+LMMM) + j);
 }

 /* compute a new hash value */
 hashval = hash2(state->string, (ub8)(BYTES/8), (ub8)0);
 x[0] = (ub4)hashval;
 x[1] = (ub4)(hashval>>32);

 /* look up where it belongs in the hash table */
 val = x[0];
 zip = ((val>>LMMM)&(ARRAY-1))*HLEN;
 off = (val)&(MMM-1);
 where = &state->hash[off][zip];

 /* did we actually get a collision? */
 if (where[0] == x[0] && where[1] == x[1]) {
   int i;
   printf("collision!  hash value ");
   for (i=0; i<HLEN; ++i) printf("%.8lx ", x[i]);
   printf("count %.8lx %.8lx\n", 
	  (ub4)(state->count>>32), (ub4)(state->count & 0xffffffff));
 }

 /* store the current hash value */
 where[0] = x[0];
 where[1] = x[1];
}

void recurse(int depth, mystate *state)
{
 /* set my bit */
 state->string[state->a[depth]>>3] ^= (1<<(state->a[depth]&0x7));

 test(state);

 /* should we set some more bits? */
 if (depth+1 < BITS) {
   int newdepth = depth+1, i;
   for (i=state->a[depth]; i--;) {
     state->a[newdepth] = i;
     recurse(newdepth, state);
   }
 }

 /* clear my bit */
 state->string[state->a[depth]>>3] ^= (1<<(state->a[depth]&0x7));
}


void driver()
{
 mystate state;
 ub4    *a = state.a;
 ub8     s[(BYTES+7)/8];
 ub4     i, j;

 /* initialize the state */
 for (i=0; i<(BYTES+7)/8; ++i)
   s[i] = (ub8)0;
 state.string = (ub1 *)s;
 state.count  = (ub8)0;

 /* allocate memory for the hash table */
 for (i=0; i<MMM; ++i) {
   state.hash[i] = (ub4 *)malloc(ARRAY*HLEN*sizeof(ub4));
   if (!state.hash[i])
     printf("could not malloc\n");
   for (j = 0; j < ARRAY; ++j)
     state.hash[i][j] = (ub4)0;          /* zero out the hash table */
 }

 /* do the work */
 for (a[0] = 8*BYTES; a[0]--;)
   recurse(0, &state);

 /* free the hash table */
 for (i=0; i<MMM; ++i)
   free(state.hash[i]);
}


int main()
{
  driver();
}
