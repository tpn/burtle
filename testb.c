/*
------------------------------------------------------------------------------
testb - all the avalanche test routines that are not likely to change
By Bob Jenkins, 1995.  Public Domain.
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

/* convert an additive delta to a minimalist representation */
#define convdelt(a) ((a)^((a)>>1))

/*
------------------------------------------------------------------------------
tb* - build a list of tests to be run.
------------------------------------------------------------------------------
*/

/*
 * For every input word, for every bit of that word, for both the
 * function being tested (forwards) and its reverse (backwards),
 * put a node on a list signifying that that configuration needs to
 * be tested.
 */
void tbone(ctrl)
tctrl     *ctrl;        /* describes everything */
{
  register ub4    i,j,k,l;
  register tnode *x, *y;
  troot          *root = &ctrl->root;
  root->count = 0;
  x = (tnode *)0;
  for (i=0; i<ctrl->numword; ++i)
    for (j=0; j<ctrl->numbits; ++j)
      for (k=0; k<ctrl->numparam; ++k)
      {
        y = (tnode *)renew(root->mem);
        for (l=0; l<ctrl->numword; ++l) y->delta[l] = 0;
        y->delta[i] ^= (ub4)1<<j;
        for (l=ctrl->numword; --l;)
          if (!y->delta[l]) break;
        y->clear = l;
        y->param = &ctrl->param[k];
        y->next  = x;
        x = y;
        ++root->count;
      }
  root->head = x;
}


/*
 * Same as tbone, but instead of all single-bit deltas, try all deltas
 * that have only the high bit set for some combination of words.
 */
void tbtop(ctrl)
tctrl     *ctrl;        /* describes everything */
{
  register ub4    i,j,k,l,mask;
  register tnode *x, *y;
  troot          *root = &ctrl->root;
  root->count = 0;
  x = (tnode *)0;
  mask = (ub4)1<<(ctrl->numbits-1);
  i = (ub4)1<<ctrl->numword;
  while (--i)
  {
    for (k=0; k<ctrl->numparam; ++k)
    {
      y = (tnode *)renew(root->mem);
      for (l=0; l<ctrl->numword; ++l) y->delta[l] = 0;
      for (j=1,l=0; j<=i; j=j<<1, ++l)
        if (j&i) y->delta[l] ^= mask;
      for (l=ctrl->numword; --l;)
        if (!y->delta[l]) break;
      y->clear = l;
      y->param = &ctrl->param[k];
      y->next  = x;
      x = y;
      ++root->count;
    }
  }
  root->head = x;
}


/*
 * Same as tbone, but instead of all single-bit deltas, try all deltas
 * that have only the high bit set for some combination of words.
 */
void tbbot(ctrl)
tctrl     *ctrl;        /* describes everything */
{
  register ub4    i,j,k,l,mask;
  register tnode *x, *y;
  troot          *root = &ctrl->root;
  root->count = 0;
  x = (tnode *)0;
  mask = 1;
  i = (ub4)1<<ctrl->numword;
  while (--i)
  {
    for (k=0; k<ctrl->numparam; ++k)
    {
      y = (tnode *)renew(root->mem);
      for (l=0; l<ctrl->numword; ++l) y->delta[l] = 0;
      for (j=1,l=0; j<=i; j=j<<1, ++l)
        if (j&i) y->delta[l] ^= mask;
      for (l=ctrl->numword; --l;)
        if (!y->delta[l]) break;
      y->clear = l;
      y->param = &ctrl->param[k];
      y->next  = x;
      x = y;
      ++root->count;
    }
  }
  root->head = x;
}


/*
 * For every pair of input bits, put a node on a list saying that that
 * delta should be tested on the permutation both forward and backward.
 */
