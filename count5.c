/*
 * Given a subsequence of 5 32-bit random numbers, compute the number
 * of bits set in each term, reduce that to BUCKETS values using ftab[],
 * and concatenate those results into a single integer.  Do this for 
 * 1<<arg1 overlapping sequences.  And report the chi-square measure 
 * of the results compared to the ideal distribution.
 *
 * This is my implementation of Orz (Bob Blorp2)'s tests.  Both his test
 * and my implementation of it are public domain.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <time.h>

typedef  unsigned char      u1;
typedef  unsigned long      u4;
typedef  unsigned long long u8;

#define LOGBUCKETS 3
#define BUCKETS (1<<LOGBUCKETS)
#define ARRAYSIZE (BUCKETS*BUCKETS*BUCKETS*BUCKETS*BUCKETS)
#define GRAY_CODE 1   /* 0 normal values, 1 gray coded values */

typedef struct ranctx { u4 a; u4 b; u4 c; u4 d;} ranctx;

#define rot(x,k) ((x<<k)|(x>>(32-k)))

static u4 iii = 0;

static u4 ranval( ranctx *x ) {
  u4 e;
  e = x->a;
  x->a = x->b;
  x->b = rot(x->c, 19) + x->d;
  x->c = x->d ^ x->a;
  x->d = e + x->b;
  return x->c; 
}

static void raninit( ranctx *x, u4 seed ) {
  u4 i;
  x->a = 0xf1ea5eed; 
  x->b = x->c = x->d = seed;
  for (i=0; i<20; ++i) {
    (void)ranval(x);
  }
}

/* count how many bits are set in a 32-bit integer, returns 0..32 */
static u4 count(u4 x)
{
  u4 c = x;

  if (GRAY_CODE) {
    c = c^(c<<1);         /* convert to gray code before counting bits? */
  }

  c = (c & 0x55555555) + ((c>>1 ) & 0x55555555);
  c = (c & 0x33333333) + ((c>>2 ) & 0x33333333);
  c = (c & 0x0f0f0f0f) + ((c>>4 ) & 0x0f0f0f0f);
  c = (c & 0x00ff00ff) + ((c>>8 ) & 0x00ff00ff);
  c = (c & 0x0000ffff) + ((c>>16) & 0x0000ffff);
  return c;
}

/* somehow covert 0..32 into 0..BUCKETS-1 */
static u4 ftab[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1,
  1, 
  1, 1, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2
};

/* (a,b,c,d,e)->(x,y) where the most common (a,b,c,d,e) have the same x */
static void g( u4 a, u4 b, u4 c, u4 d, u4 e, u4 *z)
{
  u4 w,x,y;
  w = (a+(b<<LOGBUCKETS)+(c<<(2*LOGBUCKETS))+
       (d<<(3*LOGBUCKETS))+(e<<(4*LOGBUCKETS)));
#if (LOGBUCKETS == 4)
  x = (w&0xcc)|((w&0xccc00)>>10);  /* the most significant bits */
  y = (w&0x333)|((w&0x33000)>>10); /* the least significant bits */
  *z = (x<<10)|y;  /* this code only works when LOGBUCKETS == 4 */
#else
  *z = w;
#endif
}

/* initialize the data collection array */
static void datainit( u8 *data)
{
  u4 a,b,c,d,e,z;
  for (a=0; a<BUCKETS; ++a)
    for (b=0; b<BUCKETS; ++b)
      for (c=0; c<BUCKETS; ++c)
	for (d=0; d<BUCKETS; ++d)
	  for (e=0; e<BUCKETS; ++e) {
	    g(a,b,c,d,e,&z);
	    data[z] = 0;
	  }
}

