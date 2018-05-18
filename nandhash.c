/*
-----------------------------------------------------------------
Software simulation of a 128-bit hardware hash

I made no attempt to make this hash optimal.  I'm just exploring
what can be done if you can build a hash out of NAND gates and 
wires instead of out 32-bit + and ^ operations.

Bits are arranged in a 8 x 16 grid, well, a (2x4) grid of (4x4) 
blocks.  A nonlinear reversible mix, f(), works on 4x4 blocks.
There are 

4x4 blocks of bits are arranged like so
    0 1 2 3
    7 6 5 4
    8 9 a b
    f e d c
and each implement the mixing function in f().

The 2x4 blocks of 4x4 bits are arranged
    (0,0) (0,1) (0,2) (0,3)
    (1,0) (1,1) (1,2) (1,3)

Horizontal wires:
Bits 8,f of (*,0) are XORed to bits 8,f of all other blocks in that row.
Bits 9,e of (*,1) are XORed to bits 9,e of all other blocks in that row.
Bits a,d of (*,2) are XORed to bits a,d of all other blocks in that row.
Bits b,c of (*,3) are XORed to bits b,c of all other blocks in that row.

Vertical wires:
Bits 0,1,2,3 of block (1,*) are XORed to bit 0,1,2,3 respectively
of block (0,*), and bits 7,6,5,4 of block (0,*) are XORed to bit
7,6,5,4 respectively of block (1,*).

The hash is done in rounds.  Each round activates the long wires,
then mixes the 4x4 blocks.  4 rounds causes every bit to affect
at least 32 other bits, and 8 rounds is enough that you can use the 
results.  If you want 64-bit hash values, make that 5 and 9 rounds.

The software below uses 16 8-byte values (rather than 8 16-byte values),
so that the nonlinear mix can be done to all blocks at once.
-----------------------------------------------------------------
*/

#include <stdio.h>
#include <stddef.h>

typedef unsigned char  u1;
typedef unsigned long  u4;

typedef struct ranctx { u4 a; u4 b; u4 c; u4 d; } ranctx;

#define rot(x,k) ((x<<k)|(x>>(32-k)))
u4 ranval( ranctx *x ) {
  u4 e = x->a - rot(x->b, 27);
  x->a = x->b ^ rot(x->c, 17);
  x->b = x->c + x->d;
  x->c = x->d + e;
  x->d = e + x->a;
  return e;
}

#define LEN 16

#define ROWS 2
#define COLS 4


/* do one round of the hash */
void hash(u1 x[LEN])
{
  u1 i;
  u1 y[LEN];

  /* Horizontal linear mixing */
  i =  x[ 8]&0x11;
  x[ 8] ^= (i<<1) | (i<<2) | (i<<3);
  i =  x[15]&0x11;
  x[15] ^= (i<<1) | (i<<2) | (i<<3);
  i =  x[ 9]&0x22;
  x[ 9] ^= (i>>1) | (i<<1) | (i<<2);
  i =  x[14]&0x22;
  x[14] ^= (i>>1) | (i<<1) | (i<<2);
  i =  x[10]&0x44;
  x[10] ^= (i>>2) | (i>>1) | (i<<1);
  i =  x[13]&0x44;
  x[13] ^= (i>>2) | (i>>1) | (i<<1);
  i =  x[11]&0x88;
  x[11] ^= (i>>3) | (i>>2) | (i>>1);
  i =  x[12]&0x88;
  x[12] ^= (i>>3) | (i>>2) | (i>>1);

  /* Vertical linear mixing */
  x[0] ^= x[0] >> 4;
  x[1] ^= x[1] >> 4;
  x[2] ^= x[2] >> 4;
  x[3] ^= x[3] >> 4;
  x[4] ^= x[4] << 4;
  x[5] ^= x[5] << 4;
  x[6] ^= x[6] << 4;
  x[7] ^= x[7] << 4;

  /* Do the nonlinear mixing of the bits in the 4x4 blocks */
  y[ 0] = (~x[15]) ^ ((x[11] & x[5]) | (x[7] & x[4]));
  y[ 1] = (~x[14]) ^ ((x[12] & x[2]) | (x[10] & x[4]));
  y[ 2] = (~x[13]) ^ ((x[12] & x[0]) | (x[6] & x[5]));
  y[ 3] = (~x[12]) ^ ((x[11] & x[2]) | (x[10] & x[1]));
  y[ 4] = (~x[11]) ^ ((x[8] & x[3]) | (x[4] & x[1]));
  y[ 5] = (~x[10]) ^ ((x[6] & x[1]) | (x[4] & x[3]));
  y[ 6] = (~x[ 9]) ^ ((x[8] & x[6]) | (x[7] & x[2]));
  y[ 7] = (~x[ 8]) ^ ((x[7] & x[6]) | (x[3] & x[0]));
  y[ 8] = x[2] ^ x[3] ^ x[6] ^ x[7];
  y[ 9] = x[0] ^ x[1] ^ x[2] ^ x[3];
  y[10] = x[0] ^ x[5] ^ x[6] ^ x[7];
  y[11] = x[0] ^ x[1] ^ x[2] ^ x[5];
  y[12] = x[1] ^ x[2] ^ x[3] ^ x[4];
  y[13] = x[0] ^ x[4] ^ x[6] ^ x[7];
  y[14] = x[0] ^ x[1] ^ x[2] ^ x[6];
  y[15] = x[0] ^ x[2] ^ x[3];

  for (i=0; i<LEN; ++i) x[i] = y[i];
}




