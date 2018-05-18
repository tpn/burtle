/*
------------------------------------------------------------------------------
By Bob Jenkins, 1996.  Public Domain.

This is a pre-pre-beta preview of a 256-bit block cipher using 32-bit
arithmetic.  Guaranteed to be stronger than rot-13.  Feel free to try to
break it.

Here's what I know:
* each bit affects at least 140 bits after 2 mixes or 2 unmixes
* avalanche after 3 mixes or 3 unmixes, probability 1/2 +- d, d = 1/4
* no funnels after 4 mixes or 4 unmixes
* DES does avalanche 3x, this does avalanche 4x
* This key schedule would be secure if 12*mix had no flaws
* A 256-bit key is enough to thwart brute-force search forever, and
  this key schedule makes the 512-bit key effectively 256 bits.

Some good shift patterns are
  7   9   4   4  10   6  11   7
 15  13   5   4  10   3   7   4
  7  11  10   4   5   3  15   6
 11   3   6   5  13   8   8   9
 13   4   6   5  10   8   9   6
  6  10  13   6   8   5  11   3
 13   8   8   8  11   5   6   3
 11   4   6   7  13   6   8   7
 10   3   5   6  15   6   7  12
 12   7   9   9  14   4   6   4
  8   8  11   3   6   4  13  13

------------------------------------------------------------------------------
*/

typedef  unsigned long int  ub4;

#define mix32(a,b,c,d,e,f,g,h) \
{ \
   a-=e; f^=h>>8;  h+=a; \
   b-=f; g^=a<<8;  a+=b; \
   c-=g; h^=b>>11; b+=c; \
   d-=h; a^=c<<3;  c+=d; \
   e-=a; b^=d>>6;  d+=e; \
   f-=b; c^=e<<4;  e+=f; \
   g-=c; d^=f>>13; f+=g; \
   h-=d; e^=g<<13; g+=h; \
}

#define unmix32(a,b,c,d,e,f,g,h) \
{ \
   g-=h; e^=g<<13; h+=d; \
   f-=g; d^=f>>13; g+=c; \
   e-=f; c^=e<<4;  f+=b; \
   d-=e; b^=d>>6;  e+=a; \
   c-=d; a^=c<<3;  d+=h; \
   b-=c; h^=b>>11; c+=g; \
   a-=b; g^=a<<8;  b+=f; \
   h-=a; f^=h>>8;  a+=e; \
}

void enc32(block, k1, k2)
ub4 *block;
ub4 *k1;
ub4 *k2;
{
  register ub4 a,b,c,d,e,f,g,h;
  a=block[0]^k1[0]; b=block[1]^k1[1]; c=block[2]^k1[2]; d=block[3]^k1[3];
  e=block[4]^k1[4]; f=block[5]^k1[5]; g=block[6]^k1[6]; h=block[7]^k1[7];
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  mix32(a,b,c,d,e,f,g,h);
  block[0]=a^k2[0]; block[1]=b^k2[1]; block[2]=c^k2[2]; block[3]=d^k2[3];
  block[4]=e^k2[4]; block[5]=f^k2[5]; block[6]=g^k2[6]; block[7]=h^k2[7];
}

void dec32(block, k1, k2)
ub4 *block;
ub4 *k1;
ub4 *k2;
{
  register ub4 a,b,c,d,e,f,g,h;
  a=block[0]^k2[0]; b=block[1]^k2[1]; c=block[2]^k2[2]; d=block[3]^k2[3];
  e=block[4]^k2[4]; f=block[5]^k2[5]; g=block[6]^k2[6]; h=block[7]^k2[7];
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  unmix32(a,b,c,d,e,f,g,h);
  block[0]=a^k1[0]; block[1]=b^k1[1]; block[2]=c^k1[2]; block[3]=d^k1[3];
  block[4]=e^k1[4]; block[5]=f^k1[5]; block[6]=g^k1[6]; block[7]=h^k1[7];
}

/*
------------------------------------------------------------------------------
Pre-pre-beta preview of a 512-bit block cipher, 64-bit arithmetic.
6.5 instructions per byte.  Use this only if you are a fool.
Feel free to try to break this.  This is the same structure as the
32-bit pre-pre-beta block cipher.

Here's what I know:
* Ideal parallelism of 8 instructions per 3 cycles, both mix and unmix
* each bit affects at least 210 bits after 2 mixes or 2 unmixes
* each bit affects at least 510 bits after 3 mixes or 3 unmixes
* avalanche after 4 mixes or 4 unmixes, probability 1/2 +- d, d < 1/12
* no funnels after 5 mixes or 5 unmixes, maybe 4 and 4, not sure
* DES does avalanche 3x, this does avalanche 3x
* This key schedule would be secure if 12*mix had no flaws
* A 512-bit key is more than enough to thwart brute-force search
  forever, and this 1024-bit key schedule is effectively 512 bits.
------------------------------------------------------------------------------
*/

typedef  unsigned long long  ub8;
#define mix64(a,b,c,d,e,f,g,h) \
{ \
   a-=e; f^=h>>9;  h+=a; \
   b-=f; g^=a<<9;  a+=b; \
   c-=g; h^=b>>23; b+=c; \
   d-=h; a^=c<<15; c+=d; \
   e-=a; b^=d>>14; d+=e; \
   f-=b; c^=e<<20; e+=f; \
   g-=c; d^=f>>17; f+=g; \
   h-=d; e^=g<<14; g+=h; \
}

#define unmix64(a,b,c,d,e,f,g,h) \
{ \
   g-=h; e^=g<<14; h+=d; \
   f-=g; d^=f>>17; g+=c; \
   e-=f; c^=e<<20; f+=b; \
   d-=e; b^=d>>14; e+=a; \
   c-=d; a^=c<<15; d+=h; \
   b-=c; h^=b>>23; c+=g; \
   a-=b; g^=a<<9;  b+=f; \
   h-=a; f^=h>>9;  a+=e; \
}

void enc64(block, k1, k2)
ub8 *block;
ub8 *k1;
ub8 *k2;
{
  register ub8 a,b,c,d,e,f,g,h;
  a=block[0]^k1[0]; b=block[1]^k1[1]; c=block[2]^k1[2]; d=block[3]^k1[3];
  e=block[4]^k1[4]; f=block[5]^k1[5]; g=block[6]^k1[6]; h=block[7]^k1[7];
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  mix64(a,b,c,d,e,f,g,h);
  block[0]=a^k2[0]; block[1]=b^k2[1]; block[2]=c^k2[2]; block[3]=d^k2[3];
  block[4]=e^k2[4]; block[5]=f^k2[5]; block[6]=g^k2[6]; block[7]=h^k2[7];
}

void dec64(block, k1, k2)
ub8 *block;
ub8 *k1;
ub8 *k2;
{
  register ub8 a,b,c,d,e,f,g,h;
  a=block[0]^k2[0]; b=block[1]^k2[1]; c=block[2]^k2[2]; d=block[3]^k2[3];
  e=block[4]^k2[4]; f=block[5]^k2[5]; g=block[6]^k2[6]; h=block[7]^k2[7];
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  unmix64(a,b,c,d,e,f,g,h);
  block[0]=a^k1[0]; block[1]=b^k1[1]; block[2]=c^k1[2]; block[3]=d^k1[3];
  block[4]=e^k1[4]; block[5]=f^k1[5]; block[6]=g^k1[6]; block[7]=h^k1[7];
}
