/*
------------------------------------------------------------------------------
gauss.h
Bob Jenkins, 1996, Public Domain
------------------------------------------------------------------------------
*/
#ifndef STANDARD
#include "standard.h"
#endif

#ifndef GAUSS
#define GAUSS

/* x := y * m;
 *   x:   a bitvec of col values
 *   y:   a bitvec of row values
 *   m:   an array of row bitvecs of col values each
 *   row: number of rows
 *   col: number of columns
 */
void xym(/*_ bitvec *x, bitvec *y, bitvec **m, word row, word col _*/);

/* Gaussian Elimination (of boolean values)
 *   m:     bitvtst(m[i],j) is the jth term of the ith equation
 *   row:   how many equations are there
 *   col:   how many terms are there
 * Return the number of linearly independent equations
 */
word gauss(/*_ bitvec **m, word row, word col _*/);

#endif

