#include "akron.h"
#include <stdio.h>
#include <stddef.h>
#include <windows.h>

typedef  unsigned long  u4;
u4 one_at_a_time(const char *message, size_t length, u4 seed)
{
  int i;
  u4 hash = seed;
  for (i=0; i<length; ++i) {
    hash += message[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 15);
  hash ^= (hash >> 11);
  hash += (hash << 3);
  return hash;
}

void plain(const void *message, size_t length, uint64_t *hash1, uint64_t *hash2)
{
  const uint64_t *data = (const uint64_t *)message;
  const uint64_t *end = data + length/8;
  uint64_t a = *hash1, b = *hash2;
  int i;
  for (; data < end; data += 8) {
    a += data[0];
    b += data[1];
    a += data[2];
    b += data[3];
    a += data[4];
    b += data[5];
    a += data[6];
    b += data[7];
  }
  *hash1 = a;
  *hash2 = b;
}


typedef struct ranctx { uint64_t a; uint64_t b; uint64_t c; uint64_t d;} ranctx;

static uint64_t ranval( ranctx *x ) {
  uint64_t e = x->a - rot64(x->b, 23);
  x->a = x->b ^ rot64(x->c, 16);
  x->b = x->c + rot64(x->d, 11);
  x->c = x->d + e;
  x->d = e + x->a;
  return x->d;
}

void raninit( ranctx *x, uint64_t seed) {
  int i;
  x->a = 0xdeadbeef; x->b = x->c = x->d = seed;
  for (i=0; i<20; ++i) ranval(x);
}


#define NUMBUF 1024
#define BUFSIZE (1<<20)
void DoTiming()
{
  uint64_t a,z, hash1, hash2;
  char *buf[NUMBUF];
  int i, j;
  Akron state;
  a = GetTickCount();
  for (i=0; i<NUMBUF; ++i) {
    size_t skew;
    buf[i] = (char *)malloc(BUFSIZE);
    memset(buf[i], (char)a, BUFSIZE);
  }

  a = GetTickCount();
  hash1 = hash2 = a;
  for (j=0; j<200; ++j) {
    a = GetTickCount();
    for (i=0; i<NUMBUF; ++i) {
      AkronHash(buf[i], BUFSIZE, &hash1, &hash2);
    }
    z = GetTickCount();
    printf("hash is %.16llx %.16llx, time is %lld\n", hash1, hash2, z-a);
  }

  for (i=0; i<NUMBUF; ++i) { 
    free(buf[i]);
    buf[i] = 0;
  }
}


// count how many bits are set in a 64-bit integer, returns 0..64
static uint64_t count8(uint64_t x)
{
  uint64_t c = x;

  c = (c & 0x5555555555555555LL) + ((c>>1 ) & 0x5555555555555555LL);
  c = (c & 0x3333333333333333LL) + ((c>>2 ) & 0x3333333333333333LL);
  c = (c & 0x0f0f0f0f0f0f0f0fLL) + ((c>>4 ) & 0x0f0f0f0f0f0f0f0fLL);
  c = (c & 0x00ff00ff00ff00ffLL) + ((c>>8 ) & 0x00ff00ff00ff00ffLL);
  c = (c & 0x0000ffff0000ffffLL) + ((c>>16) & 0x0000ffff0000ffffLL);
  c = (c & 0x00000000ffffffffLL) + ((c>>32) & 0x00000000ffffffffLL);
  return c;
}

#undef NUMBUF
#undef BUFSIZE
#define BUFSIZE 256
#define TRIES 60
#define MEASURES 6

void TestPair()
{
  int h,i,j,k,l,m, maxk;
  ranctx ctx;
  raninit(&ctx, 1);

  for (h=0; h<BUFSIZE; ++h) {  // for messages 0..255 bytes
    maxk = 0;
    for (i=0; i<h*64; ++i) {   // first bit to set
      for (j=0; j<=i; ++j) {   // second bit to set, or don't have a second bit
	uint64_t measure[MEASURES][2];
	uint64_t counter[MEASURES][2];
	for (l=0; l<2; ++l) {
	  for (m=0; m<MEASURES; ++m) {
	    counter[m][l] = 0;
	  }
	}
	for (k=0; k<TRIES; ++k) { // try to hit every output bit TRIES times
	  uint64_t buf1[BUFSIZE];
	  uint64_t buf2[BUFSIZE];
	  int done = 1;
	  for (l=0; l<h; ++l) {
	    buf1[l] = buf2[l] = ranval(&ctx);
	  }
	  buf1[i/64] ^= (((uint64_t)1) << (i%64));
	  if (j != i) {
	    buf1[j/64] ^= (((uint64_t)1) << (j%64));
	  }
	  AkronHash(buf1, h, &measure[0][0], &measure[0][1]);
	  AkronHash(buf2, h, &measure[1][0], &measure[1][1]);
	  for (l=0; l<2; ++l) {
	    measure[2][l] = measure[0][l] ^ measure[1][l];
	    measure[3][l] = ~(measure[0][l] ^ measure[1][l]);
	    measure[4][l] = measure[0][l] - measure[1][l];
	    measure[4][l] ^= (measure[4][l]>>1);
	    measure[5][l] = measure[0][l] + measure[1][l];
	    measure[5][l] ^= (measure[4][l]>>1);
	  }
	  for (l=0; l<2; ++l) {
	    for (m=0; m<MEASURES; ++m) {
	      counter[m][l] |= measure[m][l];
	      if (~counter[m][l]) done = 0;
	    }
	  }
	  if (done) break;
	}
	if (k == TRIES) {
	  printf("failed %d %d %d\n", h, i, j);
	} else if (k > maxk) {
	  maxk = k;
	}
      }
    }
    printf("done with buffer size %d  max %d\n", h, maxk);
  }
}

int main()
{
  char buf[256];
  int i, j, k;
  for (i=0; i<256; ++i) {
    buf[i] = i;
  }
  for (i=0; i<256; ++i) {
    uint64_t a,b,c,d,e,f;
    Akron state;
    a = c = e = 1;
    b = d = f = 2;

    // all as one call
    AkronHash(buf, i, &a, &b);

    // all as one piece
    AkronInit(&state, c, d);
    AkronUpdate(&state, buf, i);
    AkronFinal(&state, &c, &d);
    
    // a bunch of 1-byte pieces
    AkronInit(&state, e, f);
    for (j=0; j<i; ++j) {
      AkronUpdate(&state, &buf[j], 1);
    }
    AkronFinal(&state, &e, &f);

    if (a != c || a != e) {
      printf("wrong %d: %.16llx %.16llx %.16llx\n", i, a,c,e);
    }
    if (b != d || b != f) {
      printf("wrong %d: %.16llx %.16llx %.16llx\n", i, b,d,f);
    }
  }

  TestPair();
  DoTiming();
}
