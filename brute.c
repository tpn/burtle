/*
------------------------------------------------------------------------
Bob Jenkins, 1996, 2000, 2002, Public Domain

April 2002: Added heuristics to quit if the solution is not easy and 
retry later in the stream.  Breaks 4-bit rc4 with 2^^18 values pushed
(under a second on an 866mhz Pentium with gcc -O4), breaks 5-bit RC4
with about 2^^34 values pushed (about an hour on same).

September 2000: Rewrote, made "info" structure for state of the break,
support for searching forwards and backwards opportunistically.  Breaks
4-bit rc4 in about 36 seconds on a 166mhz Pentium, but would never finish
for full scale RC4. 

1996: code for breaking rc4.  Breaks 4-bit RC4 in 2 to 10 minutes on a
66mhz PC.  Doesn't work for full-scale RC4.

The driver is set up to have rc4 really produce some set of results, then
the recursive search tries to derive the internal state from those results.
------------------------------------------------------------------------
*/

#include <time.h>

#define ALPHA 5                                  /* number of bits per value */
#define SIZE  (1<<ALPHA)             /* number of terms in internal state m[]*/
#define MASK  (SIZE-1)                           /* (x+y)&MASK is (x+y)%SIZE */
#define TRUE  1
#define FALSE 0

typedef  unsigned long long  u8;

/*
------------------------------------------------------------------------
rc4, rc4b, rc4step, rc4bstep

These generate the results of rc4.
The rc4 algorithm starts with i=0 but first fills in r[1].
rc4step() and rc4bstep() are inverses, except that they produce the same
  result.  If you call rc4step() lots of times, it will eventually be
  overwriting a value of r[i] that was produced SIZE steps ago.  If you
  then call rc4bstep(), you don't reproduce the result that was produced
  SIZE steps ago, no, you reproduce the result that was just produced.
rc4() and rc4b() are inverses in the same way, except they assume s->i=0
and they produce SIZE results at a time.
------------------------------------------------------------------------
*/

/* internal state of rc4, required to produce the results */
struct rc4state
{
  int  i;        /* counter */
  int  j;        /* accumulator */
  int  m[SIZE];  /* memory */
  int  r[SIZE];  /* last SIZE results */
};
typedef  struct rc4state  rc4state;

/*
 * RC4 forward SIZE steps.
 * Fills all of r.  Beware, results are filled 1..SIZE-1,0.
 * Assumes (s->i == 0).
 * Inverse of rc4b (except for r).
 */
static void rc4( rc4state *s)
{
  int i,j,x,y,*m,*r;

  j = s->j;
  m = s->m;
  r = s->r;
  i=0;
  do
  {
    i=(i+1)&MASK;
    x = m[i];
    j = (x+j) & MASK;
    y = m[j];
    m[i] = y;
    m[j] = x;
    r[i] = m[(x+y) & MASK];
  }
  while (i != 0);

  s->j = j;
}

/*
 * RC4 forward one step.
 * Fills r[s->i+1].
 * Inverse of rc4bstep() (except for r)
 */
static void rc4step( rc4state *s)
{
  int i,j,x,y,*m;

  m = s->m;
  i = s->i = (s->i+1) & MASK; 
  x = m[i];
  j = (x + s->j) & MASK;
  s->j = j;
  y = m[j];
  m[i] = y;
  m[j] = x;
  s->r[i] = m[(x+y) & MASK];
}

/*
 * RC4 backwards SIZE steps.
 * Fills all of r.  Beware, fills 0,SIZE-1..1.
 * Assumes (s->i == 0).
 * Inverse of rc4 (except for r)
 */
static void rc4b( rc4state *s)
{
  int i,j,x,y,*m,*r;

  j = s->j;
  m = s->m;
  r = s->r;
  i=0;
  do
  {
    x=m[i];
    y=m[j];
    r[i] = m[(x+y)&MASK];
    m[i]=y;
    m[j]=x;
    j=(j-y)&MASK;
    i=(i-1)&MASK;
  }
  while (i != 0);
  s->j = j;
}

/*
 * Run rc4 backwards one step.
 * Fills r[i].
 * Inverse of rc4step (except for r)
 */
static void rc4bstep( rc4state *s)
{
  int i,j,x,y,*m;

  i = s->i;
  j = s->j;
  m = s->m;
  x = m[i];
  y = m[j];
  s->r[i] = m[(x+y) & MASK];
  m[i] = y;
  m[j] = x;
  j = (j-y) & MASK;
  s->i = (i-1) & MASK;
  s->j = j;
}

