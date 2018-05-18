/*
--------------------------------------------------------
Bit-array related stuff.  
Bob Jenkins, 1996.  Public domain.
--------------------------------------------------------
*/

#ifndef STANDARD
#include "standard.h"
#endif
#ifndef BIT
#include "bit.h"
#endif

/* set mybit in x */
void bitv1(x, mybit)
bitvec *x;
word    mybit;
{
   x[mybit>>5] |= ((bitvec)1)<<(mybit & 31);
}

/* clear mybit in x */
void bitv0(x, mybit)
bitvec *x;
word    mybit;
{
   x[mybit>>5] &= ~(((bitvec)1)<<(mybit & 31));
}

/* Return TRUE if mybit is set in x, FALSE otherwise */
word bitvtst(x, mybit)
bitvec *x;
word    mybit;
{
   return ((x[mybit>>5] & (((bitvec)1)<<(mybit & 31))) ? TRUE : FALSE);
}

/* clear x */
void bitvclr(x, len)
bitvec *x;
word    len;
{
   word i;
   for (i=0; i<MAKEBIT(len); ++i) x[i] = 0;
}

/* x := y */
void bitvcpy(x, y, len)
bitvec *x;
bitvec *y;
word    len;
{
   word i;
   for (i=0; i<MAKEBIT(len); ++i) x[i] = y[i];
}

/* (x!=y) */
word bitvcmp(x, y, len)
bitvec *x;
bitvec *y;
word    len;
{
   word i;
   for (i=0; i<MAKEBIT(len); ++i) if (x[i] != y[i]) return TRUE;
   return FALSE;
}

/* x = y ^ z */
void bitvxor(x, y, z, len)
bitvec *x;
bitvec *y;
bitvec *z;
word    len;
{
   word i;
   for (i=0; i<MAKEBIT(len); ++i) x[i] = y[i] ^ z[i];
}

/* print a bit vector */
void bitprint(x, len)
bitvec *x;
word    len;
{
   word i;
   for (i=0; i<MAKEBIT(len); ++i) printf("%.8lx ",x[i]);
   printf("\n");
}
