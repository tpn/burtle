/*
------------------------------------------------------------------------------
Bob Jenkins, 1996, Public Domain.  Code to break IA.
------------------------------------------------------------------------------
*/
#include <stdio.h>
   
typedef  unsigned long int  ub4;
#define MAXSIZE (1<<8)
#define ALPHA    3
#define SIZE    (1<<ALPHA)
#define ind(x)  ((x)&(SIZE-1))

static void rng(m,r,bb)
ub4  *m;
ub4  *r;
ub4  *bb;
{
   register ub4  b,i,x,y;
   b = *bb;
   for (i=0; i<SIZE; ++i)
   {
      x = m[i];
      m[i] = y = b + m[ind(x)];
      r[i] = b = x + m[ind(y>>ALPHA)];
   }
   *bb = b;
}

static void guess(m,r,bb)
ub4  *m;
ub4  *r;
ub4  *bb;
{
   register ub4  b,i,x,y;
   b = *bb;
   for (i=0; i<SIZE; ++i)
   {
      x = m[i];
      m[i] = y = b + m[ind(x)];
      b = r[i];
   }
   *bb = b;
}

static void driver(int x)
{
   ub4 mr[MAXSIZE], mg[MAXSIZE], r[MAXSIZE], br, bg;
   ub4 i,j;
   for (i=0; i<SIZE; ++i) mr[i] = i+x;
   for (i=0; i<SIZE; ++i) mg[i] = i;
   br = 1;   bg = 7;
   for (i=1;;i++)
   {
      rng(mr,r,&br);
      guess(mg,r,&bg);
      for (j=0; j<SIZE; ++j) if (ind(mr[j]) != ind(mg[j])) break;
      if ((j == SIZE) && (ind(br)==ind(bg))) break; 
      if (!(i&0xfffff)) printf("still working .. i = %ld\n",i);
   }
   printf("It took %ld passes for the guess to converge\n",i);
}

int main()
{
   int x;
   printf("This fills a random number generator with one seed,\n");
   printf("and places another seed in guesser.  The guesser\n");
   printf("uses the random number generator's results to correct\n");
   printf("its guesses.  Eventually the two seeds converge, and\n");
   printf("the guessed results will be correct.\n\n");
   printf("The seed has 1+%d 32-bit values.\n",SIZE);
   printf("Every %ld passes, this will print 'still working'\n",
          ((ub4)1)<<20);
   for (x=0; x<SIZE; ++x)
   {
      driver(x);
   }
}
