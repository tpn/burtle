/*
--------------------------------------------------------------------
By Bob Jenkins, 2002.  hicursor.c.  Public Domain.

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
#include "hicursor.h"
#endif
#ifndef RECYCLE
#include "recycle.h"
#endif

/* Every byte is a random ordering of 0..255.  Only the top byte has to be. */
const static int hivect[] = {
  0x836c5504,0x3598fdc7,0x5c531ae9,0xab5633f1,
  0xa82aa678,0xf3b5e9f0,0xd17f5a31,0x810e566f,
  0xa6e7c8cb,0x8ef71460,0xd534d444,0xf151ac9c,
  0x447b8b4f,0xaf7d24e7,0x94927f11,0x64c29810,
  0xa11583c9,0x265919e1,0xa224796b,0x3487801b,
  0x9bf20aaf,0x541ac1e0,0x92fc78ec,0x51bc999b,
  0xe9aaf57e,0x60cea9ab,0xb2f4c47f,0xfb9ee065,
  0x7c6a920a,0x84dd6e08,0xce1f04b4,0x864e0d2a,
  0x68bad3e5,0x5a12aed6,0xb6b238f9,0xc6fe47fc,
  0xfe865957,0xbf0574c8,0xc51b34ba,0xd0186a1a,
  0x4cc5cdd3,0x4d7cc622,0x7f199b89,0xf4a3e47a,
  0x2b800275,0x46a023d8,0xeb339125,0x072b5198,
  0x0d62cfef,0xd4957601,0xe589682d,0xa508e30d,
  0xcfd61b6d,0xaa61d162,0x1643a795,0x383e43de,
  0xc28db68b,0xf69996d4,0xe293e674,0x55651da5,
  0x242f8ad0,0xe1eafec2,0xdcb4cc4a,0xbe91d880,
  0xa3755fd5,0xc9c98dc0,0x80394e07,0xb1d3d7d1,
  0x0549c07b,0xf75d6de2,0x9f117702,0x008cad68,
  0x9cec5be8,0xb0f55ec4,0xb9280505,0xfd6064b6,
  0xed70b214,0xf5479e5e,0x09abfcb3,0xf96f3ba2,
  0x70e47d51,0x56a9c99a,0x0e375283,0x12f94826,
  0x79efba33,0x087ad98c,0x10066c2c,0xf016bbca,
  0xec07bca1,0x63f1b1c5,0xc8a4268d,0x98046273,
  0x400f1caa,0xcd9d7e56,0x8869eda9,0x2dcca824,
  0x1e5c329d,0xe8bf9596,0x306e90a3,0x4b5a930e,
  0xba0cb53d,0xd9a84a41,0xd2214cf5,0x3f3fe890,
  0x8f4bdc2e,0x78dfb470,0x1431e23b,0xb4a52b36,
  0x739b9acd,0xefb3e78f,0x5b3c8ec6,0x7e42ec18,
  0x06fd41f3,0x425f4f2b,0x27d7a266,0x75e9639f,
  0x1720f25f,0xead42f55,0x0fa76015,0x01015797,
  0x580213c1,0x0bc71e23,0xa9deda13,0x0c0a3a88,
  0x9667c267,0x48906b42,0x5e2c3743,0x62ad390c,
  0xc78eb75c,0xc1d0153a,0xac5eef61,0x8daeb9f7,
  0x204dab54,0x7b0d94e3,0x49558671,0x59f8f452,
  0x9abd6fb8,0xdde28917,0x21db69eb,0x11dc36fd,
  0x31fb546a,0x934a3dc3,0x1857dedc,0x7473dbed,
  0x22263077,0x872e851e,0xdba1a348,0xd8e0229e,
  0xb554cbcf,0xae25ebbf,0x6d1d4dbd,0x77f6e1ae,
  0xb8e3f959,0x03e865a4,0xfca642da,0xa78b585b,
  0x3bfa7006,0x6c327b21,0xccc30127,0xbc00751d,
  0xdf9f9dbc,0xd7275d46,0x37d5c35a,0x904fd2e4,
  0xfaca50fe,0x04ed9fb0,0x2e633c5d,0xe7cf1fee,
  0x5040f329,0x6772bd69,0x2fb82a35,0x479ab045,
  0x716dcabb,0xee79f186,0xb7f317db,0x45d28c7c,
  0xa41467d2,0x69f0f8f6,0x1aac27a6,0x9d448730,
  0x61a20c19,0x2a663f53,0xca3ad5a0,0xadc0494b,
  0x13b1df58,0x97e52163,0x4164ddd9,0x0a76286c,
  0x9552b8d7,0x7d4616b5,0x395ba079,0xe6cd25f8,
  0xc0b7f7be,0xcb7e73f2,0xbdb9110f,0x3d7882ad,
  0xff1335b1,0x65234491,0x1f29eee6,0xe41ec76e,
  0x28bbfaa7,0x6ee1d68a,0x825008dd,0x3a30317d,
  0x9e8fea0b,0x361c5349,0x6fe69c03,0x91710f92,
  0xdebe8116,0xf83545b7,0xda38a4df,0x253d6134,
  0x1d2dbfcc,0x5f48183e,0x02ee4b85,0x3ed9f650,
  0xf2178fac,0xbbebff00,0x6bc81293,0x85844637,
  0x4a3b883f,0x1bcbd032,0xd3d8f01f,0x4e81fb09,
  0x99740081,0x2c830982,0x53360376,0x8c45b340,
  0x438a2087,0xc3960e4d,0x32687138,0xa06b2964,
  0x6a9c7ab2,0x8a77e520,0x23d1972f,0x89c6af39,
  0x3c09aaa8,0x52972e4e,0x29c46628,0x1cb0724c,
  0xc4887c1c,0x4f10a5f4,0x15852c99,0x8b035cfa,
  0x5daf3eea,0x1982073c,0x3341a194,0xd64c408e,
  0x72c106b9,0x7658c5ce,0x66b6ce72,0x7aff1084,
  0xe30b84fb,0xe0dabe12,0xb3942dff,0x57220b47
};

