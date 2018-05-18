/*
-------------------------------------------------------------------------------
By Bob Jenkins, March 2003.  Public domain.

jenny.c -- jennyrate tests from m vectors of features that cover all n-tuples
  of features, n<m, with each feature chosen from a different vector.  For
  example, given 10 vectors (m=10) with 2 to 8 features apiece, cover all 
  feature triplets (n=3).  A lower bound on the number of tests required is
  the product of the sizes of the largest n vectors.  Already-written tests
  can be piped in to be reused.

Arguments
  Arguments without a leading '-' : an integer in 2..52.  Represents a vector.
  Vectors are implicitly numbered 1..65535, in the order they appear.
  Features in vectors are always implicitly given 1-character names, which
  are in order a..z, A..Z .  It's a good idea
  to pass the output of jenny through a postprocessor that expands these names
  into something intelligible.

  -o : old.  -ofoo.txt reads existing tests from the file foo.txt, includes
       those tests in the output, and adds whatever other tests are needed to
       complete coverage.  An error is reported if the input tests are of the
       wrong shape or contain disallowed feature interactions.

  -h : help.  Print out instructions for using jenny.

  -n : an integer.  Cover all n-tuples of features, one from each vector.
       Default is 2 (pairs).  3 (triplets) may be reasonable.  4 (quadruplets)
       is definitely overkill.  n > 4 is highly discouraged.

  -s : seed.  An integer.  Seed the random number generator.

  -w : without this combination of features.  A feature is given by a vector
       number followed by a one-character feature name.  A single -w can
       disallow multiple features in a vector.  For example, -w1a2cd4ac
       disallows the combinations (1a,2c,4a),(1a,2c,4c),(1a,2d,4a),(1a,2d,4c)
       where 1a represents the first vector's first feature, 2c is the second
       vector's third feature, and 4a is the fourth vector's first feature.

Example: 10 vectors of 2, 3, 8, 3, 2, 2, 5, 3, 2, 2 features apiece,
with some restrictions, asking for all triplets of features to be covered.
This will produce at least 8*5*3=120 tests.  Splitting the 8 features in the
third vector into three vectors each of length 2 would reduce the number of
tests required to at least 5*3*3=45.

  jenny -n3 2 3 8 -w1a2bc3b -w1b3a 3 -w1a4b 2 2 5 3 2 2 -w9a10b -w3a4b -s3
-------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stddef.h>

char *bob_malloc( size_t len)
{
  return (char *)malloc(len);
}

void bob_free( char *x)
{
  free(x);
}

/*
-------------------------------------------------------------------------------
Implementation:

Internally, there can be 64K vectors with 64K features apiece.  Externally,
the number of features per vector is limited to just 52, and are implicitly
named a..z, A..Z.  Other printable characters, like |, caused trouble in the
shell when I tried to give them during a without.

The program first finds tests for all features, then adds tests to cover
all pairs of features, then all triples of features, and so forth up to
the tuples size the user asked for.
-------------------------------------------------------------------------------
*/

/*
-------------------------------------------------------------------------------
Structures
-------------------------------------------------------------------------------
*/

typedef  unsigned char      ub1;
typedef  unsigned short     ub2;
typedef  signed   short     sb2;
typedef  unsigned long      ub4;
typedef  signed   long      sb4;
typedef  unsigned long long ub8;
typedef  signed   long long sb8;
#define TRUE  1
#define FALSE 0
#define UB4MAXVAL 0xffffffff
#define UB2MAXVAL 0xffff

/*
-------------------------------------------------------------------------------
Random number stuff
-------------------------------------------------------------------------------
*/

#define RAND_SIZE 256
typedef  struct randctx {
  ub4 b,c,d,z;                                             /* special memory */
  ub4 m[RAND_SIZE];                             /* big random pool of memory */
  ub4 r[RAND_SIZE];                                               /* results */
  ub4 q;                     /* counter, which result of r was last reported */
} randctx;

/* Pseudorandom numbers, courtesy of FLEA */
void rand_batch( randctx *x) {
  ub4 a, b=x->b, c=x->c+(++x->z), d=x->d, i, *m=x->m, *r=x->r;
  for (i=0; i<RAND_SIZE; ++i) {
    a = m[b % RAND_SIZE];
    m[b % RAND_SIZE] = d;
    d = (c<<19) + (c>>13) + b;
    c = b ^ m[i];
    b = a + d;
    r[i] = c;
  }
  x->b=b; x->c=c; x->d=d;
}

ub4 rand( randctx *x) {
  if (!x->q--) {
    x->q = RAND_SIZE-1;
    rand_batch(x);
  }
  return x->r[x->q];
}

void rand_init( randctx *x, ub4 seed) {
  ub4    i;

  x->b = x->c = x->d = x->z = seed;
  for (i = 0; i<RAND_SIZE; ++i) {
    x->m[i] = seed;
  }
  for (i=0; i<10; ++i) {
    rand_batch(x);
  }
  x->q = 0;
}




