/*
-------------------------------------------------------------------------------
anagram.c : find all anagrams of a given string (not in lexicographic order)

usage:
gcc -O anagram.c -o anagram
anagram xyz

(c) Bob Jenkins, October 18 2000.  I place this in the public domain.
Feel free to use or modify this in any way you wish.
-------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <string.h>

#define MAXLEN 2000

/*
-------------------------------------------------------------------------------
Internal representation of a string, used to construct anagrams
-------------------------------------------------------------------------------
*/
struct anagram {
  /* string representation */
  int   len;                                    /* original length of string */
  char  letter[256];                   /* list of distinct letters in string */
  int   count[256];                              /* how many of each letter? */
  int   n;                                     /* number of distinct letters */

  /* anagram state */
  int   pos[MAXLEN];                              /* positions in an anagram */
  int   ip;                      /* I Pos: pos[ip] is currently being filled */
  int   il;                /* I Letter: letter[il] is currently being placed */
  int   is;                     /* I Swap: swap with only pos[is] or greater */
  char  ana[MAXLEN];                                   /* the anagram itself */
};
typedef  struct anagram  anagram;


/*
-------------------------------------------------------------------------------
Given a string, initialize internal anagram state
-------------------------------------------------------------------------------
*/
void init_anagram( char    *string, /* IN: we will find all anagrams of this */
		   anagram *a) {              /* IN/OUT: state to initialize */
  int z[256];
  int i;

  a->len = strlen(string);
  for (i=0;  i<256;  ++i) {                              /* clear hash table */
    z[i] = 0;
  }
  for (i=0;  i<a->len;  ++i) {
    ++z[string[i]];                   /* find counts for each distinct value */
    a->pos[i] = i;                             /* initialize positions array */
  }
  for (i=0, a->n=0;  i<256;  ++i) {               /* find all nonzero counts */
    if (z[i] > 0) {
      a->letter[a->n] = (char)i;
      a->count[a->n] = z[i];
      a->n++;
    }
  }
  a->ip = 0;                                          /* start at position 0 */
  a->il = 0;                                            /* start at letter 0 */
  a->is = 0;                            /* consider swapping with position 0 */
}

/*
-------------------------------------------------------------------------------
Swap two integers
-------------------------------------------------------------------------------
*/
void swap( int *x, int *y) {
  int temp = *x;
  *x = *y;
  *y = temp;
}

/*
-------------------------------------------------------------------------------
Construct all anagrams of the given string

We fill one position of the anagram at a time.
We place all occurences of one letter, then all occurences of the next,
  and so forth.
When we reach the last distinct letter we just fill in all remaining holes 
  with that letter and report the result without any fancy state maintenance.
When placing a letter, we place the first occurence, then place the second
  somewhere after the first, then place the third somewhere after the second,
  and so forth.  We always leave room at the top for all remaining occurences.
  This way we do (n choose m) work to place the m occurences in n positions.
-------------------------------------------------------------------------------
*/
void recurse( anagram *a) {
  int   i;
  int   len   = a->len;
  char *ana   = a->ana;
  int  *pos   = a->pos;
  int   ip    = a->ip;
  int   il    = a->il;
  char  x     = a->letter[il];

  if (il == a->n-1) {
    /*------------------------------------------------ last distinct letter? */
    for (i=ip;  i<len;  ++i) {          /* just fill all remaining positions */
      ana[pos[i]] = x;
    }
    printf("%.*s\n", len, ana);                     /* and report the result */
  } else if (a->count[il] == 1) {
    /*---------------------------------------- last instance of this letter? */
    int oldis = a->is;
    ++a->il;
    ++a->ip;

    a->is = a->ip;                        /* consider all unfilled positions */
    for (i=oldis;  i<len;  ++i) {
      swap(pos+ip, pos+i);                                           /* swap */
      ana[pos[ip]] = x;                                  /* fill this letter */
      recurse( a);                          /* go on to next distinct letter */
      swap(pos+ip, pos+i);                                        /* restore */
    }
    a->is = oldis;
    --a->ip;
    --a->il;
  } else {
    /*----------------------------------------- in the middle of some letter */
    int limit = len - (--a->count[il]);
    int oldis = a->is;

    ++a->ip;
    for (i=oldis;  i<limit;  ++i) {
      a->is = i+1;              /* consider only positions beyond this point */
      swap(pos+ip, pos+i);                                           /* swap */
      ana[pos[ip]] = x;                                  /* fill this letter */
      recurse( a);                                /* go on to next duplicate */
      swap(pos+ip, pos+i);                                        /* restore */
    }
    --a->ip;
    ++a->count[il];
    a->is = oldis;
  }
}

/*
-------------------------------------------------------------------------------
Handle getting the input, initialize, then get all anagrams
-------------------------------------------------------------------------------
*/
int main( int argc, char **argv) {
  if (argc != 2) {
    printf("'anagram xxx' will produce all anagrams of string xxx\n");
  } else {
    anagram a;
    init_anagram( argv[1], &a);                          /* initialize state */
    recurse( &a);                            /* find all anagrams of argv[1] */
  }
  return 1;
}
