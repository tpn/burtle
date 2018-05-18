/*
------------------------------------------------------------------------------
test.c -
By Bob Jenkins, 1995.  Public Domain.
Run the avalanche test on some function forwards() and its reverse backward().
Any routines that might change over time go in this file.
For the reasoning behind this program, see
  http://burtleburtle.net/bob/hash/evahash.html
Currently searching for function structures using 3 4-byte values.
------------------------------------------------------------------------------
*/

#ifndef STANDARD
#include "standard.h"
#endif
#ifndef RAND
#include "rand.h"
#endif
#ifndef RECYCLE
#include "recycle.h"
#endif
#ifndef TEST
#include "test.h"
#endif

#define NUMTERMS 3

/*
------------------------------------------------------------------------------
tg* - routines to generate parameters
------------------------------------------------------------------------------
*/

/* exhaustive search */
void tgexh(ctrl)
tctrl  *ctrl;
{
  ub4 i,j;
  tparam *p = &ctrl->param[0];
  p->p[0] = 1; p->p[1] = 0;
  p->p[2] = 1; p->p[3] = 0;
  p->p[4] = 0; p->p[5] = 2;
  for (p->q[0]=0; p->q[0]<32; ++p->q[0])
  for (p->q[1]=0; p->q[1]<32; ++p->q[1])
  for (p->q[2]=0; p->q[2]<32; ++p->q[2])
  for (p->q[3]=0; p->q[3]<32; ++p->q[3])
  for (p->q[4]=0; p->q[4]<32; ++p->q[4])
  for (p->q[5]=0; p->q[5]<32; ++p->q[5])
  {
    (*ctrl->loop)(ctrl);
  }
}

static int qqq;

/* random search */
void tgran(ctrl)
tctrl  *ctrl;
{
  ub4 x;
  tparam *p = &ctrl->param[0];
  p->p[0]=0;
  for (p->p[1]=0; p->p[1]<NUMTERMS; ++p->p[1])
  if (p->p[0]!=p->p[1])
  for (p->p[2]=0; p->p[2]<NUMTERMS; ++p->p[2])
  for (p->p[3]=0; p->p[3]<NUMTERMS; ++p->p[3])
  if (p->p[2]!=p->p[3])
  for (p->p[4]=0; p->p[4]<NUMTERMS; ++p->p[4])
  for (p->p[5]=0; p->p[5]<NUMTERMS; ++p->p[5])
  if (p->p[4]!=p->p[5])
  for (x=0; x<100000; ++x)
  {
    ub4 i;
    qqq = 0;
    do {p->q[0] = (rand(&ctrl->rctx)&31);} while ((p->q[0]<0) || (p->q[0]>31));
    do {p->q[1] = (rand(&ctrl->rctx)&31);} while ((p->q[1]<0) || (p->q[1]>31));
    do {p->q[2] = (rand(&ctrl->rctx)&31);} while ((p->q[2]<0) || (p->q[2]>31));
    do {p->q[3] = (rand(&ctrl->rctx)&31);} while ((p->q[3]<0) || (p->q[3]>31));
    do {p->q[4] = (rand(&ctrl->rctx)&31);} while ((p->q[4]<0) || (p->q[4]>31));
    do {p->q[5] = (rand(&ctrl->rctx)&31);} while ((p->q[5]<0) || (p->q[5]>31));
    do {p->q[6] = (rand(&ctrl->rctx)&31);} while ((p->q[6]<0) || (p->q[6]>31));
    do {p->q[7] = (rand(&ctrl->rctx)&31);} while ((p->q[7]<0) || (p->q[7]>31));
    do {p->q[8] = (rand(&ctrl->rctx)&31);} while ((p->q[8]<0) || (p->q[8]>31));
    (*ctrl->loop)(ctrl);
    if (qqq) break;
  }
}

/*
 * Read in parameters from standard input.  "test < inp.txt > out.txt".
 * The file is typically the successes from a previous round of tests.
 */