/* Hash function.  No known particularly bad inputs. */
static ub4 hihash(ub4 key)
{
  ub4 x = key;
  x = hivect[x&0xff]^(x>>8);
  x = hivect[x&0xff]^(x>>8);
  x = hivect[x&0xff]^(x>>8);
  x = hivect[x&0xff]^(x>>8);
  return x;
}

/* sanity check -- make sure ipos, apos, and count make sense */
static void  hisanity(hitab *t)
{
  ub4    i, end, counter;
  hint  *h;

  /* test that apos makes sense */
  end = (ub4)1<<(t->logsize);
  if (end < t->apos)
    printf("error:  end %ld  apos %ld\n", end, t->apos);

  /* test that ipos is in bucket apos */
  if (t->ipos) {
    for (h=t->table[t->apos];  h && h != t->ipos;  h = h->next)
      ;
    if (h != t->ipos)
      printf("error:ipos not in apos, apos is %ld\n", t->apos);
  }

  /* test that t->count is the number of elements in the table */
  counter=0;
  for (counter=0, i=0;  i<end;  ++i)
    for (h=t->table[i];  h;  h=h->next)
      ++counter;
  if (counter != t->count)
    printf("error: counter %ld  t->count %ld\n", counter, t->count);
}


/*
 * higrow - Double the size of a hash table.
 * Allocate a new, 2x bigger array,
 * move everything from the old array to the new array,
 * then free the old array.
 */
static void higrow(hitab *t)
{
  register ub4    newsize = (ub4)1<<(++t->logsize);
  register ub4    newmask = newsize-1;
  register ub4    i;
  register hint **oldtab = t->table;
  register hint **newtab = (hint **)malloc(newsize*sizeof(hint *));

  /* make sure newtab is cleared */
  for (i=0; i<newsize; ++i)
    newtab[i] = (hint *)0;
  t->table = newtab;
  t->mask = newmask;

  /* Walk through old table putting entries in new table */
  for (i=newsize>>1; i--;) {
    register hint *this, *that, **newplace;
    for (this = oldtab[i]; this;) {
      that = this;
      this = this->next;
      newplace = &newtab[(hihash(that->key) & newmask)];
      that->next = *newplace;
      *newplace = that;
    }
  }

  /* position the hash table on some existing item */
  hifirst(t);

  /* free the old array */
  free((char *)oldtab);

}

/* hicreate - create a new hash table of integers */
hicursor *hicreate(word logsize)  /* log base 2 of initial hash table size */
{
  ub4       i,len = ((ub4)1<<logsize);
  hitab    *t = (hitab *)malloc(sizeof(hitab));
  hicursor *cursor;

  t->table = (hint **)malloc(sizeof(hint *)*(ub4)len);
  for (i=0; i<len; ++i) t->table[i] = (hint *)0;
  t->logsize = logsize;
  t->mask = len-1;
  t->count = 0;
  t->apos = (ub4)0;
  t->ipos = (hint *)0;
  t->bcount = 0;
  t->space = remkroot(max(sizeof(hint),sizeof(hicursor)));
  cursor = (hicursor *)renew(t->space);
  t->ccount = 1;
  return t;
}

/* hidestroy - destroy a hash table and all its cursors, free all memory*/
void hidestroy(hicursor *c)
{
  hitab *t = c->tab;
  refree(t->space);
  free((char *)t->table);
  free((char *)t);
}

/* hicount() is a macro, see hicursor.h */
/* hikey() is a macro, see hicursor.h */
/* histuff() is a macro, see hicursor.h */