/* Copy the state of rc4 */
static void rc4copy( rc4state *outs, rc4state *ins)
{
  int i;
  outs->i = ins->i;
  outs->j = ins->j;
  for (i=0; i<SIZE; ++i)
  {
    outs->m[i] = ins->m[i];
    outs->r[i] = ins->r[i];
  }
}

/*
 * return FALSE if s1 and s2 are not the same 
 * s2 is allowed to be partially determined, with -1 for "don't care".
 */
static int rc4test( rc4state *s1, rc4state *s2)
{
  int i;
  if ((s1->i != s2->i) && (s2->i != -1))
    return FALSE;
  if ((s1->j != s2->j) && (s2->i != -1))
    return FALSE;
  for (i=0; i<SIZE; ++i)
    if ((s1->m[i] != s2->m[i]) && (s2->m[i] != -1))
      return FALSE;
  return TRUE;
}

/* initialize rc4 */
int rc4init( rc4state *s)
{
  int i;
  int r[SIZE];

  s->i = 0;
  s->j = 7;
  for (i=0; i<SIZE; ++i) 
  {
    s->m[i] = (3*i)&(SIZE-1);
    s->r[i] = -1;
  }
}

/* initialize rc4 */
int rc4show( rc4state *s)
{
  int i;
  int r[SIZE];

  printf("i %3d j %3d\n", s->i, s->j);
  printf("m\n");
  for (i=0; i<SIZE; ++i) 
  {
    printf("%3d ", s->m[i]);
    if ((i & 0xf) == 0xf) printf("\n");
  }
  printf("\nr\n");
  for (i=0; i<SIZE; ++i) 
  {
    printf("%3d ", s->r[i]);
    if ((i & 0xf) == 0xf) printf("\n");
  }
  printf("\n");
}


/*
------------------------------------------------------------------------
Tools for brute forcing rc4

This recursively searches for an internal state that produces a given
set of results.  The searching is done by recursive routines, like so:

  i = (i+1)&MASK;         -- done in choose_mi()
  x = m[i];               -- choose_mi() -> choose() -> choose_mj()
  j = (j+x)&MASK;         -- done in choose_mj()
  swap(m[i], m[j]);       -- done in choose_mj()
  y = m[i];               -- choose_mj() -> choose() -> choose_mr()
  r[i] = m[(x+y)&MASK];   -- filled/verified in choose_mr()
  ...                     -- choose_mr() -> choose_mi() ...

All these recursive routines do something, recurse, then undo whatever
it was they did.  This is faster than copying the whole internal state
every time we want to recurse.

Any success is detected in choose_mi().
------------------------------------------------------------------------
*/
struct rc4info
{
  /* rc4 internal state */
  rc4state state;

  /* additional indexes */
  int    pv[SIZE];      /* pv[i] is position if i is a chosen value, else -1 */
  int    nc;                                /* how many choices made so far? */
  int    v[SIZE];                             /* value chosen, indexed by nc */
  int    p[SIZE];                 /* position of chosen value, indexed by nc */
  int    high_j;                                /* j for the high end, or -1 */
  int    low_j;                                         /* j for the low end */
  int    ir;                                    /* index into the original r */
  int    nr;                               /* number of values in original r */

  /* plan of attack: these can wrap around */
  int    top;                    /* if info->state.i = top, go up no further */
  int    bottom;            /* if info->state.i = bottom, go down no further */
  int    high;         /* info->state.i = info->bottom+info.high at high end */
  int    low;            /* info->state.i = info->bottom+info.low at low end */

  int    success;
  int    iter;
  u8     work;
};
typedef  struct rc4info  rc4info;

static void choose_mi( rc4info *info);
static void choose_mj( rc4info *info);
static void choose_mr( rc4info *info);
static void choose_mb( rc4info *info);

/*
 * Initialize info used in the search.
 *   info - the structure to initialize
 *   r    - results 0..n-1
 *   n    - number of results
 *   i    - info->state.i = i before producing r[0].
 */