void show(u1 x[LEN])
{
  int i,j,k;
  for (i=0; i<ROWS; ++i) {
    for (j=0; j<COLS; ++j) {
      for (k=0; k<4; ++k) {
	printf(" %s", (x[k] & (1<<(COLS*i+j))) ? "1" : "0");
      }
    }
    printf("\n");
    for (j=0; j<COLS; ++j) {
      for (k=8; --k>=4; ) {
	printf(" %s", (x[k] & (1<<(COLS*i+j))) ? "1" : "0");
      }
    }
    printf("\n");
    for (j=0; j<COLS; ++j) {
      for (k=8; k<12; ++k) {
	printf(" %s", (x[k] & (1<<(COLS*i+j))) ? "1" : "0");
      }
    }
    printf("\n");
    for (j=0; j<COLS; ++j) {
      for (k=16; --k>=12; ) {
	printf(" %s", (x[k] & (1<<(COLS*i+j))) ? "1" : "0");
      }
    }
    printf("\n");
  }
  printf("\n");
}

#define ROUNDS 100000

void showx(float x, float *worst, int verbose)
{
  x = x/ROUNDS;
  if (x < *worst) *worst = x;
  if (verbose) printf(" %.3f", x);
  if (1.0-x < *worst) *worst = 1.0-x;
}

void showc(float c[ROWS][COLS][LEN], int verbose)
{
  int i,j,k;
  float worst = 1.0;
  for (i=0; i<ROWS; ++i) {
    for (j=0; j<COLS; ++j) {
      for (k=0; k<4; ++k) {
	showx(c[i][j][k], &worst, verbose);
      }
      if (verbose) printf(" ");
    }
    if (verbose) printf("\n");
    for (j=0; j<COLS; ++j) {
      for (k=8; --k>=4; ) {
	showx(c[i][j][k], &worst, verbose);
      }
      if (verbose) printf(" ");
    }
    if (verbose) printf("\n");
    for (j=0; j<COLS; ++j) {
      for (k=8; k<12; ++k) {
	showx(c[i][j][k], &worst, verbose);
      }
      if (verbose) printf(" ");
    }
    if (verbose) printf("\n");
    for (j=0; j<COLS; ++j) {
      for (k=16; --k>=12; ) {
	showx(c[i][j][k], &worst, verbose);
      }
      if (verbose) printf(" ");
    }
    if (verbose) printf("\n\n");
  }
  printf("  worst = %f\n", worst);
}

int main()
{
  ranctx rctx = {1,1,1,1};
  u1 x[LEN], y[LEN];
  u4 m,n,o,count,i,j,k;

  /* see how thoroughly bit "tab[m][n] & (1<<o)" propogates */
  for (m=0; m<ROWS; ++m) if (m==0) {
    for (n=0; n<COLS; ++n) if (n==0) {
      for (o=0; o<LEN; ++o) if (o==14) {

	float c[ROWS][COLS][LEN];  /* count of how many times bit flipped */
	u4 r;  /* count # of rounds */

	/* initialize c */
	for (i=0; i<ROWS; ++i) {
	  for (j=0; j<COLS; ++j) {
	    for (k=0; k<LEN; ++k) {
	      c[i][j][k] = 0;
	    }
	  }
	}

	/* try r keys */
	for (r=0; r<ROUNDS; ++r) {

	  /* initialize to a random key */
	  for (i=0; i<LEN; ++i) {
	    for (j=0; j<COLS; ++j) {
	      x[i] = y[i] = ranval(&rctx) % (1 << (ROWS*COLS));
	    }
	  }

	  /* start with bit [m][n][o] different */
	  y[o] ^= (1 << (COLS*m+n));

	  /* apply several rounds of the hash to both keys */
	  hash(x); hash(y);
	  hash(x); hash(y);
	  hash(x); hash(y);

	  hash(x); hash(y);

	  /* measure which bits were affected */
	  for (k=0; k<LEN; ++k) {
	    u1 diff = x[k] ^ y[k];
	    for (i=0; i<ROWS; ++i) {
	      for (j=0; j<COLS; ++j) {
		if (diff & (1 << (COLS*i+j))) {
		  ++c[i][j][k];
		}
	      }
	    }
	  }
	}

	/* report which bits were affected */
	printf("%2d %2d %2d:\n", m,n,o);
	showc(c, 1);
      }
    }
  }
}