/* hifind - find an item with a given key in a hash table */
word   hifind(hitab *t, ub4 key )
{
  hint *h;
  ub4    x = hihash(key);
  ub4    y;
  for (h = t->table[y=(x&t->mask)]; h; h = h->next) {
    if (key == h->key) {
      t->apos = y;
      t->ipos = h;
      return TRUE;
    }
  }
  return FALSE;
}

/* hicopy - make a copy of an existing cursor */
hicursor *hicopy(hicursor *c)
{
  hitab    *t = c->tab;
  hicursor *c2 = (hicursor *)renew(t->space);
  *c2 = *c;
  ++t->ccount;
  return c2;
}

/* hifree - free a cursor, plus the whole table if this is the last cursor */
void hifree(hicursor *c)
{
  hitab    *t = c->tab;
  --t->ccount;
  if (!t->ccount)
    hidestroy(c);
  else
    redel(t->space, c);
}

/*
 * hiadd - add an item to a hash table.
 * return FALSE if the key is already there, otherwise TRUE.
 */
word hiadd(hicursor *c, ub4 key, void *stuff)
{
  register hint  *h,**hp;
  register ub4    y, x = hihash(key);

  /* make sure the key is not already there */
  for (h = t->table[(y=(x&t->mask))]; h; h = h->next) {
    if (key == h->key) {
      t->apos = y;
      t->ipos = h;
      return FALSE;
    }
  }

  /* find space for a new item */
  h = (hint *)renew(t->space);

  /* make the hash table bigger if it is getting full */
  if (++t->count > (ub4)1<<(t->logsize)) {
    higrow(t);
    y = (x&t->mask);
  }

  /* add the new key to the table */
  h->key   = key;
  h->stuff = stuff;
  hp = &t->table[y];
  h->next = *hp;
  *hp = h;
  t->ipos = h;
  t->apos = y;

#ifdef HSANITY
  hisanity(t);
#endif  /* HSANITY */

  return TRUE;
}

/* hdel - delete the item at the current position */
word  hidel(hitab *t)
{
  hint  *h;    /* item being deleted */
  hint **ip;   /* a counter */

  /* check for item not existing */
  if (!(h = t->ipos))
    return FALSE;

  /* remove item from its list */
  for (ip = &t->table[t->apos]; *ip != h; ip = &(*ip)->next)
    ;
  *ip = (*ip)->next;
  --(t->count);

  /* adjust position to something that exists */
  if (!(t->ipos = h->next))
    hinbucket(t);

  /* recycle the deleted hint node */
  redel(t->space, h);

#ifdef HSANITY
  hsanity(t);
#endif  /* HSANITY */

  return TRUE;
}

/* hifirst - position on the first element in the table */
word hifirst(hitab *t)
{
  t->apos = t->mask;
  (void)hinbucket(t);
  return (t->ipos != (hint *)0);
}

/* hinext() is a macro, see hicursor.h */

/*
 * hinbucket - Move position to the first item in the next bucket.
 * Return TRUE if we did not wrap around to the beginning of the table
 */
word hinbucket(hitab *t)
{
  ub4  oldapos = t->apos;
  ub4  end = (ub4)1<<(t->logsize);
  ub4  i;

  /* see if the element can be found without wrapping around */
  for (i=oldapos+1; i<end; ++i) {
    if (t->table[i&t->mask]) {
      t->apos = i;
      t->ipos = t->table[i];
      return TRUE;
    }
  }

  /* must have to wrap around to find the last element */
  for (i=0; i<=oldapos; ++i) {
    if (t->table[i]) {
      t->apos = i;
      t->ipos = t->table[i];
      return FALSE;
    }
  }

  return FALSE;
}

void histat(hitab *t)
{
  ub4    i,j;
  double total = 0.0; /* higgledy piggledy, thisaway, thataway */
  hint  *h;
  hint  *walk, *walk2, *stat = (hint *)renew(t->space);

  /* in stat, key stores the number of buckets */
  for (i=0; i<=t->mask; ++i) {
    for (h=t->table[i], walk=stat; h; h=h->next, walk=walk->next)
      if (!walk->next)
	walk->next = (hint *)renew(t->space);
    ++walk->key;
  }

  /* figure out average list length for existing elements */
  for (i=0, walk=stat; walk; ++i, walk=walk->next) {
    total += (double)walk->key*(double)((i*(i+1))/2);
  }
  if (t->count)
    total /= (double)t->count;
  else 
    total  = (double)0;

  /* print statistics */
  printf("\n");
  for (i=0, walk=stat; walk; ++i, walk=walk->next)
    printf("items %ld:  %ld buckets\n", i, walk->key);
  printf("\nbuckets: %ld  items: %ld  existing: %g\n\n",
         ((ub4)1<<t->logsize), t->count, total);

  /* clean up */
  while (stat) {
    walk = stat->next;
    redel(t->space, stat);
    stat = walk;
  }
}