static void info_init( rc4info *info, int *r, int n, int i, u8 work)
{
  int  j,k;
  int *m  = info->state.m;
  int *sr = info->state.r;
  int *pv = info->pv;

  if (n > SIZE)
  {
    printf("can only match SIZE or fewer results\n");
    n = SIZE;
  }

  info->nc      = 0;
  info->state.i = i;
  info->state.j = 0;
  info->high_j  = 0;
  info->low_j   = 0;
  info->bottom  = i;
  info->top     = (i + n) & MASK;
  info->nr      = n;
  for (j=0; j<SIZE; ++j)
  {
    m [j] = -1;
    sr[j] = -1;
    pv[j] = -1;
  }
  for (j=0, k=i;  j<n;  ++j, k=(k+1)&MASK)
  {
    sr[k] = r[j];
  }
  info->success = 0;
  info->work = work;
}

static int *limit;

/* Push a choice: s->m[position] = value */
static void push_info( rc4info *info, int position, int value)
{
  info->state.m[position] = value;
  info->pv[value]   = position;
  info->v[info->nc] = value;
  info->p[info->nc] = position;
  ++info->nc;
  ++info->work;
}

/* Pop a choice */
static void pop_info( rc4info *info)
{
  int nc = --info->nc;
  int position = info->p[nc];
  int value    = info->v[nc];

  info->state.m[position] = -1;
  info->pv[value] = -1;
}

/* swap m[i] and m[j], even if one or both isn't known */
static void swap_info( rc4info *info)
{
  int *m = info->state.m;
  int  i = info->state.i;
  int  j = info->state.j;
  int  x = m[i];
  int  y = m[j];
  int *pv = info->pv;
  int  temp;

  m[i] = y;
  m[j] = x;
  if (x != -1) pv[x] = j;
  if (y != -1) pv[y] = i;
}

/* 
 * Go backwards to the edge.
 * Only call this between results, not in the middle of results.
 */
static void go_back( rc4info *info, int newir)
{
  int  i   = info->state.i;
  int  j   = info->state.j;
  int *m   = info->state.m;
  int *pv  = info->pv;
  int  low = info->low;
  int  ir  = info->ir;
  int  x;
  int  y;

  while (ir > newir)
  {
    x = m[j];
    y = m[i];
    m[i] = x;
    m[j] = y;
    if (x != -1) pv[x] = i;
    if (y != -1) pv[y] = j;
    if ((x == -1) || (y == -1)) 
      printf("x %3d y %3d i %3d j %3d high %3d low %3d ir %3d\n", 
	     x, y, i, j, info->high, info->low, ir);
    j = ((j - x) & MASK);
    i = ((i - 1) & MASK);
    --ir;
  }
  info->state.i = i;
  info->state.j = j;
  info->ir = ir;
}

/* 
 * Go forward to the edge.
 * Only call this between results, not in the middle of results.
 */
static void go_forth( rc4info *info, int newir)
{
  int  i    = info->state.i;
  int  j    = info->state.j;
  int  ir   = info->ir;
  int *m    = info->state.m;
  int *pv   = info->pv;
  int  high = info->high;
  int  x;
  int  y;

  while (ir < newir)
  {
    ++ir;
    i = ((i + 1) & MASK);
    x = m[i];
    j = ((j + x) & MASK);
    y = m[j];
    m[j] = x;
    m[i] = y;
    if (x != -1) pv[x] = j;
    if (y != -1) pv[y] = i;
  }
  info->ir = ir;
  info->state.i = i;
  info->state.j = j;
}


/* report success */
static void report_success( rc4info *info)
{
  int i;
  int ir = info->ir;
  go_back( info, 0);
  for (i=1; i<info->iter; ++i)
    rc4b( &info->state);

  printf("Found a match %3d:\n", info->ir);
  rc4show( &info->state);

  for (i=1; i<info->iter; ++i)
    rc4( &info->state);
  go_forth( info, ir);
  info->success = 1;
}


/*
 * Decide whether to forge ahead, or to attack in the other direction.
 * And then recursively do it.
 */