void tbtwo(ctrl)
tctrl     *ctrl;        /* describes everything */
{
  ub4    i,j,k,l,m,n;
  register tnode *x, *y;
  troot *root = &ctrl->root;
  root->count = 0;
  x = (tnode *)0;
  for (i=0; i<ctrl->numword; ++i)
    for (j=0; j<ctrl->numword; ++j)
      for (k=0; k<ctrl->numbits; ++k)
        for (l=0; l<ctrl->numbits; ++l)
          for (m=0; m<ctrl->numparam; ++m)
          {
            if ((i==j) && (k==l)) break;
            if ((i>j) || ((i==j)&&(k>l))) break;
            y = (tnode *)renew(root->mem);
            for (n=0; n<ctrl->numword; ++n) y->delta[n] = 0;
            y->delta[i] ^= (ub4)1<<k;
            y->delta[j] ^= (ub4)1<<l;
            for (n=ctrl->numword; --n;)
              if (!y->delta[n]) break;
            y->clear = n;
            y->param = &ctrl->param[m];
            y->next = x;
            x = y;
            ++root->count;
          }
  root->head = x;
}

/*
------------------------------------------------------------------------------
tl* - routines for looping over all tests given some set of parameters.
------------------------------------------------------------------------------
*/

/* Do all the tests.  Most Recently Failed test goes in front of list.
 * If any test fails, make it the front of the list and return FALSE.
 * Otherwise return TRUE.
 */
void tlmrf(ctrl)
tctrl     *ctrl;                                             /* context */
{
  tnode **t, *u;
  ub4     count,i;
  troot  *root = &ctrl->root;

  /* for each test */
  (*ctrl->other)(ctrl);
  for (count=0; count<ctrl->numparam; ++count)
    ctrl->param[count].good = -0xfffffff;
  for (count=0, t=&root->head; *t; t=&((*t)->next), ++count)
  {
    if (!(*ctrl->test)(*t, ctrl))             /* run the test */
    {                                        /* the test failed */
      u = *t;
      *t = (*t)->next;
      u->next = root->head;
      root->head = u;           /* move it to the head of the list */
      for (i=1; i<ctrl->numparam; ++i) if 
        (ctrl->param[0].good < ctrl->param[i].good) 
          ctrl->param[0].good = ctrl->param[i].good;
      return;
    }
  }
  /* all tests have passed */
  if (count != root->count)     /* sanity check */
  {
    printf("internal error! only %ld out of %ld tests were run\n",
           count, root->count);
  }
  for (i=1; i<ctrl->numparam; ++i) if 
    (ctrl->param[0].good < ctrl->param[i].good) 
      ctrl->param[0].good = ctrl->param[i].good;
  if (ctrl->outsucc) (*ctrl->outsucc)(&ctrl->param[0]);
}

/*
 * Do all the tests in order.
 * Return FALSE as soon as some test returns FALSE, otherwise return TRUE.
 */
void tlord(ctrl)
tctrl    *ctrl;                                        /* context */
{
  tnode *t;
  ub4    count,i;
  (*ctrl->other)(ctrl);
  for (count=0; count<ctrl->numparam; ++count)
    ctrl->param[count].good = -0xfffffff;
  for (t=ctrl->root.head; t; t=(t->next))
  {                                              /* for each test */
    if (!(*ctrl->test)(t, ctrl))                 /* run the test */
    {
      for (i=1; i<ctrl->numparam; ++i) if 
        (ctrl->param[0].good < ctrl->param[i].good) 
          ctrl->param[0].good = ctrl->param[i].good;
      return;
    }
  }
  for (i=1; i<ctrl->numparam; ++i) if 
    (ctrl->param[0].good < ctrl->param[i].good) 
      ctrl->param[0].good = ctrl->param[i].good;
  if (ctrl->outsucc) (*ctrl->outsucc)(&ctrl->param[0]);
}


/*
------------------------------------------------------------------------------
tt* - Given some input delta and function, check all output bits
------------------------------------------------------------------------------
*/

