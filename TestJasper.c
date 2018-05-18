// Test for Jasper, a 128-bit noncryptographic hash
// By Bob Jenkins, September 2010, public domain.  Alpha-quality, beware.

#include "jasper.h"
#include <stdio.h>
#include <windows.h>

#define NUMBUF 1024
#define BUFSIZE (1<<20)
void DoTiming()
{
  uint64_t a,z, hash1, hash2;
  char *buf[NUMBUF];
  int i, j;
  Jasper state;
  a = GetTickCount();
  for (i=0; i<NUMBUF; ++i) {
    buf[i] = (char *)malloc(BUFSIZE);
    memset(buf[i], (char)a, BUFSIZE);
  }

  for (j=0; j<200; ++j) {
    a = GetTickCount();
    JasperInit(&state, a, a);
    for (i=0; i<(1<<0)*NUMBUF; ++i) {
      JasperUpdate(&state, buf[i], BUFSIZE);
    }
    JasperFinal(&state, &hash1, &hash2);
    z = GetTickCount();
    printf("hash is %.16llx %.16llx, time is %lld\n", hash1, hash2, z-a);
  }

  for (i=0; i<NUMBUF; ++i) { 
    free(buf[i]);
    buf[i] = 0;
  }
}


int main()
{
  char buf[256];
  int i, j;
  for (i=0; i<256; ++i) {
    buf[i] = i;
  }
  for (i=96; i<256; ++i) {
    uint64_t a,b,c,d,e,f;
    Jasper state;
    a = c = e = 1;
    b = d = f = 2;

    // all as one call
    JasperHash(buf, i, &a, &b);

    // all as one piece
    JasperInit(&state, c, d);
    JasperUpdate(&state, buf, i);
    JasperFinal(&state, &c, &d);
    
    // a bunch of 1-byte pieces
    JasperInit(&state, e, f);
    for (j=0; j<i; ++j) {
      JasperUpdate(&state, &buf[j], 1);
    }
    JasperFinal(&state, &e, &f);

    if (a != c || a != e) {
      printf("wrong %d: %.16llx %.16llx %.16llx\n", i, a,c,e);
    }
    if (b != d || b != f) {
      printf("wrong %d: %.16llx %.16llx %.16llx\n", i, b,d,f);
    }
  }

  DoTiming();
}
