/*
------------------------------------------------------------------------------
 By Bob Jenkins, 1989, in association with his thesis project
 Public Domain
------------------------------------------------------------------------------
*/
#ifndef STANDARD
#include "standard.h"
#endif
#ifndef DLLINK
#include "dllink.h"
#endif
#ifndef KNOT
#include "knot.h"
#endif
#ifndef ORDER
#include "order.h"
#endif

/* o_order2: order, look at neighbors and their neighbors */
static void   o_order2(/*_ crossing *k, word *oldorder, word *order, 
			 word crossings _*/);
/* o_order1: order, look at neighbors */
static void   o_order1(/*_ crossing *k, word *oldorder, word *order,
			 word crossings _*/);
/* o_add: move a crossing into the solved region */
static void   o_add(/*_ word *n, dllink **boundary, word *going_in, crossing *k,
		      word newcross, instruct *answer _*/);
/* o_delete :connect a pair of strings in the solved region */
static void   o_delete(/*_ word *n, dllink **boundary, word *going_in, 
			 instruct *answer, word *done, word i _*/);
/* o_one_make: make instructions to handle one crossing */
static void   o_one_make(/*_ word *n, dllink **boundary, word *going_in,
			   crossing *k, word which, instruct *answer _*/);
/* o_tabs: mark the neighbors and the neighbors of the neighbors */
#define o_tabs( k, t, big) \
if (1) \
{ \
  (t)[(k)->o->c] += (big); \
  (t)[(k)->o->a->c] += 2; \
  (t)[(k)->o->z->c] -= 1; \
  (t)[(k)->u->a->c] += 2; \
  (t)[(k)->u->z->c] -= 1; \
} else

static void o_order2(k, oldorder, order, crossings)
crossing  *k;
word      *oldorder;
word      *order;
word       crossings;
/*  This assumes that every crossing is used.
    This lists the crossings in an order my algorithm will hopefully like. */
{
  word       tab[MAXCROSS];
  word       j,
            *ip,
            *endp,
             tmp1,
             tmp2;
  crossing  *kp,
            *kp2;

  for (ip = tab, endp = tab+crossings; ip != endp;) *(ip++) = 0;
  *order = oldorder[crossings-1];
  for (ip = order+1, endp = order+crossings; ip < endp; ip++)
  {
    j       = *(ip-1);
    *ip     = j;
    tab[j] -= 100;
    kp2     = k+j;
    kp      = k+kp2->o->a->c;
    o_tabs(kp, tab, 16);
    kp = k+kp2->o->z->c;
    o_tabs(kp, tab, 20);
    kp = k+kp2->u->a->c;
    o_tabs(kp, tab, 16);
    kp = k+kp2->u->z->c;
    o_tabs(kp, tab, 20);
    tmp1 = *ip;
    for (j = 0; j < crossings; ++j)
    {
      tmp2 = oldorder[crossings-1-j];
      if (tab[tmp2] > tab[tmp1]) tmp1 = tmp2;
    }
    *ip = tmp1;
  }
}



static void       o_order1(k, oldorder, order, crossings)
crossing  *k;
word      *oldorder;
word      *order;
word       crossings;
/*  This assumes that every crossing is used.
    This lists the crossings in an order my algorithm will hopefully like. */
{
  word       tab[MAXCROSS];
  word       j,
            *ip,
            *endp,
             tmp1,
             tmp2;
  crossing  *kp2;

  for (ip = tab, endp = tab+crossings; ip != endp;) *(ip++) = 0;
  *order = oldorder[crossings-1];
  for (ip = order+1, endp = order+crossings; ip != endp; ip++)
  {
    j                  = *(ip-1);
    *ip                = j;
    tab[j]            -= 100;
    kp2                = k+j;
    tab[kp2->o->a->c] += 20;
    tab[kp2->o->z->c] += 20;
    tab[kp2->u->a->c] += 20;
    tab[kp2->u->z->c] += 20;
    tmp1 = *ip;
    for (j = 0; j < crossings; ++j)
    {
      tmp2 = oldorder[crossings-1-j];
      if (tab[tmp2] > tab[tmp1]) tmp1 = tmp2;
    }
    *ip = tmp1;
  }
}

