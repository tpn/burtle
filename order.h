/*
------------------------------------------------------------------------------
  ORDER.H - Bob Jenkins, 1989, Public Domain
  Structures and routines used for choosing an order in which to add crossings
  to the solved region, plus code to make the instructions for how weaves have
  to change as those crossings are added.
------------------------------------------------------------------------------
*/

#ifndef STANDARD
#include "standard.h"
#endif
#ifndef BOUND
#include "bound.h"
#endif

#ifndef ORDER
#define ORDER

/*  Something of type INSTRUCT can describes manipulations of a weave boundary.
    There are two types of manipulations:  adding a crossing connected by one
    arc, which uses WHICH, PREV, and OVER; and removing a pair of adjacent
    boundary crossings, which requires REDUCTIONS and R.
    Something of type struct INSTRUCT can store one added crossing
        and 9 boundary removals. */

struct instruct
{
  word  which;     /* which arc to add the crossing at (-1 for just removal) */
  word  prev;                 /* is new previous boundary crossing an input? */
  word  over;                             /* is the old string the overpass? */
  word  oldn;          /* number of boundary crossings in the original weave */
  word  newn;             /* number of boundary crossings in the final weave */
  word  reductions;          /* number of boundary crossing pairs to connect */
  word  r0[MAXSTRING+1];               /* first boundary crossing to connect */
  word  r1[MAXSTRING+1];              /* second boundary crossing to connect */
};
typedef struct instruct   instruct;

/* Procedures defined in order.c */

void   o_make(/*_ struct crossing *k, word crossings, instruct **list _*/);
                                                        /* make instructions */
void   o_show(/*_ instruct *l, word crossings _*/);
                                                 /* display all instructions */


#endif  /* ifndef ORDER */