static void what_next( rc4info *info)
{
  int low  = info->low;
  int high = info->high;
  int i    = info->state.i;
  int old_high_j = info->high_j;
  int old_low_j  = info->low_j;
  int do_low;

  /* Decide whether to extend the low end or the high end */
  if (high == info->nr)
  {
    do_low = TRUE;
    if (low == 0)
    {
      report_success( info);                                     /* success! */
      return;
    }
  }
  else if (low == 0)
    do_low = FALSE;
  else
  {
    int  high_cost;
    int  low_cost;
    int  high_i = ((info->bottom + high + 1) & MASK);       /* i on high end */
    int  low_i = ((info->bottom + low) & MASK);              /* i on low end */
    int *m = info->state.m;
    int *pv = info->pv;

    /* record high_j or low_j if we can */
    if (info->state.i == low_i)
      info->low_j = info->state.j;
    else if (info->state.i == high_i)
    {
      if (m[high_i] == -1) 
	info->high_j = -1;
      else info->high_j = ((info->state.j + m[high_i]) & MASK);
    }

    high_cost = (m[high_i] == -1) +
                ((info->high_j == -1) || (m[info->high_j] == -1)) +
                (pv[info->state.r[high_i]] == -1);
    low_cost  = (m[low_i] == -1) + 
                (m[info->low_j] == -1) +
                (pv[info->state.r[low_i]] == -1);

    /* Decide which way to go based on the cost */
    if (high_cost < low_cost)
      do_low = FALSE;
    else if (low_cost < high_cost)
      do_low = TRUE;
    else
      do_low = (i == low_i);
  }

  /* Do it */
  if (do_low)
  {
    --info->low;
    if (info->ir != low)
    {
      int ir = info->ir;
      go_back( info, low);                                /* get in position */
      choose_mb( info);                                           /* recurse */
      go_forth( info, ir);                  /* get back to where you started */
    }
    else
    {
      choose_mb( info);                                      /* just recurse */
    }
    ++info->low;
  }
  else
  {
    ++info->high;
    if (info->ir != high)
    {
      int ir = info->ir;
      go_forth( info, high);                              /* get in position */
      choose_mi( info);                                           /* recurse */
      go_back( info, ir);                   /* get back to where you started */
    }
    else
    {
      choose_mi( info);                                      /* just recurse */
    }
    --info->high;
  }

  info->high_j = old_high_j;
  info->low_j  = old_low_j;
}

/* Try all possible choices for an unfilled position */
static void choose( rc4info *info, int position, void (*recurse)( rc4info *))
{
  int i;

  if (info->state.m[position] != -1)
  {
    (*recurse)( info);
  }
  else
  {
    /* heuristic: the real solution sometimes obeys this rule */
    if (info->nc - (info->high-info->low) > limit[info->nc])
      return;

    for (i=0; i<SIZE; ++i)
    {
      if (info->pv[i] != -1) continue;                     /* already taken? */
      push_info( info, position, i);         /* choose info->m[position] = i */
      (*recurse)( info);           /* go on to the next step, whatever it is */
      pop_info( info);                                    /* undo the choice */
    }
  }
}

/*
 * 
 * if (deep enough)
 *   report success
 * else
 *   i = (i + 1) & MASK;
 *   guess m[i];
 *   recursively call choose_mj()
 */
static void choose_mi(rc4info *info)
{
  rc4state s;
  int      i = info->state.i;

  /* increment i on the way in */
  i = (i+1)&MASK;
  info->state.i = i;
  ++info->ir;
  
  choose( info, i, choose_mj);
  
  /* decrement i on the way out */
  --info->ir;
  i = (i-1)&MASK;
  info->state.i = i;
}

/* 
 * x = m[i];                     -- must already be guessed
 * j = (j + x) & MASK;
 * swap(m[i], m[j])
 * guess m[i]                    -- providing r[i] has not already been placed
 * recursively call choose_mr()  -- or choose_mi if r[i] known to be handled
 */
static void choose_mj(rc4info *info)
{
  int  i  = info->state.i;
  int  j  = info->state.j;
  int *m  = info->state.m;
  int  ri = info->state.r[i];

  info->state.j = j = ((j + info->state.m[i]) & MASK);         /* find new j */
  swap_info( info);                                    /* swap m[i] and m[j] */

  if (m[i] == -1)                                /* do we know m[i] already? */
  {
    int *pv = info->pv;
    int  xy = pv[ri];
    if (xy == -1)
    {
      choose( info, i, choose_mr);            /* guess m[i] then fill m[x+y] */
    }
    else
    {                             /* r[i]=m[x+y] fixed x+y, so deduce y=m[i] */
      int x = m[j];
      int y = ((xy - x) & MASK);

      if (pv[y] == -1)
      {
	push_info( info, i, y);
	what_next( info);                        /* no need to verify m[x+y] */
	pop_info( info);
      }
      else
      {
        ;                    /* failure, y is already placed but not in m[i] */
      }
    }
  }
  else 
  {
    choose_mr( info);                                         /* fill m[x+y] */
  }

  swap_info( info);                                  /* unswap m[i] and m[j] */
  info->state.j = (j - info->state.m[i]) & MASK;               /* find old j */
}