/* sanity check: make sure that backward(forwards(x)) == x */
word  ttsan(t, ctrl)
tnode   *t;
tctrl   *ctrl;
{
  ub4 i,rsl=TRUE;
  ub4 u[MAXARR];
  tparam *p = t->param;
  for (i=0; i<ctrl->numword; ++i) u[i] = t->delta[i];
  forwards(u, p);
  backward(u, p);
  for (i=0; i<ctrl->numword; ++i) 
    if (u[i] != t->delta[i])
      rsl=FALSE;
  if (rsl==TRUE)
  {
    forwards(u, p);
    forwards(u, p);
    forwards(u, p);
    forwards(u, p);
    backward(u, p);
    backward(u, p);
    backward(u, p);
    backward(u, p);
  }
  for (i=0; i<ctrl->numword; ++i) 
    if (u[i] != t->delta[i])
    {
      if (ctrl->outfail) (*ctrl->outfail)("sanity",u,t,ctrl);
      rsl=FALSE;
      break;
    }
  redel(ctrl->root.mem,u);
  return rsl;
}

/*
 * Run trials until every output bit has changed and not changed once.
 * If some output bit has not been affected after NUMTEST trials,
 *   then return failure.
 * This is the fastest version of the avalanche test, requiring on
 *   as few as NUMTEST trials to fail a bad function and at most 
 *   2n*NUMTEST trials to pass a good one.  NUMTEST should be a little
 *   more than log_2(4nn) for a permutation with n-bit inputs, since
 *   there are 4nn tests and each passes each trial with probability
 *   1/2.
 */
word ttlim(t, ctrl)
tnode   *t;
tctrl   *ctrl;
{
  sb4 i, j, rsl = TRUE;
  ub4 a[MAXARR];
  ub4 c[MAXARR];
  ub4 change[MAXARR];    /* do bits change? */
  ub4 same[MAXARR];      /* do bits stay the same? */
  tparam *p = t->param;
  for (j=0; j<ctrl->numword; ++j)
    change[j] = same[j] = ~(ub4)0;
  for (i=0; i<ctrl->numtest; ++i)
  {
    ctrl->first(i, t, ctrl, a);
    ctrl->delta(ctrl, p, a, t->delta, c);
    for (j=0; j<ctrl->numword; ++j)
    {
      change[j] &= ~c[j];
      same[j]   &=  c[j];
    }
    for (j=0 /*ctrl->numword-1*/; j<ctrl->numword; ++j)
    {
      if (change[j] || same[j]) break;
    }
    if (j == ctrl->numword) break;
  }
  if (p->good < i) p->good = i;
  if (j < ctrl->numword)
  {
    rsl = FALSE;
    if (ctrl->outfail)
    {
      if (change[j])
        (*ctrl->outfail)("change",change,t,ctrl);
      else if (same[j])
        (*ctrl->outfail)("same",same,t,ctrl);
    }
  }
  return rsl;
}


/*
 * Run NUMTEST trials, then count how many bits have changed, and how
 *   many have not changed.
 * If fewer than ctrl->mylimit bits have changed or not changed, fail.
 */
word ttcou(t, ctrl)
tnode   *t;
tctrl   *ctrl;
{
  ub4 i, j, k, rsl = TRUE;
  sb4 cc,cs;
  ub4 a[MAXARR];
  ub4 c[MAXARR];
  ub4 change[MAXARR];    /* do bits change? */
  ub4 same[MAXARR];      /* do bits stay the same? */
  tparam *p = t->param;
  for (j=0; j<ctrl->numword; ++j)
    change[j] = same[j] = ~(ub4)0;
  for (i=0; i<ctrl->numtest; ++i)
  {
    ctrl->first(i, t, ctrl, a);
    ctrl->delta(ctrl, p, a, t->delta, c);
    for (j=0; j<ctrl->numword; ++j)
    {
      change[j] &= ~c[j];
      same[j]   &=  c[j];
    }
  }
  for (cc=0, j=0/*ctrl->numword-1*/; j<ctrl->numword; ++j)
  {
    cs = change[j] | same[j];
    for (k=0; k<ctrl->numbits; ++k)
    {
      if (!bit(cs, (ub4)1<<k)) ++cc;
    }
  }
  if (p->good < -cc) p->good = -cc;
  if (p->good > ctrl->mylimit)
  {
    rsl = FALSE;
    if (ctrl->outfail)
    {
      if (-cc > ctrl->mylimit)
        (*ctrl->outfail)("change",change,t,ctrl);
      else if (-cs > ctrl->mylimit)
        (*ctrl->outfail)("same",same,t,ctrl);
    }
  }
  return rsl;
}