/*
------------------------------------------------------------------------------
Other helper routines
------------------------------------------------------------------------------
*/

#define TUPLE_ARRAY  0x1000             /* number of tuples in a tuple array */

/* An arbitrary feature, prefix fe */
typedef  struct feature {
  ub2    v;                                                   /* Vector name */
  ub2    f;                                                  /* Feature name */
} feature;

/* a tuple array, prefix tu */
typedef  struct tu_arr {
  struct tu_arr  *next;                                  /* next tuple array */
  ub2             len;                               /* length of this array */
  feature         fe[TUPLE_ARRAY];                        /* array of tuples */
} tu_arr;

/* an iterator over a tuple array, prefix tu */
typedef  struct tu_iter {
  struct tu_arr  *tu;                                    /* next tuple array */
  ub2             offset;                         /* offset of current tuple */
  ub2             n;                         /* number of features per tuple */
  feature        *fe;                                       /* current tuple */
} tu_iter;

/* names of features, for output */
static const char feature_name[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* insert a tuple into a tuple array */
int insert_tuple( tu_arr **tu, ub2 tuple_length, feature *tuple)
{
  ub4      i;
  feature *fe;
  while (*tu && (*tu)->len == (TUPLE_ARRAY/tuple_length)) {
    tu = &((*tu)->next);
  }
  if (!*tu) {
    if (!(*tu = (tu_arr *)bob_malloc(sizeof(tu_arr)))) {
      return FALSE;
    }
    (*tu)->len = 0;
    (*tu)->next = (tu_arr *)0;
  }
  fe = &(*tu)->fe[(*tu)->len*tuple_length];
  for (i=0; i<tuple_length; ++i) {
    fe[i].v = tuple[i].v;
    fe[i].f = tuple[i].f;
  }
  ++(*tu)->len;
}

/* print out a single tuple */
void show_tuple( feature *fe, ub2 len)
{
  ub4 i;
  for (i=0; i<len; ++i) {
    printf(" %d%c", fe[i].v+1, feature_name[fe[i].f]);
  }
  printf(" \n");
}

/* delete a tuple from a tuple array */
feature *delete_tuple( tu_arr **tu, ub2 tuple_length, feature *tuple)
{
  tu_arr **tu2;
  feature *fe;
  ub4      i;

  for (tu2 = tu; (*tu2)->next; tu2 = &(*tu2)->next)
    ;
  --(*tu2)->len;
  fe = &(*tu2)->fe[(*tu2)->len*tuple_length];
  for (i=0; i<tuple_length; ++i) {
    tuple[i].v = fe[i].v;
    tuple[i].f = fe[i].f;
  }
  if (!(*tu2)->len) {
    bob_free((char *)*tu2);
    *tu2 = (tu_arr *)0;
  }

  if (tuple == fe) {
    return (feature *)0;
  } else {
    return tuple;
  }
}

/* start a tuple iterator */
feature *start_tuple( tu_iter *ctx, tu_arr *tu, ub4 n)
{
  ctx->tu = tu;
  ctx->offset = 0;
  ctx->n = n;
  if (tu) {
    ctx->fe = tu->fe;
  } else {
    ctx->fe = (feature *)0;
  }
  return ctx->fe;
}

/* get the next tuple from a tuple iterator (0 if no more tuples) */
feature *next_tuple( tu_iter *ctx)
{
  if (ctx->fe) {
    if (++ctx->offset < ctx->tu->len) {
      ctx->fe += ctx->n;
    } else {
      ctx->tu = ctx->tu->next;
      ctx->offset = 0;
      if (ctx->tu && ctx->tu->len) {
	ctx->fe = ctx->tu->fe;
      } else {
	ctx->fe = (feature *)0;
      }
    }
  }
  return ctx->fe;
}


/* test if this test covers this tuple */
int test_tuple( ub2 *test, feature *tuple, ub2 n)
{
  sb4 i;
  for (i=0; i<n; ++i) {
    if (tuple[i].f != test[tuple[i].v]) {
      break;
    }
  }
  return (i==n);
}



/*
------------------------------------------------------------------------------
Stuff specific to jenny
------------------------------------------------------------------------------
*/

#define MAX_FEATURES 52                  /* can't name more than 52 features */
#define MAX_N        32       /* never can do complete coverage of 33-tuples */
#define MAX_WITHOUT  (MAX_FEATURES*MAX_N)       /* max features in a without */
#define MAX_VECTORS  (((ub2)~0)-1)   /* More than 64K vectors requires a ub4 */
#define TUPLE_MEMORY 0x100*TUPLE_ARRAY /* below this, store tuples in memory */

/* A "test", which is a combination of features.  Prefix t. */
typedef  struct test {
  struct test *next;                                            /* next test */
  ub2         *f;                                   /* features in this test */
} test;

/* representation of a restriction, prefix w */
typedef  struct without {
  struct without *next;                                  /* next restriction */
  ub2             len;                            /* length of feature array */
  feature        *fe;                                       /* feature array */
} without;

/* whole current state, prefix s */
typedef  struct state {
  ub1      n_final;            /* The n in the user's n-tuples, default is 2 */
  ub1      n;                                               /* the current n */
  ub2      nvec;                                        /* number of vectors */
  ub2      ntests;                                   /* number of tests in t */
  ub2     *vec;                         /* number of features in each vector */
  without *w;                             /* list of disallowed combinations */
  tu_arr  *tu;                  /* root of the list of all untested n-tuples */
  test    *t;                              /* all the tests generated so far */
  test    *tuple_tester;    /* an all -1 test used to test individual tuples */
  ub2     *vecord;                       /* order in which to choose vectors */
  ub2     *featord;                     /* order in which to choose features */
  randctx  r;                                       /* random number context */
} state;

/* token definitions */
typedef enum token_type {
  TOKEN_ERROR = 0,                                /* TOKEN_ERROR has to be 0 */
  TOKEN_END,
  TOKEN_NUMBER,
  TOKEN_FEATURE,
  TOKEN_SPACE
} token_type;


void show_without( without *w)
{
  without *wp;
  ub4 i;
  for (wp = w; wp; wp = wp->next) {
    for (i=0; i<w->len; ++i) {
      printf("%d%c", w->fe[i].v+1, feature_name[w->fe[i].f]);
    }
  }
  printf("\n");
}

void initialize( state *s)
{
  /* make all freeable pointers start out zero */
  s->vec          = (ub2 *)0;
  s->w            = (without *)0;
  s->tu           = (tu_arr *)0;
  s->t            = (test *)0;
  s->tuple_tester = (test *)0;
  s->vecord       = (ub2 *)0;
  s->featord      = (ub2 *)0;

  /* fill in default values */
  s->nvec = (ub2)0;
  s->n = 2;              /* guarantees that all pairs of vectors are covered */
  s->ntests = 0;
  rand_init(&s->r, 0);                 /* initialize random number generator */
}


void cleanup(state *s)
{
  while (s->tu) {
    tu_arr *tu2 = s->tu;
    s->tu = tu2->next;
    bob_free((char *)tu2);
  }

  /* free the restrictions */
  while (s->w) {
    without *w = s->w;
    s->w = s->w->next;
    if (w->fe) bob_free((char *)w->fe);
    bob_free((char *)w);
  }

  while (s->t) {
    test *t = s->t;
    s->t = s->t->next;
    if (t->f) {
      bob_free((char *)t->f);
    }
    bob_free((char *)t);
  }

  if (s->tuple_tester) {
    if (s->tuple_tester->f) {
      bob_free((char *)s->tuple_tester->f);
    }
    bob_free((char *)s->tuple_tester);
  }

  if (s->vecord) {
    bob_free((char *)s->vecord);
  }

  if (s->featord) {
    bob_free((char *)s->featord);
  }

  /* free the array of vector lengths */
  if (s->vec) bob_free((char *)s->vec);
}


char *my_alloc( state *s, size_t len)
{
  char *rsl;
  if (!(rsl = (char *)bob_malloc(len))) {
    printf("jenny: could not allocate space\n");
    cleanup(s);
    exit(0);  
  }
  return rsl;
}

/* add one test to the list of tests */
void add_test( state *s, test *t)
{
  test **tp;
  for (tp = &s->t; *tp; tp = &(*tp)->next)
    ;
  t->next = (test *)0;
  *tp = t;
  ++s->ntests;
}

/*
 * parse a token
 * Start at *curr in string *inp of length inl
 * Adjust *curr to be after the just-parsed token
 * Place the token value in *rsl
 * Return the token type
 */
token_type parse_token(char *inp, ub4 inl, ub4 *curr, ub4 *rsl)
{
  char mychar;
  ub4  i;

  if (*curr == inl)
    return TOKEN_END;
  mychar = inp[*curr];

  if (mychar == '\0') {
    return TOKEN_END;
  } else if (mychar == ' ' || mychar == '\t' || mychar == '\n') {
    /*--------------------------------------------------------- parse spaces */
    for (i=*curr+1; i < inl; ++i) {
      mychar = inp[i];
      if (!(mychar == ' ' || mychar == '\t' || mychar == '\n'))
	break;
    }
    *curr = i;
    return TOKEN_SPACE;
  } else if (mychar >= '0' && mychar <= '9') {
    /*------------------------------------------------------- parse a number */
    ub4 i, number = 0;
    for (i=*curr; i < inl && inp[i] >= '0' && inp[i] <= '9';  ++i) {
      number = number*10 + (inp[i] - '0');
    }
    *curr = i;
    *rsl = number;
    return TOKEN_NUMBER;
  } else if ((mychar >= 'a' && mychar <= 'z') ||
	     (mychar >= 'A' && mychar <= 'Z')) {
    /*------------------------------------------------- parse a feature name */
    ub4 i;
    for (i=0; i<MAX_FEATURES; ++i)
      if (feature_name[i] == mychar)
	break;
    if (i == MAX_FEATURES) {
      printf("jenny: the name '%c' is not used for any feature\n",
	     mychar);
	return TOKEN_ERROR;
    }
    *rsl = i;
    ++*curr;
    return TOKEN_FEATURE;
  } else {
    return TOKEN_ERROR;
  }
}

/* Return TRUE if this test obeys all the restrictions */
int test_without( test *t, without *w)
{
  for (; w; w = w->next) {
    ub4 i = 0;
    int match = TRUE;                        /* match the entire restriction */
    while (i<w->len) {
      int vector_match = FALSE;                      /* match in this vector */
      do {
	if (t->f[w->fe[i].v] == w->fe[i].f) {
	  vector_match = TRUE;
	}
	++i;
      } while (i<w->len && (w->fe[i].v == w->fe[i-1].v));
      if (!vector_match) {
	match = FALSE;
	break;
      }
    }
    if (match) {
      return FALSE;
    }
  }
  return TRUE;
}

#define BUFSIZE (MAX_VECTORS*7+2)
/* load old tests before generating new ones */
int load( state *s, char *testfile)
{
  char  buf[BUFSIZE];            /* buffer holding a line read from the file */
  FILE *f;
  
  if (testfile[0] == '\0') {
    f = stdin;
  } else {
    f = fopen(testfile, "r");
  }

  if (!f) {
    printf("jenny: file %s could not be opened\n", testfile);
    return FALSE;
  }

  while (fgets(buf, BUFSIZE, f) && (buf[0] != '.')) {
    ub4   curr = 0;                               /* current offset into buf */
    ub4   value;                                              /* token value */
    token_type token;                                          /* token type */
    ub4   i;
    test *t;

    t = (test *)my_alloc( s, sizeof(test));
    t->f = (ub2 *)my_alloc( s, sizeof(ub2)*s->nvec);
    add_test(s, t);

    for (i=0; i<s->nvec; ++i) {
      if (parse_token(buf, UB4MAXVAL, &curr, &value) != TOKEN_SPACE) {
	printf("jenny: -o, non-space found where space expected\n");
	goto failure;
      }
      if (parse_token(buf, UB4MAXVAL, &curr, &value) != TOKEN_NUMBER) {
	printf("jenny: -o, non-number found where number expected\n");
	goto failure;
      }
      if (value-1 != i) {
	printf("jenny: -o, number %d found out-of-place\n", value);
	goto failure;
      }
      if (parse_token(buf, UB4MAXVAL, &curr, &value) != TOKEN_FEATURE) {
	printf("jenny: -o, non-feature found where feature expected\n");
	goto failure;
      }
      if (value >= s->vec[i]) {
	printf("jenny: -o, feature %c does not exist in vector %d\n", 
	       feature_name[value], i+1);
	goto failure;
      }
      t->f[i] = value;
    }
    if (parse_token(buf, UB4MAXVAL, &curr, &value) != TOKEN_SPACE) {
      printf("jenny: -o, non-space found where trailing space expected\n");
      goto failure;
    }
    if (parse_token(buf, UB4MAXVAL, &curr, &value) != TOKEN_END) {
      printf("jenny: -o, testcase not properly terminated\n");
      goto failure;
    }

    /* make sure the testcase obeys all the withouts */
    if (!test_without(t, s->w)) {
      printf("jenny: -o, old testcase contains some without\n");
      goto failure;
    }
  }

  (void)fclose(f);
  return TRUE;

 failure:
  while (fgets(buf, BUFSIZE, f) && (buf[0] != '.'))
    ;                                            /* finish reading the input */
  (void)fclose(f);                                         /* close the file */
  return FALSE;
}

static const ub1 *jenny_doc[] = {
  "jenny:\n",
  "  Given a set of feature dimensions and withouts, produce tests\n",
  "  covering all n-tuples of features where all features come from\n",
  "  different dimensions.  For example (=, <, >, <=, >=, !=) is a\n",
  "  dimension with 6 features.  The type of the left-hand argument is\n",
  "  another dimension.  Dimensions are numbered 1..65535, in the order\n",
  "  they are listed.  Features are implicitly named a..z, A..Z.\n",
  "   3 Dimensions are given by the number of features in that dimension.\n",
  "  -h prints out these instructions.\n",
  "  -n specifies the n in n-tuple.  The default is 2 (meaning pairs).\n",
  "  -w gives withouts.  -w1b4ab says that combining the second feature\n",
  "     of the first dimension with the first or second feature of the\n",
  "     fourth dimension is disallowed.\n",
  "  The output is a test per line, one feature per dimension per test,\n",
  "  followed by the list of all allowed tuples that jenny could not\n",
  "  reach.\n",
  "\n",
  "  Example: jenny -n3 3 2 2 -w2b3b 5 3 -w1c3b4ace5ac 8 2 2 3 2\n",
  "  This gives ten dimensions, asks that for any three dimensions all\n",
  "  combinations of features (one feature per dimension) be covered,\n",
  "  plus it asks that certain combinations of features\n",
  "  (like (1c,3b,4c,5c)) not be covered.\n",
  "\n"
};


/* parse -n, the tuple size */
int parse_n( state *s, char *myarg)
{
  ub4 curr = 0;
  ub4 temp = UB4MAXVAL;
  token_type token;
  ub4 dummy;

  if ((token=parse_token(myarg, UB4MAXVAL, &curr, &temp)) != TOKEN_NUMBER) {
    printf("jenny: -n should give an integer in 1..32, for example, -n2.\n");
    return FALSE;
  }
  if ((token=parse_token(myarg, UB4MAXVAL, &curr, &dummy)) != TOKEN_END) {
    printf("jenny: -n should be followed by just an integer\n");
    return FALSE;
  }
  
  if ((temp < 1) || (temp > 32)) {
    printf("jenny: -n says all n-tuples should be covered.\n");
    return FALSE;
  }
  if (temp > s->nvec) {
    printf("jenny: -n, %ld-tuples are impossible with only %d dimensions\n",
	   temp, s->nvec);
    return FALSE;
  }
  s->n = (ub2)temp;
  return TRUE;
}



/* parse -w, a without */
int parse_w( state *s, ub1 *myarg)
{
  without   *w;
  feature    fe[MAX_WITHOUT];
  ub1        used[MAX_VECTORS];
  ub4        vector_number;
  ub4        curr = 0;
  ub4        fe_len, value;
  ub4        i, j, k;
  size_t     len = strlen(myarg);
  token_type t = parse_token(myarg, len, &curr, &value);
  
 start:
  for (i=0; i<s->nvec; ++i)
    used[i] = FALSE;
  if (t != TOKEN_NUMBER) {
    printf("jenny: -w is <number><features><number><features>...\n");
    printf("jenny: -w must start with an integer (1 to #dimensions)\n");
    return FALSE;
  }
  fe_len=0;
  
 number:
  vector_number = --value;
  if (vector_number >= s->nvec) {
    printf("jenny: -w, dimension %ld does not exist, ", vector_number+1);
    printf("you gave only %d dimensions\n", s->nvec);
    return FALSE;
  }
  if (used[vector_number]) {
    printf("jenny: -w, dimension %d was given twice in a single without\n",
	   vector_number+1);
    return FALSE;
  }
  used[vector_number] = TRUE;
  
  
  switch (parse_token(myarg, len, &curr, &value)) {
  case TOKEN_FEATURE: goto feature;
  case TOKEN_END:
    printf("jenny: -w, withouts must follow numbers with features\n");
    return FALSE;
  default:
    printf("jenny: -w, unexpected without syntax\n");
    printf("jenny: proper withouts look like -w2a1bc99a\n");
    return FALSE;
  }
  
 feature:
  if (value >= s->vec[vector_number]) {
    printf("jenny: -w, there is no feature '%c' in dimension %d\n",
	   feature_name[value], vector_number+1);
    return FALSE;
  }
  fe[fe_len].v = vector_number;
  fe[fe_len].f = value;
  if (++fe_len >= MAX_WITHOUT) {
    printf("jenny: -w, at most %d features in a single without\n",
	   MAX_WITHOUT);
    return FALSE;
  }

  
  switch (parse_token(myarg, len, &curr, &value)) {
  case TOKEN_FEATURE: goto feature;
  case TOKEN_NUMBER: goto number;
  case TOKEN_END: goto end;
  default:
    printf("jenny: -w, unexpected without syntax\n");
    printf("jenny: proper withouts look like -w2a1bc99a\n");
    return FALSE;
  }
  
 end:

  /* sort the vectors and features in this restriction */
  for (i=0; i<fe_len; ++i) {
    for (j=i+1; j<fe_len; ++j) {
      if ((fe[i].v > fe[j].v) ||
	  ((fe[i].v == fe[j].v) && (fe[i].f > fe[j].f))) {
	ub2 fe_temp;
	fe_temp = fe[i].v;
	fe[i].v = fe[j].v;
	fe[j].v = fe_temp;
	fe_temp = fe[i].f;
	fe[i].f = fe[j].f;
	fe[j].f = fe_temp;
      }
    }
  }

  /* allocate a without */
  w = (without *)my_alloc( s, sizeof(without));
  w->next = s->w;
  w->len = fe_len;
  w->fe = (feature *)my_alloc( s, sizeof(feature)*fe_len);
  for (i=0; i<fe_len; ++i) {
    w->fe[i].v = fe[i].v;
    w->fe[i].f = fe[i].f;
  }
  s->w = w;

  return TRUE;
}

/* parse -s, a seed for the random number generator */
int parse_s( state *s, ub1 *myarg)
{
  ub4 seed = 0;
  ub4 dummy = 0;
  ub4 curr = 0;
  if (parse_token( myarg, UB4MAXVAL, &curr, &seed) != TOKEN_NUMBER) {
    printf("jenny: -s must be followed by a positive integer\n");
    return FALSE;
  }
  if (parse_token( myarg, UB4MAXVAL, &curr, &dummy) != TOKEN_END) {
    printf("jenny: -s should give just an integer, example -s123\n");
    return FALSE;
  }
  rand_init(&s->r, seed);          /* initialize random number generator */
  return TRUE;
}

/* parse the inputs */
int parse( int argc, char *argv[], state *s)
{
  int   i, j;
  ub4   temp;
  char *testfile = (char *)0;

  /* internal check: we have MAX_FEATURES names for features */
  if (strlen(feature_name) != MAX_FEATURES) {
    printf("feature_name length is wrong, %d\n", strlen(feature_name));
    return FALSE;
  }

  /* How many vectors are there?  Set nvec, allocate space for vec.  */
  for (temp=0, i=1; i<argc; ++i) {
    if (argv[i][0] >= '0' && argv[i][0] <= '9') {
      ++temp;
    }
  }
  if (temp > MAX_VECTORS) {
    printf("jenny: maximum number of vectors is %ld.  %ld is too many.\n",
	   MAX_VECTORS, temp);
    return FALSE;
  }
  s->nvec = (ub2)temp;
  s->vec = (ub2 *)my_alloc( s, sizeof(ub2)*(s->nvec));

  /* Read the lengths of all the vectors */
  for (i=1, j=0; i<argc; ++i) {
    if (argv[i][0] >= '0' && argv[i][0] <= '9') {           /* vector length */
      ub1 *myarg = argv[i];
      ub4  dummy;
      ub4  curr = 0;

      (void)parse_token(myarg, UB4MAXVAL, &curr, &temp);
      if (parse_token(myarg, UB4MAXVAL, &curr, &dummy) != TOKEN_END) {
	printf("jenny: something was trailing a dimension number\n");
	return FALSE;
      }
      if (temp > MAX_FEATURES) {
	printf("jenny: vectors must be smaller than %d.  %ld is too big.\n",
	       MAX_FEATURES, temp);
	return FALSE;
      }
      if (temp < 2) {
	printf("jenny: a vector must have at least 2 features, not %d\n",
	       temp);
	return FALSE;
      }
      s->vec[j++] = (ub2)temp;
    } else if (argv[i][1] == 'h') {
      int i;
      for (i=0; i<(sizeof(jenny_doc)/sizeof(ub1 *)); ++i) {
	printf(jenny_doc[i]);
      }
      return FALSE;
    }
  }

  /* Read the rest of the arguments */
  for (i=1; i<argc; ++i) if (argv[i][0] == '-') {           /* vector length */
    switch(argv[i][1]) {
    case '\0':
      printf("jenny: '-' by itself isn't a proper argument.\n");
      return FALSE;
    case 'o':                               /* -o, file containing old tests */
      testfile = &argv[i][2];
      break;
    case 'n':                       /* -n, get the n of "cover all n-tuples" */
      if (!parse_n( s, &argv[i][2])) return FALSE;
      break;
    case 'w':                                /* -w, "without", a restriction */
      if (!parse_w( s, &argv[i][2])) return FALSE;
      break;
    case 's':                           /* -s, "random", change the behavior */
      if (!parse_s( s, &argv[i][2])) return FALSE;
      break;
    default:
      printf("jenny: legal arguments are numbers, -n, -s, -w, -h, not -%c\n",
	     argv[i][1]);
      return FALSE;
    }
  }                                            /* for (each argument) if '-' */

  if (s->n > s->nvec) {
    printf("jenny: %ld-tuples are impossible with only %d dimensions\n",
	   s->n, s->nvec);
    return FALSE;
  }

  s->tuple_tester = (test *)my_alloc( s, sizeof(test));
  s->tuple_tester->f = (ub2 *)my_alloc( s, sizeof(ub2)*s->nvec);
  s->tuple_tester->next = (test *)0;
  s->vecord = (ub2 *)my_alloc( s, sizeof(ub2)*s->nvec);
  s->featord = (ub2 *)my_alloc( s, sizeof(ub2)*MAX_FEATURES);
  for (i=0; i<s->nvec; ++i) {
    s->tuple_tester->f[i] = (ub2)~0;
    s->vecord[i] = (ub2)i;
  }

  /* read in any old tests so we can build from that base */
  if (testfile) {
    if (!load( s, testfile)) {
      return FALSE;
    }
  }

  return TRUE;
}

/* generate a random test */
void rand_test( state *s, test *t)
{
  ub4 i;
  for (i=0; i<s->nvec; ++i) {
    t->f[i] = rand(&s->r) % s->vec[i];
  }
}

/* Generate one random test that obeys all the restrictions 
 * and add it to the list of tests
 */
test *rand_good_test( state *s)
{
  test *t = (test *)my_alloc( s, sizeof(test));
  t->f = (ub2 *)my_alloc( s, sizeof(ub2)*s->nvec);
  do {
    rand_test(s, t);
  } while (!test_without(t, s->w));
  return t;
}

/* print out a single test */
void report( test *t, ub2 len)
{
  ub4 i;
  for (i=0; i<len; ++i) {
    printf(" %d%c", i+1, feature_name[t->f[i]]);
  }
  printf(" \n");
}

/* print out all the tests */
void report_all( state *s)
{
  test *tp;
  for (tp = s->t; tp; tp = tp->next) {
    report(tp, s->nvec);
  }
}


void build_tuples( state *s)
{
  feature  offset[MAX_N];
  sb4      i, j;
  ub8      count = 0;
  test    *t;

  if (s->tu) {
    printf("danger, danger will robinson! %d\n", s->tu->len);
  }

  /* Make a list of allowed but uncovered tuples */
  for (i=0; i<s->n; ++i) {
    offset[i].v = i;
    offset[i].f = 0;
  }
  for (;;) {

    /* is this tuple disallowed? */
    for (i=0; i<s->n; ++i) {
      s->tuple_tester->f[offset[i].v] = offset[i].f;
    }
    if (!test_without(s->tuple_tester, s->w))
      goto make_next_tuple;

    /* is this tuple covered by the existing tests? */
    for (t=s->t; t; t = t->next) {
      for (i=0; i<s->n; ++i) {
	if (t->f[offset[i].v] != offset[i].f) {
	  break;
	}
      }
      if (i == s->n) {
	goto make_next_tuple;
      }
    }

    /* add it to the list of uncovered tuples */
    insert_tuple( &s->tu, s->n, offset);
    ++count;

#ifdef NEVER
    printf("missed %d:", s->n);
    for (i=0; i<s->n; ++i)
      printf("%d%c ",
	     offset[i].v+1, feature_name[offset[i].f]);
    printf("\n");
#endif

    /* next tuple */
  make_next_tuple:
    for (i=0; i<s->n; ++i) {
      s->tuple_tester->f[offset[i].v] = (ub2)~0;
    }
    i=s->n;
    while (i-- &&
	   offset[i].v == s->nvec-s->n+i &&
	   offset[i].f == s->vec[offset[i].v]-1)
      ;
    if (i == -1)
      break;                                                         /* done */
    else if (offset[i].f < s->vec[offset[i].v]-1) {
      ++offset[i].f;                                    /* increment feature */
      for (i; i<s->n-1; ++i) {       /* reset all less significant positions */
	offset[i+1].v = offset[i].v+1;
	offset[i+1].f = 0;
      }
    }
    else {
      ++offset[i].v;   /* increment least significant non-maxed-out position */
      offset[i].f = (ub2)0;                             /* reset its feature */
      for (i; i<s->n-1; ++i) {       /* reset all less significant positions */
	offset[i+1].v = offset[i].v+1;
	offset[i+1].f = 0;
      }
    }
  }

#ifdef NEVER  
  printf("uncovered: %ld %ld %ld\n", (ub4)s->ntests, (ub4)count, (ub4)s->n);
#endif
}

/*
 * Generate one test that obeys all the restrictions and covers at 
 * least one tuple.  Do not add it to the list of tests yet.  Return FALSE
 * if it cannot be done.
 */
#define MAX_ITERS 100
int generate_good( state *s, test *t, feature *tuple)
{
  int  i;
  ub2 *vecord = s->vecord;
  ub2 *featord = s->featord;
  ub4  iter = 0;

 top:
  if (++iter > MAX_ITERS) {
    return FALSE;
  }
  /* choose to place tuple first, then other vectors at random */
  for (i=0; i<s->nvec; ++i) {
    vecord[i] = i;
  }
  for (i=0; i<s->n; ++i) {
    ub2 temp = vecord[i];
    vecord[i] = tuple[i].v;
    vecord[tuple[i].v] = temp;
  }
  vecord = &s->vecord[s->n];
  for (i=1; i<s->nvec - s->n; ++i) {
    ub2 j = rand(&s->r) % (i+1);
    ub2 temp = vecord[i];
    vecord[i] = vecord[j];
    vecord[j] = temp;
  }
  vecord = s->vecord;

  /* next, place just the tuple */
  for (i=0; i<s->n; ++i) {
    t->f[vecord[i]] = tuple[i].f;
  }
  for (i=s->n; i<s->nvec; ++i) {
    t->f[vecord[i]] = (ub2)~0;
  }
  if (!test_without(t, s->w)) {
    return FALSE;
  }

  /*
   * Now add one vector at a time, covering as many uncovered tuples at
   * each step as possible.
   */
  for (i=s->n; i<s->nvec; ++i) {
    sb4 best = -1;
    sb4 best_count = -1;
    sb4 j;
    sb4 j_count = -1;

    for (j=0; j<s->vec[vecord[i]]; ++j) {
      featord[j] = j;
    }
    for (j=1; j<s->vec[vecord[i]]; ++j) {
      ub2 k = rand(&s->r) % (j+1);
      ub2 temp = featord[j];
      featord[j] = featord[k];
      featord[k] = temp;
    }

    for (j=0; j<s->vec[vecord[i]]; ++j) {
      tu_iter  ctx;
      feature *this;
      t->f[vecord[i]] = featord[j];

      /* does it obey the restrictions? */
      if (!test_without(t, s->w)) {
	continue;
      }

      /* how many tuples does it cover? */
      j_count = 0;
      this = start_tuple(&ctx, s->tu, s->n);
      while (this) {
	j_count += test_tuple( t->f, this, s->n);
	this = next_tuple(&ctx);
      }

      /* remember it if it is the best so far */
      if (j_count > best_count) {
	best = featord[j];
	best_count = j_count;
      }
    }
    if (best_count > -1) {
      t->f[vecord[i]] = best;
    } else {
      /*
       * I really ought to figure out the minimal disallowed tuple here
       * and add it to the set of restrictions.
       */
      goto top;
    }
  }

  return TRUE;
}

#define GROUP_SIZE 5
void cover_tuples( state *s)
{
  while (s->tu) {
    ub4         i;
    sb4         best_count = -1;
    test       *best_test;
    test       *curr_test;

    curr_test = (test *)my_alloc( s, sizeof(test));
    curr_test->f = (ub2 *)my_alloc( s, sizeof(ub2)*s->nvec);
    best_test = (test *)my_alloc( s, sizeof(test));
    best_test->f = (ub2 *)my_alloc( s, sizeof(ub2)*s->nvec);

    /* find a good test */
    for (i=0; i<GROUP_SIZE; ++i) {
      tu_iter  ctx;
      sb4      this_count = 0;
      feature *this = start_tuple(&ctx, s->tu, s->n);

      /* generate a test that covers the first tuple */
      if (!generate_good(s, curr_test, this)) {
	break;
      }

      /* see how many tuples are covered altogether */
      while (this) {
	this_count += test_tuple( curr_test->f, this, s->n);
	this = next_tuple(&ctx);
      }
      if (this_count > best_count) {
	test *temp = best_test;
	best_test = curr_test;
	curr_test = temp;

	best_count = this_count;
      }
    }

    if (i < GROUP_SIZE) {
      without *w = (without *)my_alloc( s, sizeof(without));
      printf("Could not cover tuple ");
      show_tuple(s->tu->fe, s->n);

      /* add this tuple to the list of restrictions */
      w->fe = (feature *)0;
      w->next = s->w;
      s->w = w;
      w->len = s->n;
      w->fe = (feature *)my_alloc( s, sizeof(feature)*s->n);
      for (i=0; i<s->n; ++i) {
	w->fe[i].v = s->tu->fe[i].v;
	w->fe[i].f = s->tu->fe[i].f;
      }
      
      (void)delete_tuple(&s->tu, s->n, s->tu->fe);
    } else {
      tu_iter  ctx;
      feature *this = start_tuple(&ctx, s->tu, s->n);

      /* remove all the tuples covered by it */
      while (this) {
	ub4 j;
	for (j=0;  j < s->n;  ++j) {
	  if (this[j].f != best_test->f[this[j].v]) {
	    break;
	  }
	}
	if (j == s->n) {
	  this = delete_tuple(&s->tu, s->n, this);
	} else {
	  this = next_tuple(&ctx);
	}
      }

      /* add it to the list of tests */
      add_test(s, best_test);

      bob_free((char *)curr_test->f);
      bob_free((char *)curr_test);
    }
  }
}

void driver( int argc, char *argv[])
{
  state s;                                                 /* internal state */

  initialize(&s);

  if (parse(argc, argv, &s)) {               /* read the user's instructions */

    /* First cover all singletons, then all pairs, then all triples ... */
    for (s.n_final = s.n, s.n = 1; s.n <= s.n_final; ++s.n) {
      ub4 more_guesses;          /* how many more tests to randomly generate */

      /* build a list of all uncovered tuples in memory */
      build_tuples(&s);

      /* now accept tests only if they cover the uncovered tuples */
      cover_tuples(&s);

      /* make sure s->tu is reset to null */
      while (s.tu) {
	tu_arr *x = s.tu->next;
	bob_free((char *)s.tu);
	s.tu = x;
      }
    }
    report_all(&s);                                    /* report the results */
  }
  cleanup(&s);                                      /* deallocate everything */
}

int main( int argc, char *argv[])
{
  driver(argc, argv);
  return 0;
}