void tgfil(ctrl)
tctrl  *ctrl;
{
  ub4     i,dummy;
  tparam *p = &ctrl->param[0];
  while (13==fscanf(stdin,"%ld %ld %ld %ld %ld %ld  %ld %ld %ld %ld %ld %ld  %ld +",
                   &p->p[0],&p->p[1],&p->p[2],&p->p[3],&p->p[4],&p->p[5],
                   &p->q[0],&p->q[1],&p->q[2],&p->q[3],&p->q[4],&p->q[5],
                   &dummy))
  {
    (*ctrl->loop)(ctrl);
  }
}

/* hardcode the parameters */
void tgone(ctrl)
tctrl  *ctrl;
{
  ub4 i,j;
  tparam *p = &ctrl->param[0];
  p->p[0] = 1; p->p[1] = 0;
  p->p[2] = 1; p->p[3] = 0;
  p->p[4] = 0; p->p[5] = 2;
  for (i=0; i<100000; ++i)
  {
    p->q[0]=rand(&ctrl->rctx)%32;     p->q[1]=rand(&ctrl->rctx)%32;
    p->q[2]=rand(&ctrl->rctx)%32;     p->q[3]=rand(&ctrl->rctx)%32;
    p->q[4]=rand(&ctrl->rctx)%32;     p->q[5]=rand(&ctrl->rctx)%32;
    p->q[6]=rand(&ctrl->rctx)%32;     p->q[7]=rand(&ctrl->rctx)%32;
#ifdef NEVER
    /* make sure no shifting by zero */
    for (j=0; j<8; ++j) if (!p->q[j]) break;
    if (j != 8) break;
#endif
    (*ctrl->loop)(ctrl);
  }
}

/*
------------------------------------------------------------------------------
to* - copy parameters
Suppose you want to run tests on the function forwards and backwards,
normally and starting from halfway through.  Then you make 4 sets of
parameters, one for each of the options.  tg* generates param[0], then
calls tl* which calls this to derive param[1..ctrl->numparam-1],
then tl* runs them.  tb* creates tests pointing to each param[i].
The goal is to give tlmrf() more tests to choose from.
  2: backward, forward
  4: backward, forward, start at lines 0 and 4
  8: backward, forward, start at lines 0, 2, 4, 6
------------------------------------------------------------------------------
*/
void tofoo(ctrl)
tctrl *ctrl;
{
  ub4 i,j,k;
  tparam *p0 = &ctrl->param[0];
  p0->fun = forwards;
  for (i=1; i<ctrl->numparam; ++i)
  {
    tparam *pi = &ctrl->param[i];
    pi->fun = (bit(i,1) ? backward : forwards);
    for (j=0; j<6; ++j) pi->p[j] = p0->p[j];
    k=((bit(i,2)<<1)^(bit(i,4)>>1));
    for (j=0; j<2*NUMTERMS; ++j) pi->q[j]=p0->q[j];
    for (j=0; j<k; ++j) 
      pi->q[j] = p0->q[(j+NUMTERMS-k)%NUMTERMS];
    for (j=k; j<NUMTERMS; ++j)
      pi->q[j] = p0->q[(j-k)%NUMTERMS];
  }
}

/*
------------------------------------------------------------------------------
ts* - show results if test is successful
------------------------------------------------------------------------------
*/
void tsnul(p)
tparam *p;   /* parameters being tested */
{
}

void tshow(p)
tparam *p;   /* parameters being tested */
{
  ub4 i;
  for (i=0; i<6; ++i)
    printf("%1ld ",p->p[i]);
  printf("  ");
  for (i=0; i<2*NUMTERMS; ++i)
    printf("%2ld ",p->q[i]);
  printf("  %ld +\n",p->good);
  qqq = 1;
}

/* mark it */
tsmar(p)
tparam *p;   /* parameters being tested */
{
  p->q[MINEPAR-1] = 1;
}
/*
------------------------------------------------------------------------------
tf* - show results if a test fails
------------------------------------------------------------------------------
*/
void tfnul(str,state,t,ctrl)
char  *str;
ub4   *state;
tnode *t;
tctrl *ctrl;
{
  tparam *p = t->param;
  static ub4 i=0;
#ifdef NEVER
  if (!(i&0xfff)) printf("%ld\n",i);
  ++i;
  for (i=0; i<ctrl->numword; ++i) printf("%.8lx ",~state[i]);
  printf("   %ld -\n",p->good);
#endif
}

