/* this finds the check bits for the basis of the d=7 binary lexicodes */

#include <stddef.h>

#define DIST   7
#define LENGTH 4096

typedef  unsigned long  int  ub4;

static ub4 last = 0;
static int last_cnt = 1;
static int last_k = DIST-1;

/* show (n,k,d) and check bits for a basis member */
void show(int i, ub4 x, int d)
{
  ub4 j;
  for (j=0; (((ub4)1)<<j) < x && j<31; ++j) ;
  printf("(%4d,%4d,%2d)  ", (int)(i+1+j), i+1, d);
  printf("0x%.8lx  ", (ub4)x);
  for (j=((ub4)1)<<31; j; j>>=1) {
    printf("%d", (x & j) ? 1 : 0);
  }
  printf("\n");
}

/* count how many bits are set */
int count(ub4 x)
{
  x = (x & 0x55555555) + ((x>>1 ) & 0x55555555);
  x = (x & 0x33333333) + ((x>>2 ) & 0x33333333);
  x = (x & 0x0f0f0f0f) + ((x>>4 ) & 0x0f0f0f0f);
  x = (x & 0x00ff00ff) + ((x>>8 ) & 0x00ff00ff);
  x = (x & 0x0000ffff) + ((x>>16) & 0x0000ffff);
  return (int)x;
}

/* see how big a suffix we can skip */
int suffix(ub4 x, int dist)
{
  int i, j;
  int d = dist - count(x);
  for (i=0, j=0; j<d; ++i)
    if (!((((ub4)1)<<i) & x)) ++j;
  return i;
}

/* update last */
void update_last(ub4 j, ub4 t, int cnt, int dist)
{
  int k = suffix(j^t, dist-cnt);
  if (k >= last_k) {
    last = t;
    last_cnt = cnt;
    last_k = k;
  }
}

/* test a[len] against all XORs of cnt existing basis members */
int test(ub4  *a,
	 int   len,
	 int   dist,
	 ub4   t,
	 int   pos,
	 int   num,
         int   cnt)
{
  int  i;

  if (--num) {
    for (i=pos; i-- > num;) {
      t ^= a[i];
      if (!test(a, len, dist, t, i, num, cnt))
	return 0;
      t ^= a[i];
    }
  } else {
    for (i=pos; i--;) {
      t ^= a[i];
      if (cnt+count(a[len]^t) < dist) {
	update_last(a[len], t, cnt, dist);
	return 0;
      }
      t ^= a[i];
    }
  }
  return 1;
}

/* construct the basis */
void construct(ub4 *a, int dist, int length)
{
  int  i=0, k, q;
  ub4  j=1, z;

  for (; i<length; ++i) {
    for (;;) {
      if (1+count(j) < dist) {
	update_last(j, 0, 1, dist);
	goto next;
      }
      a[i] = j;
      for (k=1; k<dist-1; ++k)
	if (!test(a, i, dist, (ub4)0, i, k, k+1))
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
    show(i, a[i], dist);
  }
}

/* start at main */
int main()
{
  ub4 array[LENGTH];
  
  array[0] = 0;
  construct(array, DIST, LENGTH);
}
