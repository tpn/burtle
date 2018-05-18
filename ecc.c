/*
-------------------------------------------------------------------------------
Bob Jenkins, June 2002, Error Correction Codes (public domain)

This finds LENGTH codewords for representing the powers of two
2^^0 .. 2^^(LENGTH-1).  Linear combinations of these will produce a
full set of 2^^LENGTH codewords, all of which have Hamming distance
at least DIST from one another.
-------------------------------------------------------------------------------
*/

#include "standard.h"

#define DIST   23         /* Hamming distance between codewords */
#define LENGTH 1024       /* Size of the codeword basis to find */

static ub8 last = 0;
static ub8 last_cnt = 1;

/* print a set of results */
void vshow(int i, ub8 x, int d)
{
  ub8 j;
  for (j=0; (((ub8)1)<<j) < x && j<63; ++j) ;
  printf("(%2d,%2d,%2d)  ", (int)(i+1+j), i+1, d);
  printf("0x%.8lx%.8lx  ", (ub4)(x>>32), (ub4)x);
  for (j=((ub8)1)<<63; j; j>>=1) {
    printf("%d", (x & j) ? 1 : 0);
  }
  printf("\n");
}

/* Count how many bits are set in a ub8.
 * Filched from sci.crypt.  64 1-bit counters, 32 2-bit counters, etc.
 */
int count(ub8 x)
{
  ub4 c;

  x = (x & 0x5555555555555555LL) + ((x>>1) & 0x5555555555555555LL);
  x = (x & 0x3333333333333333LL) + ((x>>2) & 0x3333333333333333LL);
  c = ((ub4)x) + ((ub4)(x>>32));
  c = (c & 0x0f0f0f0f) + ((c>>4 ) & 0x0f0f0f0f);
  c = (c & 0x00ff00ff) + ((c>>8 ) & 0x00ff00ff);
  c = (c & 0x0000ffff) + ((c>>16) & 0x0000ffff);
  return (int)c;
}

/* Test if a[len] has Hamming distance at least dist from t.
 * Also recursively test c with more powers of two XORed to t.
 * Return FALSE if c fails this trial or any recursive trial.
 * Otherwise c passes, return TRUE.
 */
int test(ub8  *a,   /* array of codewords for powers of two */
	 int   len, /* length of array */
	 int   dist,/* required Hamming distance */
	 ub8   t,   /* trial vector to compare codeword to */
	 int   pos, /* next position to choose */
	 int   num, /* number of positions still to be chosen */
         int   cnt) /* final number of chosen positions total */
{
  int  i;

  if (num) {   /* just recurse */
    for (i=pos; i<len; ++i) {
      t ^= a[i];
      if (!test(a, len, dist, t, i+1, num-1, cnt))
	return FALSE;
      t ^= a[i];
    }
  } else {     /* just test */
    for (i=pos; i<len; ++i) {
      t ^= a[i];
      if (1+cnt+count(a[len]^t) < dist) {
	last = t;
	last_cnt = 1+cnt;
	return FALSE;
      }
      t ^= a[i];
    }
  }
  return TRUE;
}

/* find values that all differ by at least limit bits */
void find(ub8 *a, int dist, int length)
{
  int  i, k;
  ub8  j;
  ub8  trial;

  for (j=0, i=0; i<length; ++i) {
    for (;;) {
      if (last_cnt+count(j^last) < dist)
	goto next;
      if (1+count(j) < dist) {
	last = (ub8)0;
	last_cnt = 1;
	goto next;
      }
      a[i] = j;
      for (k=0; k<dist-2; ++k)
	if (!test(a, i, dist, (ub8)0, 0, k, k+1))
	  goto next;  /* failure */
      break;
    next:
      ++j;
    }
    vshow(i, a[i], dist);
  }
}

int main()
{
  ub8 a[LENGTH];
  find(a, DIST, LENGTH);
}
