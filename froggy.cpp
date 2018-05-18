/*
----------------------------------------------------------------------
Ideally, frog consumes all the RAM and all the cores on your machine.
Adjust that by adjusting THREAD (for cores) and LMMM (for memory).

All the cores bang out hash values and store them in a hash table the
size of RAM.  When there's a collision in a bucket, we first check if
the whole hash matched (and print out the hash value), then overwrite
that bucket with the new hash value.  False collisions are rare enough
to ignore, so there's no concurrency control between threads, and all 
the hash values start out as 0.  Each thread handles every 1/THREADth
possible input.
----------------------------------------------------------------------
*/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "spooky.h"

#define BITS   5                 /* maximum number of bits set in the string */
#define BYTES  400                         /* length of the string, in bytes */
#define LARRAY 18                          /* 1<<LARRAY HLEN*ub4s per malloc */
#define LMMM   10                                   /* 1<<LMMM mallocs total */
#define HLEN   6                         /* length of a hash value, in bytes */
#define ARRAY (((ub4)1)<<LARRAY)              /* length of a malloc, in ub4s */
#define MMM   (((ub4)1)<<LMMM)           /* number of segments in hash table */
#define THREADS 8

typedef  unsigned char      ub1;
typedef  unsigned short     ub2;
typedef  unsigned long      ub4;
typedef  unsigned long long ub8;

/*
 * A threadsafe printf statement
 */
static CRITICAL_SECTION tprint_crit;
void tprint(const char *fmt, ...)
{
  va_list args;
  EnterCriticalSection(&tprint_crit);
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  LeaveCriticalSection(&tprint_crit);
}

struct mystate
{
  uint64    count;                            /* total number of strings tested */
  ub4    a[BITS];           /* a[i] says that q[a[i]] should be the ith word */
  ub1  **hash;                                       /* last such hash value */
  ub1   *string;                                      /* string to be hashed */
  ub1    x[HLEN];                                   /* results from the hash */
  ub4    mask;                          /* what section of values to look at */
  ub4    id;                                         /* which thread is this */
};
typedef  struct mystate  mystate;

void test(mystate *state)
{
 ub4  val;
 ub4  zip;
 ub4  off;
 ub1 *where;
 ub1 *x = state->x;
 uint64 hashval1=0, hashval2=0;
 int  i;

 /* report how much progress we have made */
 if (!(state->count & (state->count-1)))
 {
   for (i=0; (((uint64)1)<<i) < state->count; ++i)
     ;
   tprint("count 2^^%d, covered 2^^%d key pairs (thread %d)\n", 
	  i, 
	  i <= LARRAY+LMMM ? 2*i-1 : (LARRAY+LMMM) + i,
	  state->id);
 }

 /* compute a new hash value */
 // hashval = CityHash128((const char *)state->string, BYTES);
 // val = (ub4)Uint128Low64(hashval);
 // for (i=0; i<8; ++i) x[i] = (ub1)(Uint128Low64(hashval)>>(i*8));
 // for (i=8; i<HLEN; ++i) x[i] = (ub1)(Uint128High64(hashval)>>(i*8));

 // ub8 h0 = ((ub8 *)state->string)[0];
 // ub8 h1 = ((ub8 *)state->string)[1];
 // ub8 h2 = ((ub8 *)state->string)[2];
 // ub8 h3 = ((ub8 *)state->string)[3];
 // ub8 h4 = ((ub8 *)state->string)[4];
 // ub8 h5 = ((ub8 *)state->string)[5];
 // ub8 h6 = ((ub8 *)state->string)[6];
 // ub8 h7 = ((ub8 *)state->string)[7];
 // ub8 h8 = ((ub8 *)state->string)[8];
 // ub8 h9 = ((ub8 *)state->string)[9];
 // ub8 h10 = ((ub8 *)state->string)[10];
 // ub8 h11 = ((ub8 *)state->string)[11];

 // SpookyHash::End(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
 // val = (ub4)h1;
 // for (i=0; i<HLEN; ++i) x[i] = (ub1)(h0>>((7-i)*8));

 SpookyHash::Hash128((const char *)state->string, BYTES, &hashval1, &hashval2);
 val = (ub4)hashval1;
 for (i=0; i<HLEN; ++i) x[i] = (ub1)(hashval2>>((7-i)*8));

 /* look up where it belongs in the hash table */
 zip = ((val>>LMMM)&(ARRAY-1))*HLEN;
 off = (val)&(MMM-1);
 where = &state->hash[off][zip];

 /* did we actually get a collision? */
 for (i=0; i<HLEN; ++i) if (where[i] != x[i]) break;
 if (i == HLEN) {
   int i;
   tprint("collision!  hash value ");
   for (i=0; i<HLEN; ++i) printf("%.2lx ", x[i]);
   tprint("count %.8lx %.8lx\n", 
	  (ub4)(state->count>>32), (ub4)(state->count & 0xffffffff));
 }

 /* store the current hash value */
 for (i=0; i<HLEN; ++i) where[i] = x[i];
}

void recurse(int depth, mystate *state)
{
 /* set my bit */
 state->string[state->a[depth]>>3] ^= (1<<(state->a[depth]&0x7));

 if ((++state->count % THREADS) == 0) {
   test(state);
 }

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

typedef struct threadargs
{
  ub1 **hash;
  int   id;
} threadargs;

/* driver for a thread */
DWORD WINAPI tdriver(LPVOID arg)
{
  threadargs *targs = (threadargs *)arg;
  int     i;
  uint64 s[(BYTES+7)/8];
  mystate state;

  /* initialize the state */
  memset(s, 0, sizeof(s));
  state.string = (ub1 *)s;
  state.count  = (uint64)targs->id;
  state.hash = targs->hash;
  state.id = targs->id;
  
  /* do the work */
  for (state.a[0] = 8*BYTES; state.a[0]--;) {
    recurse(0, &state);
  }
  return (DWORD)0;
}


void driver()
{
  threadargs t[THREADS];
  HANDLE     tid[THREADS];
  ub1   **hash;
  ub4     i, j;

  /* allocate memory for the hash table */
  hash = (ub1 **)malloc(MMM*sizeof(ub1 *));
  for (i=0; i<MMM; ++i) {
    hash[i] = (ub1 *)malloc(ARRAY*HLEN*sizeof(ub1));
    if (!hash[i])
      tprint("could not malloc\n");
    for (j = 0; j < ARRAY; ++j)
      hash[i][j] = (ub4)0;          /* zero out the hash table */
  }

  /* do the work */
  for (i=0; i<THREADS; ++i) {
    int ret;
    t[i].hash = hash;
    t[i].id = i;
    tid[i] = CreateThread(NULL, 0, tdriver, &t[i], 0, 0);
  }
  WaitForMultipleObjects(THREADS, tid, TRUE, INFINITE);
  
  /* free the hash table */
  tprint("all done\n");
  for (i=0; i<MMM; ++i)
    free(hash[i]);
  free(hash);
}


int main()
{
  InitializeCriticalSection(&tprint_crit);
  driver();
}
