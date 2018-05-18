/*
------------------------------------------------------------------------------
test.h - Run the avalanche test on reversible functions
By Bob Jenkins, 1995.  Public Domain.

This code is designed to be changed.
Code that changes belongs in test.c
Generic code belongs in modules of its own (recycle.c, rand.c, ...)
Nonchanging code that isn't generic belongs in testb.c

A reversible function f:A->A is said to have the avalanche property
  if, for every input bit, for every output bit, two inputs differing
  in only that bit are expected to differ in that output bit with
  probability 1/2.
------------------------------------------------------------------------------
*/
#ifndef STANDARD
#include "standard.h"
#endif
#ifndef RECYCLE
#include "recycle.h"
#endif
#ifndef RAND
#include "rand.h"
#endif

#ifndef TEST
#define TEST

#define MAXPARA   8   /* maximum number of sets of parameters */
#define MAINPAR  30   /* maximum number of main parameters */
#define MINEPAR  30   /* maximum number of minor parameters */
#define NUMRSLT  10   /* maximum number of results gathered by test */
#define NUMBITS  32   /* number of bits per word */
#define MAXARR   16   /* maximum input size is MAXARR ub4's */


/*------------------------------------------------------------- structures */
/* parameters for the hash function */
struct tparam
{
  void    (*fun)();    /* the function template to test */
  ub4 p[MAINPAR];      /* main parameters */
  ub4 q[MINEPAR];      /* minor parameters */
  sb4 good;            /* goodness; zeroed by tl* */
  struct tparam *next; /* next parameter */
};
typedef  struct tparam  tparam;

/* tnode: describe which input bit to test */
struct tnode
{
  struct tnode *next;     /* the next test */
  ub4           clear;    /* bits clear..clear+7 have no delta bits set */
  tparam       *param;    /* parameters to test */
  ub4           delta[1]; /* the delta to be tested */
};
typedef  struct tnode  tnode;

/* troot: list all tests that should be run */
struct troot
{
  ub4       count;     /* total number of tests */
  tnode    *head;      /* head of list of tests */
  reroot   *mem;       /* root of memory for tests */
};
typedef  struct troot  troot;

/* tctrl: describe the testing strategy */
struct tctrl
{
  void (*gen)();      /* generate parameters to be tested */
  void (*build)();    /* build the list of tests to be run */
  void (*other)();    /* build other parameters from param[0] */
  void (*loop)();     /* way of looping over all tests to be run */
  word (*test)();     /* type of test to run */
  void (*first)();    /* generate the first input to be tested */
  void (*delta)();    /* test inputs differing by delta */
  void (*outsucc)();  /* output per successful parameter */
  void (*outfail)();  /* output per failed parameter */
  tparam param[MAXPARA]; /* parameters for the test */
  troot  root;        /* list of tests */
  ub4    numparam;    /* number of sets of parameters */
  ub4    numbits;     /* number of bits per word */
  ub4    numword;     /* number of words in the input */
  ub4    numtest;     /* maximum number of tests to run */
  sb4    mylimit;     /* limit; test-specific */
  randctx rctx;       /* source of randomness */
};
typedef  struct tctrl  tctrl;

/*------------------------------------------------------ available routines */
/* objects being tested */
void forwards(/*_ ub4 *a, tparam *p _*/);
void backward(/*_ ub4 *a, tparam *p _*/);

/* tg* - generate the parameters to be tested */
void tgexh(/*_ tctrl *ctrl _*/);
void tgran(/*_ tctrl *ctrl _*/);
void tggen(/*_ tctrl *ctrl _*/);
void tgone(/*_ tctrl *ctrl _*/);
void tgfil(/*_ tctrl *ctrl _*/);

/* tb* - build the list of tests to be run */
void tbone(/*_ tctrl *ctrl _*/);
void tbtwo(/*_ tctrl *ctrl _*/);
void tbtop(/*_ tctrl *ctrl _*/);
void tbbot(/*_ tctrl *ctrl _*/);

/* to* - copy param[0] to the other parameters in this set */
void tofoo(/*_ tctrl *ctrl _*/);

/* tl* - loop through all the tests */
void tlmrf(/*_ tctrl *ctrl _*/);
void tlord(/*_ tctrl *ctrl _*/);

/* tt* - do a test */
word ttsan(/*_ tnode *t, tctrl *ctrl _*/);
word ttlim(/*_ tnode *t, tctrl *ctrl _*/);
word ttlea(/*_ tnode *t, tctrl *ctrl _*/);
word ttcou(/*_ tnode *t, tctrl *ctrl _*/);
word ttbyt(/*_ tnode *t, tctrl *ctrl _*/);

/* tp* - generate a pair of inputs */
void tpcou(/*_ ub4 i, tnode *t, tctrl *ctrl, ub4 *a _*/);
void tpexp(/*_ ub4 i, tnode *t, tctrl *ctrl, ub4 *a _*/);
void tpran(/*_ ub4 i, tnode *t, tctrl *ctrl, ub4 *a _*/);

/* ts* - print results upon success */
void tsnul(/*_ tparam *p _*/);
void tshow(/*_ tparam *p _*/);

/* tf* - print results upon failure */
void tfnul(/*_ char *str, ub4 *state, tnode *t, tctrl *ctrl _*/);
void tfsho(/*_ char *str, ub4 *state, tnode *t, tctrl *ctrl _*/);

/* td* - types of deltas */
void tdxor(/*_ tctrl *ctrl, tparam *p, ub4 *a, ub4 *delta, ub4 *c _*/);
void tdnxr(/*_ tctrl *ctrl, tparam *p, ub4 *a, ub4 *delta, ub4 *c _*/);
void tdsub(/*_ tctrl *ctrl, tparam *p, ub4 *a, ub4 *delta, ub4 *c _*/);
void tdadd(/*_ tctrl *ctrl, tparam *p, ub4 *a, ub4 *delta, ub4 *c _*/);


#endif  /* TEST */