/*
 * Run ctrl->numtest trials, then count how many times each bit has 
 *   changed and not changed.
 * If any bit changes fewer than ctrl->mylimit times, report failure.
 * The avalanche test says that every input bit should cause every
 *   output bit to change half the time.  This routine allows you
 *   to show that every input bit causes every output bit to change
 *   with probability at least 1/4, 1/3, 1/2-delta ... depending on
 *   how many trials you run.  Unfortunately, since there are so many
 *   input/output bit pairs, you need to run a huge number of trials
 *   to show much of anything.
 */
word ttlea(t, ctrl)
tnode  *t;
tctrl  *ctrl;
{
  register ub4 i,j,k,rsl=TRUE;
  ub4 a[MAXARR];
  ub4 c[MAXARR];
  ub4 same[MAXARR][NUMBITS]; /* how many times did the nth bit not change */
  tparam *p = t->param;
  for (j=0; j<ctrl->numword; ++j)
    for (k=0; k<ctrl->numbits; ++k)
      same[j][k] = 0;
  for (i=0; i<ctrl->numtest; ++i)
  {
    ctrl->first(i, t, ctrl, a);
    ctrl->delta(ctrl, p, a, t->delta, c);
    for (j=0; j<ctrl->numword; ++j)
    {
      register temp = c[j];
      for (k=0; k<ctrl->numbits; ++k)
        if (bit(temp,(ub4)1<<k)) ++same[j][k];
    }
  }
  for (j=0/*ctrl->numword-1*/; j<ctrl->numword && rsl; ++j)
    for (k=0; k<ctrl->numbits && rsl; ++k)
    {
      register sb4 temp = same[j][k];
      if (temp > ctrl->numtest-temp) 
        temp = ctrl->numtest-temp;
      if (p->good < -temp)
        p->good = -temp;
      if (ctrl->mylimit < p->good)
      {
        rsl = FALSE;
        if (ctrl->outfail)
        {
          char str[30];
          sprintf(str,"j %d k %d temp %d ",j,k,temp);
          (*ctrl->outfail)(str,t->delta,t,ctrl);
        }
      }
    }
  return rsl;
}

#define BYTVALS 256
word ttbyt(t, ctrl)
tnode   *t;
tctrl   *ctrl;
{
  register sb4 i, j, k, bytes, par, rsl = TRUE;
  ub4 a[MAXARR];
  ub4 c[MAXARR];
  sb2 count[MAXARR*4][BYTVALS]; /* number of times each value was seen */
  tparam *p = t->param;
  bytes = ctrl->numword<<2;
  for (i=0; i<bytes; ++i)
    for (j=0; j<BYTVALS; ++j)
      count[i][j] = 0;
  for (i=0; i<ctrl->numtest; ++i)
  {
    ctrl->first(i, t, ctrl, a);
    ctrl->delta(ctrl, p, a, t->delta, c);
    for (j=0; j<bytes; ++j)
      ++count[j][((ub1 *)c)[j]];
  }
  par = ctrl->numtest/BYTVALS;
  for (i=0; i<bytes; ++i)
  {
    for (j=0; j<BYTVALS; ++j)
    {
      sb4 temp = count[i][j]-par;
      if (temp < 0) temp = -temp;
      if (p->good < temp)
        p->good = temp;
      if (ctrl->mylimit < temp)
      {
        rsl = FALSE;
        if (ctrl->outfail)
        {
          char str[30];
          sprintf(str,"i %ld j %ld count %d ",i,j,count[i][j]);
          (*ctrl->outfail)(str,t->delta,t,ctrl);
        }
      }
    }
  }
  return rsl;
}

