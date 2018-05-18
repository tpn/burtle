/*
--------------------------------------------------------------------
By Bob Jenkins, 2002.  hicursor.h.  Public Domain.

This implements a hash table for 4-byte integers.  All access is through
cursors, which represent a current position in that table.
* Keys are unique.  Adding an item fails if the key is already there.
* Keys are copied.  Stuff is pointed at, not copied.
* Cursors are a position on a hash table that can be set and queried.
* The table length doubles dynamically and never shrinks.  The insert
  that causes table doubling may take a long time.
* The table length splits when the table length equals the number of items
* Computing a hash value takes 16 instructions.

  hicreate  - create a hash table
  hidestroy - destroy a hash table and all the cursors on it
   hicopy   - create a new cursor as a copy of an old cursor
   hifree   - free a cursor, plus the whole table if this is the last cursor
  hifirst   - move cursor to the first item in the table
  hinext    - move cursor to the next item in the table
  hiprev    - move cursor to the previous item in the table
  hilast    - move cursor to the last item in the table
   hifind   - find an item in the table
  hicount   - The number of items in the hash table
  hiccount  - The number of cursors the hash table
  hikey     - key at the current position
  histuff   - stuff at the current position
   histat   - print statistics about the table
  hiadd     - insert an item into the table
  hidel     - delete an item from the table
--------------------------------------------------------------------
*/

#ifndef STANDARD
#include "standard.h"
#endif

#ifndef HICURSOR
#define HICURSOR

/* PRIVATE TYPES AND FUNCTIONS */

/* private - hash table entry */
struct hint
{
  void        *stuff;    /* stuff stored in this hint */
  ub4          key;      /* key */
  struct hint *next;     /* next hint in list */
};
typedef  struct hint  hint;

/* private - hash table */
struct hitab
{
  struct hint  **table;   /* hash table, array of size 2^logsize */
  word           logsize; /* log of size of table */
  size_t         mask;    /* (hashval & mask) is position in table */
  ub4            count;   /* how many items in this hash table so far? */
  struct reroot *space;   /* space for the hints */
  ub4            bcount;  /* # hints useable in current block */
  ub4            count;   /* number of cursors using this table */
};
typedef  struct hitab  hitab;


/* hinbucket - PRIVATE - move to first item in the next nonempty bucket
  ARGUMENTS:
    t    - the hash table
  RETURNS:
    FALSE if the position wraps around to the beginning of the table
  NOTE:
    This is private to hicursor; do not use it externally.
 */
word hinbucket( hitab *t );


/* PUBLIC TYPES AND FUNCTIONS */

/* hicursor: a cursor on a hash table of integers */
struct hicursor
{
  hitab         *tab;     /* hash table this cursor is on */
  ub4            key;     /* current key */
  struct hint   *ipos;    /* current item in the array */
};
typedef  struct hicursor  hicursor;

/* hicreate - create a new hash table and a cursor on it
   ARGUMENTS:
     logsize - 1<<logsize will be the initial table length
   RETURNS:
     a new cursor
 */
hicursor *hicreate( word logsize);

/* hidestroy - destroy a hash table and all the cursors on it
   ARGUMENTS:
     c - a cursor on the table to be destroyed.
   RETURNS:
     nothing
 */
void  hidestroy( hicursor *c );

/* hicount, hiccount, hikey, histuff
   ARGUMENTS:
     c - a cursor on a hash table of integers
   RETURNS:
     hicount  - (ub4)    The number of items in the hash table
     hiccount - (ub4)    The number of cursors on the hash table
     hikey    - (ub4)    key for the current item
     histuff  - (void *) stuff for the current item
   NOTE:
     The current position always has an item as long as there
       are items in the table, so hexist can be used to test if the
       table is empty.
     hikey and histuff will crash if hicount returns 0
 */
#define hicount(c)  ((c)->tab->count)
#define hiccount(c) ((c)->tab->ccount)
#define hikey(c)    ((c)->ipos->key)
#define histuff(c)  ((c)->ipos->stuff)


/* hicopy - create a new cursor that is a copy of an old cursor
   ARGUMENTS:
     c    - a cursor on a hash table of integers
   RETURNS:
     another cursor on the same table pointing at the same place
 */
word  hicopy( hicursor *c );

/* hifree - free a cursor on a hash table of integers
   ARGUMENTS:
     c    - the cursor to be freed
   RETURNS:
     nothing
   NOTE:
     If this is the last cursor on the table, the table gets freed as well.
 */
word  hifree( hicursor *c );


/* hifind - move the current position to a given key
   ARGUMENTS:
     c    - a cursor on a hash table of integers
     key  - the key to look for
   RETURNS:
     TRUE if the item exists, FALSE if it does not.
     If the item exists, moves the current position to that item.
 */
word  hifind( hicursor *c, ub4 key );


/* hiadd - add a new item to the hash table
          change the position to point at the item with the key
   ARGUMENTS:
     c     - a cursor on a hash table of integers
     key   - the key to look for
     stuff - other stuff to be stored in this item
   RETURNS:
     FALSE if the operation fails (because that key is already there).
 */
word  hiadd( hicursor *c, ub4 key, void *stuff );


/* hidel - delete the item at the current position
          change the position to the following item
  ARGUMENTS:
    c    - a cursor on a hash table of integers
  RETURNS:
    FALSE if there is no current item (meaning the table is empty)
  NOTE:
    This frees the item, but not the key or stuff stored in the item.
    If you want these then deal with them first.  For example:
      if (hifind(tab, key, keyl))
      {
        free(histuff(tab));
        hidel(tab);
      }
 */
word  hidel( hicursor *c );


/* hifirst - move position to the first item in the table
  ARGUMENTS:
    c    - a cursor on a hash table of integers
  RETURNS:
    FALSE if there is no current item (meaning the table is empty)
  NOTE:
 */
word hifirst( hicursor *c );


/* hinext - move position to the next item in the table
   hiprev - move position to the previous item in the table
  ARGUMENTS:
    c    - a cursor on a hash table of integers
  RETURNS:
    FALSE if the position wraps around
  NOTE:
    To see every item in the table, do
      if (hifirst(c)) do
      {
        key   = hikey(c);
        stuff = histuff(c);
      }
      while (hinext(c));
 */
/* word hinext(hicursor *c); */
#define hinext(c) \
  ((!(c)->ipos) ? FALSE :  \
   ((c)->ipos=(c)->ipos->next) ? TRUE : hinbucket(c->tab))
word    hiprev(hicursor *c);

/* histat - print statistics about the hash table
  ARGUMENTS:
    c    - a cursor on a hash table of integers
  NOTE:
    items <0>:  <#buckets with zero items> buckets
    items <1>:  <#buckets with 1 item> buckets
    ...
    buckets: #buckets  items: #items  existing: x
    ( x is the average length of the list when you look for an
      item that exists.  When the item does not exists, the average
      length is #items/#buckets. )

    If you put n items into n buckets, expect 1/(n!)e buckets to
    have n items.  That is, .3678 0, .3678 1, .1839 2, ...
    Also expect "existing" to be about 1.5 for n items in n buckets.
 */
void histat( hicursor *c );

#endif   /* HICURSOR */
