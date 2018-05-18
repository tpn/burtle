/*
------------------------------------------------------------------------------
  DLLINK.H
  Declares structures and procedures for the abstract data type DLLINK
  which is implemented in dllink.c .
  Bob Jenkins, 1990.  Public Domain.
------------------------------------------------------------------------------
*/
#ifndef STANDARD
# include "standard.h"
#endif
#ifndef  DLLINK 
#define  DLLINK 

/*  Something of type DLLINK is a node in a doubly-linked cyclic list.
    C is the data stored in this node.
    A is a pointer to the previous node.
    Z is a pointer to the next node. */

struct dllink
{
  word          c;
  struct dllink  *a;
  struct dllink  *z;
};
typedef struct dllink   dllink;


/* Procedures defined in dllink.c */

# define l_init( l) (l = ((dllink *)(0)))
void   l_show(/*_ dllink *l _*/);                       /* Display all nodes */
void   l_add(/*_ dllink *inp, word c, dllink **outp _*/);      /* Add a node */
void   l_del(/*_ dllink **l _*/);                           /* Delete a node */

#endif  /* ifndef DLLINK */