/*
------------------------------------------------------------------------------
tp* - Given an input delta and a number, construct a new pair of inputs
------------------------------------------------------------------------------
*/

/* the input is the counter */
void tpcou(i, t, ctrl, a)
ub4    i;      /* which pair are we generating */
tnode *t;      /* don't overlap bits in the delta being tested */
tctrl *ctrl;   /* control structure */
ub4   *a;      /* where to put the value */
{
  ub4 k;
  for (k=0; k<ctrl->numword; ++k) a[k] = 0;
  a[t->clear] = i;
}


void tpexp(i, t, ctrl, a)
ub4    i;      /* which pair are we generating */
tnode *t;      /* don't overlap bits in the delta being tested */
tctrl *ctrl;   /* control structure */
ub4   *a;      /* where to put the value */
{
  ub4 j,k;
  for (k=1; k<ctrl->numword; ++k) a[k] = 0;
  j = i/ctrl->numbits;
  if (j > ctrl->numword)
  {
    printf("error!  tpexp overwriting memory!\n");
    return;
  }
  a[j] = (ub4)1<<(i%ctrl->numbits);
}

void tpran(i, t, ctrl, a)
ub4    i;      /* which pair are we generating */
tnode *t;      /* don't overlap bits in the delta being tested */
tctrl *ctrl;   /* control structure */
ub4   *a;      /* where to put the value */
{
  ub4 j;
  for (j=0; j<ctrl->numword; ++j)
    a[j] = rand(&ctrl->rctx);
}

/*
------------------------------------------------------------------------------
td* - Types of deltas
------------------------------------------------------------------------------
*/

/* b xor a = delta */
void tdxor(ctrl, p, a, delta, c)
tctrl  *ctrl;
tparam *p;
ub4    *a;
ub4    *delta;
ub4    *c;
{
  ub4 j;
  ub4 b[MAXARR];
  for (j=0; j<ctrl->numword; ++j)
    b[j] = delta[j] ^ a[j];
  (*p->fun)(a, p);
  (*p->fun)(b, p);
  for (j=0; j<ctrl->numword; ++j)
    c[j] = a[j] ^ b[j];
}

/* b xor ~a = delta */
void tdnxr(ctrl, p, a, delta, c)
tctrl  *ctrl;
tparam *p;
ub4    *a;
ub4    *delta;
ub4    *c;
{
  ub4 j;
  ub4 b[MAXARR];
  for (j=0; j<ctrl->numword; ++j)
    b[j] = ~(delta[j] ^ a[j]);
  (*p->fun)(a, p);
  (*p->fun)(b, p);
  for (j=0; j<ctrl->numword; ++j)
    c[j] = ~(a[j] ^ b[j]);
}

/* b - a = delta */
void tdsub(ctrl, p, a, delta, c)
tctrl  *ctrl;
tparam *p;
ub4    *a;
ub4    *delta;
ub4    *c;
{
  ub4 j;
  ub4 b[MAXARR];
  for (j=0; j<ctrl->numword; ++j)
    b[j] = a[j] + delta[j];
  (*p->fun)(a, p);
  (*p->fun)(b, p);
  for (j=0; j<ctrl->numword; ++j)
    c[j] = convdelt(b[j] - a[j]);
}

/* b + a = delta */
void tdadd(ctrl, p, a, delta, c)
tctrl  *ctrl;
tparam *p;
ub4    *a;
ub4    *delta;
ub4    *c;
{
  ub4 j;
  ub4 b[MAXARR];
  for (j=0; j<ctrl->numword; ++j)
    b[j] = delta[j] - a[j];
  (*p->fun)(a, p);
  (*p->fun)(b, p);
  for (j=0; j<ctrl->numword; ++j)
    c[j] = convdelt(a[j] + b[j]);
}