void tfsho(str,state,t,ctrl)
char  *str;
ub4   *state;
tnode *t;
tctrl *ctrl;
{
  ub4 i;
  tparam *p = t->param;
  for (i=0; i<NUMTERMS; ++i)
    printf("%3ld ",p->q[i]);
  for (i=0; i<ctrl->numword; ++i) printf("%.8lx",t->delta[i]);
  printf(" state ");
  for (i=0; i<ctrl->numword; ++i) printf("%.8lx",state[i]);
  printf(" %ld   %s -\n",p->good, str);
#ifdef NEVER
  printf("param %3ld %3ld %3ld %3ld %3ld %3ld -\n",
    p->p[0],p->p[1],p->p[2],p->p[3],p->p[4],p->p[5]);
  printf("  ");
#endif
}

/*
------------------------------------------------------------------------------
object being tested: the routines forwards and backward
------------------------------------------------------------------------------
*/

/* inverse i+=i<<s */
static ub4 inval(i,s)
ub4 i;
ub4 s;
{
  i -= i<<s;  s <<= 1;
  while (0<s && s<32) { i += i<<s; s <<= 1; }
  return i;
}

/* inverse i^=i<<s */
static ub4 invsl(i,s)
ub4 i;
ub4 s;
{
  while (0<s && s<32) { i ^= i<<s; s <<= 1; }
  return i;
}

/* inverse i^=i>>s */
static ub4 invsr(i,s)
ub4 i;
ub4 s;
{
  while (0<s && s<32) { i ^= i>>s; s <<= 1; }
  return i;
}

#define rot(a,b) (((a)<<(b))+((a)>>(32-(b))))
#define gee(r,k) \
  a[r[p->p[0]]]-=a[r[p->p[1]]]; \
  a[r[p->p[2]]]^=rot(a[r[p->p[3]]], p->q[k]); \
  a[r[p->p[4]]]+=a[r[p->p[5]]];

static void myff(a, p)
ub4    *a;
tparam *p;
{
const int r0[] = {0,1,2};
const int r1[] = {1,2,0};
const int r2[] = {2,0,1};
  gee(r0,0);
  gee(r1,1);
  gee(r2,2);
  gee(r0,3);
  gee(r1,4);
  gee(r2,5);
}

static void myf(a, p)
ub4    *a;
tparam *p;
{
}

#define gaw(r,k) \
  a[r[p->p[4]]]-=a[r[p->p[5]]]; \
  a[r[p->p[2]]]^=rot(a[r[p->p[3]]], p->q[k]); \
  a[r[p->p[0]]]+=a[r[p->p[1]]];
static void mybb(a, p)
ub4    *a;
tparam *p;
{
const int r0[] = {0,1,2};
const int r1[] = {1,2,0};
const int r2[] = {2,0,1};
  gaw(r2,5);
  gaw(r1,4);
  gaw(r0,3);
  gaw(r2,2);
  gaw(r1,1);
  gaw(r0,0);
}

static void myb(a, p)
ub4    *a;
tparam *p;
{
}

void forwards(x,p)
ub4    *x;   /* input, overwrite with output */
tparam *p;   /* parameters for the function */
{
  ub4 i,j;
  ub4 a[NUMTERMS];
  for (i=0; i<NUMTERMS; ++i) 
    a[i] = x[i];
#ifdef NEVER
printf("a ");
for (j=0; j<2*NUMTERMS; ++j) printf("%ld ",p->q[j]);
printf("\n");
  printf("hoo %.8lx %.8lx %.8lx %.8lx\n",a[0],a[1],a[2],a[3]);
#endif
  myff(a,p);
  for (i=0; i<NUMTERMS; ++i) 
    { x[i]=a[i];}
}

void backward(x,p)
ub4    *x;   /* input, overwrite with output */
tparam *p;   /* parameters for the function */
{
  ub4 i,j;
  ub4 a[NUMTERMS];
  for (i=0; i<NUMTERMS; ++i) 
    a[i] = x[i];
#ifdef NEVER
printf("b ");
for (j=0; j<2*NUMTERMS; ++j) printf("%ld ",p->q[j]);
printf("\n");
#endif
  mybb(a,p);
  for (i=0; i<NUMTERMS; ++i) 
    { x[i]=a[i]; }
}