/*
 * Fill m[x+y] with the value r[i], or verify that existing values match.
 * If (m[x+y]==r[i]) can be satisfied, recursively call choose_mi().
 */
static void choose_mr( rc4info *info)
{
  int  i  = info->state.i;
  int  j  = info->state.j;
  int *m  = info->state.m;
  int  ri = info->state.r[i];
  int  x  = m[j];                                  /* must already be chosen */
  int  y  = m[i];                                  /* must already be chosen */
  int  xy = ((x + y) & MASK);

  if ((x == -1) || (y == -1))
    printf("choose_mr: bad error x %3d y %3d\n", x, y);

  /* are we allowed to fill m[x+y] = ri? */
  if ((info->pv[ri] == -1) && (m[xy] == -1))
  {                                                    /* fill m[x+y] = r[i] */
    push_info( info, xy, ri);
    what_next( info);
    pop_info( info);
  }
  else
  {                                                 /* verify m[x+y] == r[i] */
    if (m[xy] == ri)
    {
      what_next( info);                            /* success, we lucked out */
    }
    else
    {
      return;                                                     /* failure */
    }
  }
}


/* 
 * This finds the internal state backwards, like so:
 * x = m[j];
 * y = m[i];
 * m[x+y] = r[i];
 * swap(m[i],m[j]);
 * j = j-x;
 * i = i-1;
 * call choose_mb() again
 */
static void choose_mb(rc4info *info)
{
  int  i  = info->state.i;
  int  j  = info->state.j;
  int *m  = info->state.m;
  int  ri = info->state.r[i];
  int *pv = info->pv;
  int  xy = pv[ri];
  int  x  = m[j];
  int  y  = m[i];
  
  if (xy == -1)                            /* Do we know (x+y)&MASK already? */
  {
    if (x == -1)
    {
      choose( info, j, choose_mb);                   /* guess m[j] try again */
    }
    else if (y == -1)
    {
      choose( info, i, choose_mb);                   /* guess m[i] try again */
    }
    else
    {
      xy = ((x + y)&MASK);
      if (m[xy] == -1)
      {                                               /* deduce m[xy] = r[i] */
	push_info( info, xy, ri);
	choose_mb( info);
	pop_info( info);
      }
    }
  }
  else if (y == -1)
  {                                        /* we know xy and we don't know y */
    if (x == -1)
    {                                                   /* don't know y or x */
      choose( info, j, choose_mb);                               /* choose x */
    }
    else
    {                                          /* we know xy and x, deduce y */
      y = ((xy - x) & MASK);
      
      if (pv[y] == -1)
      {
	push_info( info, i, y);
	choose_mb( info);                     /* all values chosen, clean up */
	pop_info( info);
      }
      else
      {
	return;               /* y is placed, but not at position i, failure */
      }
    }
  }
  else if (x == -1)
  {                                    /* we know xy and we know y, deduce x */
    x = ((xy - y) & MASK);
    
    if (pv[x] == -1)
    {
      push_info( info, j, x);
      choose_mb( info);                       /* all values chosen, clean up */
      pop_info( info);
    }
    else
    {
      return;                 /* x is placed, but not at position j, failure */
    }
  }
  else
  {                                                /* xy, x, y are all known */
    if (xy != ((x + y) & MASK))
    {
      return;                                                     /* failure */
    }
    else
    {                                                     /* looks OK so far */
      swap_info( info);                                /* swap m[i] and m[j] */
      info->state.j = ((j - x) & MASK);                        /* find old j */
      info->state.i = ((i - 1) & MASK);                        /* find old i */
      --info->ir;
      
      what_next( info);                                           /* recurse */
      
      ++info->ir;
      info->state.i = i;                                    /* restore new i */
      info->state.j = j;                                    /* restore new j */
      swap_info( info);                              /* unswap m[i] and m[j] */
    }
  }
}



/*
 * Given results r[0..n-1],
 * and that r[0] was produced by a state starting at s->i = i,
 * try to find a state that produces those results and generate it backwards.
 */
int find_dynamic( int *r, int n, int i, int iter, u8 *work)
{
  int j;
  rc4info info;

  info_init( &info, r, n, i, *work);

  info.ir = info.high = info.low = n/2;

  info.state.i = ((info.bottom + info.ir) & MASK);
  info.iter = iter;
  for (j=0; j<SIZE; ++j)
  {
    info.state.j = info.high_j = info.low_j = j;
    what_next( &info);
  }
  *work = info.work;
  return info.success;
}


