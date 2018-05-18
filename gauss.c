/*
------------------------------------------------------------------------------
gauss.c: Gaussian Elimination, other bitarray related stuff
Bob Jenkins, 1996, Public Domain
------------------------------------------------------------------------------
*/
#ifndef STANDARD
#include "standard.h"
#endif
#ifndef BIT
#include "bit.h"
#endif
#ifndef GAUSS
#include "gauss.h"
#endif

/* x := y * m;
 *   x:   a bitvec of col values
 *   y:   a bitvec of row values
 *   m:   an array of row bitvecs of col values each
 *   row: number of rows
 *   col: number of columns
 */
void xym( x, y, m, row, col)
bitvec  *x;
bitvec  *y;
bitvec **m;
word     row;
word     col;
{
   word i;
   bitvclr(x,col);
   for (i=0; i<row; ++i)
   {
      if (bitvtst(y,i))
      {
         bitvxor(x,x,m[i],col);
      }
   }
}

/*
--------------------------------------------------------
Gaussian Elimination (of boolean values)
  m:     bitvtst(m[i],j) is the jth term of the ith equation
  count: how many equations are there
  col:   how many terms are there
Return the number of linearly independent equations
--------------------------------------------------------
*/
word gauss( m, row, col)
bitvec **m;
word     row;
word     col;
{
   word i;          /* currently working on the ith equation */
   word j;          /* look at jth equation while working on ith */
   word l;          /* first nonzero term in ith equation is lth */

   /* Apply Gaussian elimination to all the equations */
   for (i=0,l=0; i<row; ++i)
   {
      /* find an equation with a nonzero term */
      while (!bitvtst(m[i],l))
      {
         for (j=i+1; j<row; ++j) if (bitvtst(m[j],l)) break;
         if (j==row)
         {
            if (++l >= col) return i;  /* linearly dependent */
         }
         else 
         {
            bitvec *temp = m[i];
            m[i] = m[j];
            m[j] = temp;
         }
      }
      /* do the elimination */
      for (j=i+1; j<row; ++j)
      {
         if (bitvtst(m[j],l)) bitvxor(m[j],m[j],m[i],col);
      }
   }
   return row;
}
