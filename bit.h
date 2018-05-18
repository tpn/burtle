/*
--------------------------------------------------------
Bit-array related stuff
Bob Jenkins, 1996.  Public Domain.
--------------------------------------------------------
*/

#ifndef STANDARD
#include <standard.h>
#endif

#ifndef BIT
#define BIT

#define MAKEBIT(x) (((x)+31)>>5)
typedef ub4  bitvec;

/* set mybit in x 
 *   x:     array of bits
 *   mybit: which bit in x to set to 1
 */
void bitv1(/*_ bitvec *x, word mybit _*/);

/* clear mybit in x 
 *   x:     array of bits
 *   mybit: which bit in x to set to 0
 */
void bitv0(/*_ bitvec *x, word mybit _*/);

/* Return TRUE if mybit is set in x, FALSE otherwise
 *   x:     array of bits
 *   mybit: which bit to test
 */
word bitvtst(/*_ bitvec *x, word mybit _*/);

/* clear x
 *   x:    array of bits, all of which should be set to zero
 *   len:  number of bits in x
 */
void bitvclr(/*_ bitvec *x, word len _*/);

/* x := y, assign x to have the same value as y
 *   x:   array of len bits to be set
 *   y:   array of at least len bits, first len to be copied to x
 *   len: number of bits in x
 */
void bitvcpy(/*_ bitvec *x, bitvec *y, word len _*/);

/* (x!=y), compare, return TRUE if x and y have the same value
 *   x:   array of at least len bits
 *   y:   another array of at least len bits
 *   len: number of bits to check 
 */
word bitvcmp(/*_ bitvec *x, bitvec *y, word len _*/);

/* x = y ^ z, cause x to be y xor z
 *   x:   array of len bits
 *   y:   array of at least len bits
 *   z:   array of at least len bits
 *   len: number of bits in x
 */
void bitvxor(/*_ bitvec *x, bitvec *y, bitvec *z, word len _*/);

/* print a bit vector
 *   x:   array of at lest len bits
 *   len: number of bits in x
 */
void bitprint(/*_ bitvec *x, word len _*/);

#endif  /* BIT */