/* Make instructions for just adding a crossing to the solved region */
static void       o_add(n, boundary, going_in, k, newcross, answer)
word      *n;
dllink   **boundary;
word      *going_in;
crossing  *k;
word       newcross;
instruct  *answer;
{
  word  i,
        old,
        a,
        b,
        c;

  answer->oldn = *n;
  /*-------------- Find *old*, try to make it an input rather than an output */
  /* Imagine crossing c, input 0 and output n attatched to c.  If you add the
     crossing at n, n becomes n+1 and you have input n and output n+2.  Then
     the pair of boundary crossings n+2 and 0 are removed, and string 0 becomes
     string n (really n-1 since things shift to fill in the missing 0).
     Changing a string from 0 to n-1 is likely to produce lots of bad
     crossings.  If you added the crossing at the input to begin with, the
     problem would have been avoided.  So try to add crossings at inputs. */
  for (i = *n; boundary[--i]->c != newcross;) ;
  old = i;
  if (!going_in[i])
  {
    while (i && (boundary[--i]->c != newcross || !going_in[i]));
    if (boundary[i]->c == newcross && going_in[i]) old = i;
  }
  answer->which = old;
  answer->over  = (k[newcross].o == boundary[old]);
  *n           += 2;
  for (i = *n; --i > old+2;)
  {
    boundary[i] = boundary[i-2];
    going_in[i] = going_in[i-2];
  }
  a               = going_in[old];
  b               = (k[newcross].hand == 1);
  c               = answer->over;
  going_in[old+1] = a;
  boundary[old+1] = (a ? boundary[old]->a : boundary[old]->z);
  a               = (a == b);
  if (a)
  {
    if (c)
    {
      boundary[old]   = k[newcross].u->z;
      going_in[old]   = 0;
      boundary[old+2] = k[newcross].u->a;
      going_in[old+2] = 1;
    }
    else
    {
      boundary[old]   = k[newcross].o->a;
      going_in[old]   = 1;
      boundary[old+2] = k[newcross].o->z;
      going_in[old+2] = 0;
    }
  }
  else
  {
    if (c)
    {
      boundary[old]   = k[newcross].u->a;
      going_in[old]   = 1;
      boundary[old+2] = k[newcross].u->z;
      going_in[old+2] = 0;
    }
    else
    {
      boundary[old]   = k[newcross].o->z;
      going_in[old]   = 0;
      boundary[old+2] = k[newcross].o->a;
      going_in[old+2] = 1;
    }
  }
  answer->prev       = going_in[old];
  answer->newn       = *n;
  answer->reductions = 0;
}


/* Make instructions for removing one pair of boundary crossings */
static void       o_delete(n, boundary, going_in, answer, done, i)
word      *n;
dllink   **boundary;
word      *going_in;
instruct  *answer;
word      *done,
           i;
{
  word    j;
  dllink *l,
         *m;

  j = (i == 0) ? *n-1 : i-1;
  l = (going_in[i]) ? boundary[i]->z : boundary[i]->a;
  m = (going_in[j]) ? boundary[j]->z : boundary[j]->a;
  if ((l == boundary[j]) && (m == boundary[i]))
  {
    *done                          = 0;
    answer->r0[answer->reductions] = i;
    answer->r1[answer->reductions] = j;
    if (i == 0) 
    {
      for (j = 0; j < (*n)-2; j++)
      {
        boundary[j] = boundary[j+1];
        going_in[j] = going_in[j+1];
      }
    }
    else 
    {
      for (j = i-1; j < (*n)-2; j++)
      {
        boundary[j] = boundary[j+2];
        going_in[j] = going_in[j+2];
      }
    }
    answer->reductions++;
    *n -= 2;
  }
}


/* make instructions for handling a single crossing */
static void       o_one_make(n, boundary, going_in, k, which, answer)
word      *n;
dllink   **boundary;
word      *going_in;
crossing  *k;
word       which;
instruct  *answer;
{
  word  i,
        done;

  /*  Move one crossing into the solved region */
  o_add(n, boundary, going_in, k, which, answer);

  /*  Join as many adjacent boundary pairs as possible */
  for (done = 0; !done;)
  {
    for (i = *n, done = 1; done && (--i >= 0);)
    {
      if (*n == 2) break;                        /* do not shrink to nothing */
      o_delete(n, boundary, going_in, answer, &done, i);
    }
  }
  answer->newn = *n;
}


/*  Make complete instructions for handling all the crossings */
void       o_make(k, crossings, list)
crossing  *k;
word       crossings;
instruct **list;
{
  word       order1[MAXCROSS];                         /* order of crossings */
  word       order2[MAXCROSS];                 /* another order of crossings */
  dllink    *boundary[BIGWEAVE];
  word       going_in[BIGWEAVE];
  word       i,
             n;
  instruct  *l;

  l = (instruct *)malloc(sizeof(instruct)*crossings);
  for (i = 0; i < crossings; order2[i] = i, i++) ;
  o_order2(k, order2, order1, crossings);
  o_order1(k, order1, order2, crossings);
  o_order1(k, order2, order1, crossings);
  boundary[0] = k[order1[0]].o;
  boundary[1] = k[order1[0]].o->z;
  going_in[0] = 1;
  going_in[1] = 0;
  n           = 2;
  printf("ordering: ");
  for (i = 0; i < crossings; i++) printf("%d ", order1[i]);
  printf("\n\n");
  for (i = 0; i < crossings; i++)
  {
    o_one_make(&n, boundary, going_in, k, order1[i], (l+i));
    if (n > 2*MAXSTRING)
    {
      printf("A cross section of %d is too big.\n",n);
      printf("Do you have the handednesses right?\n");
    }
  }
  *list = l;
}



void       o_show(l, crossings)
instruct  *l;
word       crossings;
{
  word  i;
  word  j;

  for (i = 0; i < crossings; i++)
  {
    printf("%d  %d %d  %d %d\n", l[i].which, l[i].prev,
           l[i].over, l[i].oldn, l[i].newn);
    for (j = 0; j < l[i].reductions; j++)
      printf("   %d %d %d\n", j, l[i].r0[j], l[i].r0[j]);
  }
}

