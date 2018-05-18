/*
 * My implementation of Orz's test.  Usage: "count5x4 20" will run the
 * generator defined by ranval() for 2^^20 values.
 *
 * Given a subsequence of 5 32-bit random numbers, compute the number
 * of bits set in each term, reduce that to 0..15 somehow, and
 * concatenate those 5 4-bit integers into a single 20-bit integer.
 * Do this for len overlapping sequences.  And report the chi-square
 * measure of the results compared to the ideal distribution.
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

typedef struct ranctx { u4 a; u4 b; u4 c; u4 d;} ranctx;

#define rot(x,k) ((x<<k)|(x>>(32-k)))

/* xxx: place the generator you want to test in ranval() */
static u4 ranval( ranctx *x ) {
  u4 e = x->a;
  x->a = rot(x->b,15);
  x->b = x->c + rot(x->d, 27);
  x->c = x->d + x->a;
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

  c = (c & 0x55555555) + ((c>>1 ) & 0x55555555);
  c = (c & 0x33333333) + ((c>>2 ) & 0x33333333);
  c = (c & 0x0f0f0f0f) + ((c>>4 ) & 0x0f0f0f0f);
  c = (c & 0x00ff00ff) + ((c>>8 ) & 0x00ff00ff);
  c = (c & 0x0000ffff) + ((c>>16) & 0x0000ffff);
  return c;
}

/* somehow covert 0..32 into 0..15 */
static u4 f( u4 x)
{
  return (x>>1)&0xf;
}

/* (a,b,c,d,e)->(x,y) where the most common (a,b,c,d,e) have the same x */
static void g( u4 a, u4 b, u4 c, u4 d, u4 e, u4 *z)
{
  u4 w,x,y;
  w = (a+(b<<4)+(c<<8))+(d<<12)+(e<<16);
  x = (w&0xcc)|((w&0xccc00)>>10);  /* the most significant bits */
  y = (w&0x333)|((w&0x33000)>>10); /* the least significant bits */
  *z = y|(x<<10);
}

/* initialize the data collection array */
static void datainit( u8 *data)
{
  u4 a,b,c,d,e,z;
  for (a=0; a<16; ++a)
    for (b=0; b<16; ++b)
      for (c=0; c<16; ++c)
	for (d=0; d<16; ++d)
	  for (e=0; e<16; ++e) {
	    g(a,b,c,d,e,&z);
	    data[z] = 0;
	  }
}

/* gather statistics on len overlapping subsequences of length 5 each */
static void gather( ranctx *r, u8 *data, u8 len)
{
  u8 i;
  u4 z;
  u4 a=f(count(ranval(r)));
  u4 b=f(count(ranval(r)));
  u4 c=f(count(ranval(r)));
  u4 d=f(count(ranval(r)));
  u4 e=f(count(ranval(r)));
  for (i=0; i<len; i+=5) {
    g(a,b,c,d,e,&z); ++data[z];
    a=f(count(ranval(r)));
    g(b,c,d,e,a,&z); ++data[z];
    b=f(count(ranval(r)));
    g(c,d,e,a,b,&z); ++data[z];
    c=f(count(ranval(r)));
    g(d,e,a,b,c,&z); ++data[z];
    d=f(count(ranval(r)));
    g(e,a,b,c,d,&z); ++data[z];
    e=f(count(ranval(r)));
  }
}

/* figure out the probability of 0..15=f(count(u4)) */
static void probinit( double *pc)
{
  u8 i,j,k;
  for (i=0; i<16; ++i) {
    pc[i] = 0.0;
  }
  for (i=0; i<=32; ++i) {
    k = 1;
    for (j=1; j<=i; ++j) {
      k = (k * (33-j)) / j;
    }
    pc[f(i)] += ldexp((double)k,-32);
  }
}

static void chi( u8 *data, u8 len)
{
  u4 a,b,c,d,e,z;           /* a-e are bitcounts, z are converted coords */
  double pc[16];            /* pc[i] is probability of a bitcount of i */
  double expectother = 0.0; /* expected fullness of "other" bucket */
  double var = 0.0;         /* total variance */
  double temp;              /* used to calculate variance of a bucket */
  u8 buckets = 0;           /* number of buckets used */
  u8 countother = 0;
  
  probinit(pc);

  /* handle the nonnegligible buckets */
  for (a=0; a<16; ++a)
    for (b=0; b<16; ++b)
      for (c=0; c<16; ++c)
	for (d=0; d<16; ++d)
	  for (e=0; e<16; ++e) {
	    double expect = ((double)len)*pc[a]*pc[b]*pc[c]*pc[d]*pc[e];
	    g(a,b,c,d,e,&z);
	    if (expect < 5.0) {
	      expectother += expect;
	      countother += data[z];
	    } else {
	      ++buckets;
	      temp = (double)data[z] - expect;
	      var += temp*temp/expect;
	    }
	  }

  /* lump all the others into one bucket */
  temp = (double)countother - expectother;
  printf("otherbucket ideal: %11.4f   got: %lld\n", 
	 (float)expectother, countother);
  var += temp*temp/expectother;

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

  data = (u8 *)malloc(sizeof(u8)*1024*1024);
  if (!data) {
    fprintf(stderr, "could not malloc data\n");
    return 1;
  }

  datainit(data);
  raninit(&r, 0);
  gather(&r, data, len);
  chi(data, len);

  free(data);

  time(&z);
  printf("number of seconds: %6d\n", (size_t)(z-a));
}
