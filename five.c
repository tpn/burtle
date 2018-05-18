/*
-------------------------------------------------------------------------------
Bob Jenkins, June 2002, Error Correction Codes (public domain)

This finds LENGTH codewords for representing the powers of two
2^^0 .. 2^^(LENGTH-1).  Linear combinations of these will produce a
full set of 2^^LENGTH codewords, all of which have Hamming distance
at least DIST from one another.
-------------------------------------------------------------------------------
*/

#include <standard.h>
#include <stddef.h>

#define DIST   5         /* Hamming distance between codewords */
#define LENGTH 4096       /* Size of the codeword basis to find */

static ub4 last = 0;
static int last_cnt = 1;
static int last_k = DIST-1;

/* print a set of results */
void vshow(int i, ub4 x, int d)
{
  ub4 j;
  for (j=0; (((ub4)1)<<j) < x && j<31; ++j) ;
  printf("(%2d,%2d,%2d)  ", (int)(i+1+j), i+1, d);
  printf("0x%.8lx  ", (ub4)x);
  for (j=((ub4)1)<<31; j; j>>=1) {
    printf("%d", (x & j) ? 1 : 0);
  }
  printf("\n");
}

void vread(ub4 *array, int *start)
{
  char buf[256];
  int  a,b,c;
  ub4  d;

  while (gets((char *)buf))
  {
    sscanf(buf, "(%d,%d,%d) %lx ", &a,&b,&c,&d);
    if (*start != b-1)
      printf("Missed a dimension (%d, %d)\n", *start+1, b);
    if (DIST != c)
      printf("Wrong distance!  Want %d, read %d\n", DIST, c);
    vshow(*start, d, c);
    array[(*start)++] = d;
  }
}


/* Count how many bits are set in a ub4.
 * Filched from sci.crypt.  64 1-bit counters, 32 2-bit counters, etc.
 */
int count(ub4 x)
{
  ub4 c = x;

  c = (c & 0x55555555) + ((c>>1 ) & 0x55555555);
  c = (c & 0x33333333) + ((c>>2 ) & 0x33333333);
  c = (c & 0x0f0f0f0f) + ((c>>4 ) & 0x0f0f0f0f);
  c = (c & 0x00ff00ff) + ((c>>8 ) & 0x00ff00ff);
  c = (c & 0x0000ffff) + ((c>>16) & 0x0000ffff);
  return (int)c;
}

/* How many 1s must be set in the low end of x to make count(x')>dist? */
int suffix(ub4 x, int dist)
{
  int i, j;
  int d = dist - count(x);
  for (i=0, j=0; j<d; ++i)
    if (!((((ub4)1)<<i)&x)) ++j;
  return i;
}

void update_last(ub4 j, ub4 t, int cnt, int dist)
{
  int k = suffix(j^t, dist-cnt);
  /* printf("k %d last_k %d j %.8lx t %.8lx\n", k, last_k, (ub4)j, (ub4)t); */
  if (k >= last_k) {
    last = t;
    last_cnt = cnt;
    last_k = k;
  }
}

/* Test if a[len] has Hamming distance at least dist from t.
 * Also recursively test c with more powers of two XORed to t.
 * Return FALSE if c fails this trial or any recursive trial.
 * Otherwise c passes, return TRUE.
 */
int test(ub4  *a,   /* array of codewords for powers of two */
	 int   len, /* length of array */
	 int   dist,/* required Hamming distance */
	 ub4   t,   /* trial vector to compare codeword to */
	 int   pos, /* next position to choose */
	 int   num, /* number of positions still to be chosen */
         int   cnt, /* final number of chosen positions total */
	 int  *zzz) /* how many things have we tried */
{
  int  i;

  if (--num) {   /* just recurse */
    for (i=pos; i-- > num;) {
      t ^= a[i];
      if (!test(a, len, dist, t, i, num, cnt, zzz))
	return FALSE;
      t ^= a[i];
    }
  } else {     /* just test */
    for (i=pos; i--;) {
      t ^= a[i];
      ++*zzz;
      if (cnt+count(a[len]^t) < dist) {
	update_last(a[len], t, cnt, dist);
	return FALSE;
      }
      t ^= a[i];
    }
  }
  return TRUE;
}

/* find values that all differ by at least limit bits */
void find(ub4 *a, int dist, int length, int start)
{
  int  i, k, q, zzz;
  ub4  j, z;
  ub4  trial;

  i = start;
  j = (i ? a[i-1]+1 : 1);
  for (; i<length; ++i) {
    for (q=0, zzz=0;; ++q) {
      if (1+count(j) < dist) {
	update_last(j, 0, 1, dist);
	goto next;
      }
      ++zzz;
      a[i] = j;
      for (k=1; k<dist-1; ++k)
	if (!test(a, i, dist, (ub4)0, i, k, k+1, &zzz))
	  goto next;
      break;
    next:
      k = suffix(j^last, dist-last_cnt);
      z = (j>>k)<<k;
      z = z | ((~last) & ((((ub4)1)<<k)-1));
      while (z <= j) {
	z = ((z>>k)+1)<<k;
	k = suffix(z^last, dist-last_cnt);
	z = (z>>k)<<k;
	z = z | ((~last) & ((((ub4)1)<<k)-1));
      }
      j = z;
      last_k = k;
    }
    vshow(i, a[i], dist);
    if (0 && i > 0)
      printf("skip %f try %f work %f\n",
	     (float)(a[i]-a[i-1])/(float)q,
	     (float)zzz/(float)q,
	     (float)zzz/(float)(a[i]-a[i-1]));
  }
}

int main()
{
  ub4 array[LENGTH];
  int start;
  time_t a,z;
  
  start = 0;
  array[0] = 0;
  vread(array, &start);
  time(&a);
  find(array, DIST, LENGTH, start);
  time(&z);
  printf("lexicode time %ld\n", (ub4)(z-a));
}