/*
------------------------------------------------------------------------
Executables, make use of the tools
------------------------------------------------------------------------
*/

static void driver()
{
  long int  i, j;                                                 /* counter */
  rc4info   info;                                          /* info on attack */
  rc4state  s;                                      /* internal state of rc4 */
  int       done = 0;
  u8        work = 0;

  /* Initialize s->r */
  rc4init( &s);
  rc4( &s);

  /* Print out the goal */
  printf("Original results and the internal state before generating them:\n");
  rc4( &s);
  rc4b( &s);
  rc4show( &s);
  printf("\n");

  /* find a matching state */
  printf("Starting search: any matches will be printed out\n");
  for (i=1; !done; ++i)
  {
    done = find_dynamic( s.r, SIZE, 0, i, &work);
    rc4( &s);
    if (!(i & (i-1))) 
      printf("iter %5d  choices %.8lx%.8lx\n", i, (int)(work>>32), (int)work);
  }
  printf("End: iter %5d choices %.8lx%.8lx\n", i, (int)(work>>32), (int)work);
}

static int lim4[] = {-1,0,1,1,1,1,1,1,2,2,2,2,2,2,2,2};
static int lim5a[] = { /* average iter 50000 choices 2^^33.2 */
  -1, 0, 0, 0, 0, 1, 1, 1,   2, 2, 2, 3, 3, 3, 4, 4,
   4, 5, 5, 5, 6, 6, 6, 6,   7, 7, 7, 7, 8, 8, 8, 8
};
static int lim5b[] = { /* average iter 1650  choices 2^^36.3 */
  -1, 0, 1, 1, 1, 2, 2, 2,   2, 3, 3, 3, 4, 4, 4, 4,
   5, 5, 6, 6, 6, 6, 6, 6,   6, 6, 6, 6, 6, 6, 6, 6
};
static int lim6[] = {
  -1, 0, 1, 2, 2, 3, 4, 4,   5, 5, 6, 6, 7, 7, 8, 8,
   9, 9, 9,10,10,10,11,11,  11,12,12,12,13,13,13,13,
  14,14,14,14,14,14,14,14,  14,14,14,14,14,14,14,14,
  14,14,14,14,14,14,14,14,  14,14,14,14,14,14,14,14
};
static int lim7[] = {
  -1, 0, 1, 2, 2, 3, 4, 4,   5, 6, 6, 7, 8, 8, 9,10,
  10,11,12,12,13,14,14,15,  16,16,17,18,18,19,20,20,
  21,21,22,22,23,23,24,24,  25,25,26,26,27,27,28,28,
  29,29,30,30,30,30,30,30,  30,30,30,30,30,30,30,30,
  30,30,30,30,30,30,30,30,  30,30,30,30,30,30,30,30,
  30,30,30,30,30,30,30,30,  30,30,30,30,30,30,30,30,
  30,30,30,30,30,30,30,30,  30,30,30,30,30,30,30,30,
  30,30,30,30,30,30,30,30,  30,30,30,30,30,30,30,30
};
static int lim8[] = {
  -1, 0, 1, 2, 2, 3, 4, 4,   5, 6, 6, 7, 8, 8, 9,10,
  10,11,12,12,13,14,14,15,  16,16,17,18,18,19,20,20,
  21,22,22,23,24,24,25,25,  26,27,27,28,29,29,30,30,
  31,31,32,33,33,34,34,35,  35,36,36,37,37,38,38,39,
  39,40,40,40,41,41,42,42,  43,43,44,44,45,45,46,46,
  47,47,48,48,48,49,49,50,  50,51,51,51,52,52,52,53,
  53,53,54,54,54,55,55,55,  56,56,56,57,57,57,58,58,
  58,59,59,59,60,60,60,61,  61,61,61,62,62,62,62,62,

  63,63,63,63,63,63,63,63,  63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,  63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,  63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,  63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,  63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,  63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,  63,63,63,63,63,63,63,63,
  63,63,63,63,63,63,63,63,  63,63,63,63,63,63,63,63
};

int main()
{
  time_t s, t;

  limit = ((ALPHA < 5) ? lim4 :
	   (ALPHA == 5) ? lim5a : 
	   (ALPHA == 6) ? lim6 :
	   (ALPHA == 7) ? lim7 :
	   lim8);

  /* do the work and report how long it took */
  time(&s);
  driver();
  time(&t);
  printf("time %ld\n", t-s);
}

