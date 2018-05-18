/*
------------------------------------------------------------------------------
Bob Jenkins, 1996, Public Domain
char.c: figure out the characteristics of a mixing function
------------------------------------------------------------------------------
*/
#ifndef STANDARD
#include "standard.h"
#endif
#ifndef BIT
#include "bit.h"
#endif
#ifndef RECYCLE
#include "recycle.h"
#endif
#ifndef GAUSS
#include "gauss.h"
#endif
#ifndef MIX
#include "mix.h"
#endif
#ifndef RAND
#include "rand.h"
#endif

#define MAXBASE  8
#define TESTSIZE 256
#define SEEDS    4


/* Set up equations.
 * pre[i] is the preimage delta when seeds differ in bit i alone.
 * post[i] is the postimage delta when seeds differ in bit i.
 * eqn[i] is twice as long as pre[i] and post[i].  The first half
 *   is the xor of pre[i] and post[i].  The second half is a copy
 *   of pre[i].
 * We'll be doing Gaussian elimination on eqn[].  Characteristics
 * will be eqn's where the first half is all zero but the second
 * half isn't.  Those characteristics for a basis of the space of
 * all characteristics; by XORing them together we can get all other
 * characteristics.
 */
static void setup( seed, pre, post, eqn, rctx)
bitvec  *seed;
bitvec **pre;
bitvec **post;
bitvec **eqn;
randctx *rctx;
{
   bitvec  this[MAKEBIT(MIXSIZE)];
   bitvec  that[MAKEBIT(MIXSIZE)];
   word    i;

   /* Choose a seed */
   for (i=0; i<MAKEBIT(MIXSIZE); ++i) seed[i] = rand(rctx);

   /* Use the mixing function to find the MIXSIZE equations */
   for (i=0; i<MIXSIZE; ++i)
   {
      /* Get two seeds that differ by only one bit */
      bitvcpy(this, seed, MIXSIZE);
      if (bitvtst(seed, i)) bitv0(this, i);
      else bitv1(this, i);

      /* find the preimage */
      bitvcpy(pre[i], seed, MIXSIZE);
      bitvcpy(that, this, MIXSIZE);
      preimage((ub4 *)pre[i]);
      preimage((ub4 *)that);
      bitvxor(pre[i], pre[i], that, MIXSIZE);

      /* find the postimage */
      bitvcpy(post[i], seed, MIXSIZE);
      bitvcpy(that, this, MIXSIZE);
      postimage((ub4 *)post[i]);
      postimage((ub4 *)that);
      bitvxor(post[i], post[i], that, MIXSIZE);

      /* set up characteristic equations */
      bitvxor(eqn[i], pre[i], post[i], MIXSIZE);
      bitvcpy(eqn[i]+MAKEBIT(MIXSIZE), pre[i], MIXSIZE);
   }
}




/* Given the equations, find the characteristics.
 * We'll be doing Gaussian elimination on eqn[].  Characteristics
 *   will be eqn's where the first half is all zero but the second
 *   half isn't.
 * If the equations were chosen randomly, we'd expect about one
 *   row in the base of characteristics.
 */
static void find( eqn, base, row)
bitvec **eqn;
bitvec **base;
word    *row;
{
   word   i;
   bitvec nichts[MAKEBIT(MIXSIZE)];

   *row = gauss(eqn, MIXSIZE, 2*MIXSIZE);
   bitvclr(nichts, MIXSIZE);

   for (i=*row, *row=0; i--;)
   {
      if (!bitvcmp(eqn[i],nichts,MIXSIZE)) 
      {
         bitvcpy(base[*row], eqn[i]+MAKEBIT(MIXSIZE), MIXSIZE);
         ++*row;
      }
      else break;
   }
}

