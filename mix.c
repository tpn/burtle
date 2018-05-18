/*
------------------------------------------------------------------------------
mix.c: some sort of block cipher.
By Bob Jenkins, 1997.  Public Domain.
------------------------------------------------------------------------------
*/
#ifndef STANDARD
#include "standard.h"
#endif
#ifndef MIX
#include "mix.h"
#endif

/* mix the x */
void mix(x)
ub4 *x;
{
   register ub4 a,b,c,d,e,f,g,h;
   a=x[0]; b=x[1]; c=x[2]; d=x[3];
   a += d; d += a; a ^= (a>>7);
   b += a; a += b; b ^= (b<<13);
   c += b; b += c; c ^= (c>>17);
   d += c; c += d; d ^= (d<<9);
   a += d; d += a; a ^= (a>>3);
   b += a; a += b; b ^= (b<<7);
   c += b; b += c; c ^= (c>>15);
   d += c; c += d; d ^= (d<<11);
   x[0]=a; x[1]=b; x[2]=c; x[3]=d;
}

/* the reverse of mix */
void unmix(x)
ub4 *x;
{
   register ub4 a,b,c,d,e,f,g,h;
   a=x[0]; b=x[1]; c=x[2]; d=x[3];
   d ^= (d<<11); c -= d; d -= c;
   c ^= (c>>15); b -= c; c -= b;
   b ^= (b<<7);  a -= b; b -= a;
   a ^= (a>>3);  d -= a; a -= d;
   d ^= (d<<9);  c -= d; d -= c;
   c ^= (c>>17); b -= c; c -= b;
   b ^= (b<<13); a -= b; b -= a;
   a ^= (a>>7);  d -= a; a -= d;
   x[0]=a; x[1]=b; x[2]=c; x[3]=d;
}

/* given the x at some point in the mix, roll back to the start */
void preimage(x)
ub4 *x;
{
   register ub4 a,b,c,d,e,f,g,h;
   a=x[0]; b=x[1]; c=x[2]; d=x[3];
   a += d; d += a; a ^= (a>>3);
   b += a; a += b; b ^= (b<<7);
   c += b; b += c; c ^= (c>>15);
   d += c; c += d; d ^= (d<<11);
   x[0]=a; x[1]=b; x[2]=c; x[3]=d;
}

/* given the x at some point in the mix, roll forward to the end */
void postimage(x)
ub4 *x;
{
   register ub4 a,b,c,d,e,f,g,h;
   a=x[0]; b=x[1]; c=x[2]; d=x[3];
   d ^= (d<<9);  c -= d; d -= c;
   c ^= (c>>17); b -= c; c -= b;
   b ^= (b<<13); a -= b; b -= a;
   a ^= (a>>7);  d -= a; a -= d;
   x[0]=a; x[1]=b; x[2]=c; x[3]=d;
}