/* gather statistics on len overlapping subsequences of length 5 each */
#define DIST (1<<5)
static void gather( ranctx *x, u8 *data, u8 len)
{
  u8 i;
  u4 z;
  u4 r[DIST];    /* last DIST results */
  u4 m = DIST-1; /* mask */
  for (i=0; i<DIST; ++i)
    r[i] = ftab[count(ranval(x))];
  for (i=0; i<len; ++i) {
    r[i&m] = ftab[count(ranval(x))];
    g(r[i&m],r[(i-1)&m],r[(i-2)&m],r[(i-3)&m],r[(i-4)&m],&z);
    ++data[z];
  }
}

/* figure out probabilities, assuming a count of an 8-bit integer */
static void probinit( double *pc, u4 bits)
{
  u8 i,j,k;
  for (i=0; i<BUCKETS; ++i) {
    pc[i] = 0.0;
  }
  for (i=0; i<=bits; ++i) {
    k = 1;
    for (j=1; j<=i; ++j) {
      k = (k * (bits+1-j)) / j;
    }
    pc[ftab[i]] += ldexp((double)k,-32);
  }
}

static void chi( u8 *data, u8 len)
{
  u4 a,b,c,d,e,z;           /* a-e are bitcounts, z are converted coords */
  double pc[BUCKETS];       /* pc[i] is probability of a bitcount of i */
  double expectother = 0.0; /* expected fullness of "other" bucket */
  double var = 0.0;         /* total variance */
  double temp;              /* used to calculate variance of a bucket */
  u8 buckets = 0;           /* number of buckets used */
  u8 countother = 0;
  
  probinit(pc, 32);

  /* handle the nonnegligible buckets */
  for (a=0; a<BUCKETS; ++a)
    for (b=0; b<BUCKETS; ++b)
      for (c=0; c<BUCKETS; ++c)
	for (d=0; d<BUCKETS; ++d)
	  for (e=0; e<BUCKETS; ++e) {
	    double expect = ((double)len)*pc[a]*pc[b]*pc[c]*pc[d]*pc[e];
	    g(a,b,c,d,e,&z);
	    if (expect < 5.0) {
	      expectother += expect;
	      countother += data[z];
	    } else {
	      ++buckets;
	      temp = (double)data[z] - expect;
	      temp = temp*temp/expect;
	      if (temp > 20.0) {
		printf("(%2d %2d %2d %2d %2d) %14.4f %14.4f %14.4f\n",
		       a,b,c,d,e,(float)temp,(float)expect,(float)data[z]);
	      }
	      var += temp;
	    }
	  }

  /* lump all the others into one bucket */
  if (expectother > 5.0) {
    temp = (double)countother - expectother;
    printf("otherbucket ideal: %11.4f   got: %lld\n", 
	   (float)expectother, countother);
    var += temp*temp/expectother;
    ++buckets;
  }
  --buckets;

  /* calculate the total variance and chi-square measure */
  printf("expected variance: %11.4f   got: %11.4f   chi-square: %6.4f\n",
         (float)buckets, (float)var, 
	 (float)((var-buckets)/sqrt((float)buckets)));
}

int main( int argc, char **argv)
{
  u8 i, len;
  u8 *data;
  u4 loglen = 0;
  ranctx r;
  time_t a,z;
  
  time(&a);
  if (argc == 2) {
    sscanf(argv[1], "%d", &loglen);
    printf("log_2 sequence length: %d\n", loglen);
    len = (((u8)1)<<loglen);
  } else {
    fprintf(stderr, "usage: \"cou 24\" means run for 2^^24 values\n");
    return 1;
  }

  data = (u8 *)malloc(sizeof(u8)*ARRAYSIZE);
  if (!data) {
    fprintf(stderr, "could not malloc data\n");
    return 1;
  }

  for (i=0; i<=32; ++i) {
    if (ftab[i] > BUCKETS) {
      fprintf(stderr, "ftab[] needs you to increase LOGBUCKETS: %d < (1<<%d)\n",
	      ftab[i], LOGBUCKETS);
      return 1;
    }
  }
  
  datainit(data);
  raninit(&r, 0);
  gather(&r, data, len);
  chi(data, len);

  free(data);

  time(&z);
  printf("number of seconds: %6d\n", (size_t)(z-a));
}