/* Test a characteristic */
static void test( seed, characteristic)
bitvec *seed;
bitvec *characteristic;
{
   ub4    i,count,found;
   word   j;
   bitvec this[MAKEBIT(MIXSIZE)];
   bitvec that[MAKEBIT(MIXSIZE)];

   /* Test TESTSIZE seeds to see if the characteristic works */
   bitvcpy(this,seed,MIXSIZE);
   for (i=0, found=0; i<TESTSIZE; ++i)
   {
      /* apply the mapping */
      bitvxor(that, this, characteristic, MIXSIZE);
      mix(that);
      mix(this);
      bitvxor(that, this, that, MIXSIZE);
      if (!bitvcmp(characteristic, that, MIXSIZE)) ++found;
   }
   if (found)
   {
      printf("%d !! ",found);
      bitprint(characteristic,MIXSIZE);
   }
   else if (0)  /* try to figure out how bad we're failing */
   {
      bitvxor(that, that, characteristic, MIXSIZE);
      for (j=0, count=0; j<MIXSIZE; ++j) 
         if (bitvtst(characteristic,j)) ++count;
      if (1 || count<MIXSIZE/3) 
      {
         printf("%ld  ",count); 
         bitprint(characteristic, MIXSIZE);
      }
   }
}


/* Check if the characteristics really are characteristics
 * Given a characteristic base with row rows, we have 2^row-1 possible
 *   characteristics.  Generate each and test it against 256 seeds
 *   to see if the characteristic really works.
 * Any characteristic that actually works, print it out.
 */
static void detect( seed, base, row)
bitvec  *seed;
bitvec **base;
word     row;
{
   bitvec i,j,k;
   bitvec characteristic[MAKEBIT(MIXSIZE)];
   bitvec this[MAKEBIT(MIXSIZE)];

   if (0)
   {
      for (i=1; i; ++i) 
      {
         for (j=1,k=0; j; j<<=1) if (i&j) ++k;
         if (k<9) test(seed, &i);
      }
      printf("once around\n");
   }
   else if (row <= MAXBASE) for (i=1; i<(((bitvec)1)<<row); ++i)
   {
      /* Generate the 2^row characteristics from the base */
      bitvcpy(this, seed, MIXSIZE);
      xym(characteristic, &i, base, row, MIXSIZE);
      test(seed,characteristic);
   }
   else for (i=0; i<row; ++i)
   {
      /* base is too big .. just test all elements of the base */
      test(seed,base[i]);
   }
}




/* Find and print out the characteristics of a function */
static void driver(rctx)
randctx    *rctx;
{
   reroot  *root1, *root2;  /* memory sources for bitvecs */
   word     i, row;
   bitvec  *pre[MIXSIZE];  /* preimage deltas for each bit in seed */
   bitvec  *post[MIXSIZE]; /* postimage delta for each bit in seed */
   bitvec  *eqn[MIXSIZE];  /* used to find characteristic basis */
   bitvec  *base[MIXSIZE]; /* base of all characteristics */
   bitvec  *seed;          /* seed used to produce deltas */

   /* allocate memory */
   root1 = remkroot(sizeof(bitvec)*MAKEBIT(MIXSIZE));
   root2 = remkroot(sizeof(bitvec)*2*MAKEBIT(MIXSIZE));
   for (i=0; i<MIXSIZE; ++i)
   {
      pre[i]  = (bitvec *)renew(root1);
      post[i] = (bitvec *)renew(root1);
      eqn[i]  = (bitvec *)renew(root2);
      base[i] = (bitvec *)renew(root1);
   }
   seed  = (bitvec *)renew(root1);

   printf("\n\n");
   printf("This program will use %d different random seeds\n",SEEDS);
   printf("to try to estimate characteristics.  A basis of the\n");
   printf("space of characteristics will be found for each seed,\n");
   printf("and the size of that basis will be printed.\n");
   printf("If the basis has less than %d terms, then all\n",MAXBASE);
   printf("nonzero characteristics will be tested, otherwise\n");
   printf("only the base characteristics will be tested.\n\n");
   printf("Each characteristic is tested on %d pairs.\n",TESTSIZE);
   printf("If it succeeds at all, the number of successes will\n");
   printf("be printed, followed by !! and the characteristic.\n\n");

   /* find characteristics based on SEEDS different random seeds */
   for (i=0; i<SEEDS; ++i)
   {
      /* use the seeds and mixing functions to set up equations */
      setup(seed, pre, post, eqn, rctx);

      /* Find a basis for the space of characteristics */
      find(eqn, base, &row);

      printf("   Size of base is %d\n",row);
      
      /* Try to actually detect each characteristic */
      detect(seed, base, row);
      /* detect(seed, pre, MIXSIZE); */
   }

   /* free everything */
   refree(root1);
   refree(root2);
}

int main()
{
  randctx rctx;

  randinit(&rctx, FALSE);
  driver(&rctx);
  return TRUE;
}