/*
------------------------------------------------------------------------------
control routines and constants
------------------------------------------------------------------------------
*/
#define NUMWORD NUMTERMS   /* number of 4-byte words in input */
#define NUMTEST 4
#define MYLIMIT -32
#define NUMPPPP 2

/* start running the program */
static void driver(ctrl)
tctrl *ctrl;
{
  ub4 i;
  for (i=0; i<RANDSIZ; ++i) ctrl->rctx.randrsl[i] = 0; /* random number seed */
  randinit(&ctrl->rctx, TRUE);     /* initialize the random number generator */
  for (i=0; i<MINEPAR; ++i)          /* choose some minor parameters */
    ctrl->param[0].q[i] = (rand(&ctrl->rctx)%32)+3;
  ctrl->root.mem = remkroot(sizeof(tnode)+(4*(NUMWORD-1)));
  ctrl->build(ctrl);                           /* make list of tests */
  ctrl->gen(ctrl);              /* for all parameters, run all tests */
  refree(ctrl->root.mem);                 /* free the list of tests */
}

/* decide what exactly this program should do */
int main()
{
  tctrl ctrl;
  /*
   * ctrl.gen:   generate the parameters to be tested
   *             tgexh: exhaustive search of all parameters
   *             tgran: generate parameters at random
   *             tgone: generate one hardcoded set of parameters
   *             tgfil: read a list of parameters from a file
   */
  ctrl.gen     = tgone;
  /*
   * ctrl.other: copy ctrl->param[0] into param[1..ctrl->numparam-1]
   *             tofoo: run tests forward, backwards, and rotate parameters 
   */
  ctrl.other   = tofoo;
  /*
   * ctrl.loop:  loop through all the tests, return as soon as one fails
   *             tlmrf: most recently failed tests are run first
   *             tlord: run tests in the order they were built
   */
  ctrl.loop    = tlmrf;
  /*
   * ctrl.test:  what test should be run
   *             ttsan: sanity check that forward(backward) == identity
   *             ttlim: fail if some output is not affected by the limit
   *             ttlea: run to limit, find least affected output bit
   *             ttcou: run to limit, count how many output bits were affected
   *             ttbyt: count how many times each byte value appears
   */
  ctrl.test    = ttcou;
  /*
   * ctrl.build: build a list of tests to be run for each param set
   *             tbone: test all deltas of one input bit
   *             tbtwo: test all deltas of two input bits
   *             tbtop: test all deltas of only top bits
   *             tbbot: test all deltas of only bottom bits
   */
  ctrl.build   = tbone;
  /* 
   * ctrl.first: given the delta, generate a first input to test
   *             tpcou: test delta, delta^1, delta^2, delta^3, ...
   *             tpexp: test delta, delta^1, delta^2, delta^4, ...
   *             tpran: test delta^(random)
   */
  ctrl.first   = tpran;
  /*
   * ctrl.delta: given the delta and the first input, find the second input
   *             tdxor: b ^ a = delta
   *             tdnxr: b ^~a = delta
   *             tdsub: b - a = delta
   *             tdadd: b + a = delta
   */
  ctrl.delta   = tdadd;
  /* ctrl.outsucc: report parameters/test results after passing all tests
   *             tsnul: don't print anything
   *             tshow: show everything
   */
  ctrl.outsucc = tshow;
  /* ctrl.outfail: report parameters/test results after failing a test 
   *             tfnul: don't print anything
   *             tfsho: show everything
   */
  ctrl.outfail = tfnul;
  ctrl.numbits = NUMBITS;
  ctrl.numword = NUMWORD;
  ctrl.numtest = NUMTEST;
  ctrl.mylimit = MYLIMIT;
  ctrl.numparam= NUMPPPP;
  if (NUMWORD > MAXARR)
  {
    printf("error!  NUMWORD %d > MAXARR %d\n",NUMWORD, MAXARR);
    return 0;
  }

  /* actually run the test */
  driver(&ctrl);
  return 1;
}

