/*
-----------------------------------------------------------------------
IBAA scaled down to 8 terms and 6 bits per term, (8+2)*6=60 bits total.
The program is currently measuring r[1]-r[0] for every batch.
-----------------------------------------------------------------------
*/

typedef  unsigned int  u4;   /* unsigned four bytes, 32 bits */
#define ALPHA      (3)
#define SIZE       (1<<ALPHA)
#define ind(x)     ((x)&(SIZE-1))
#define VALSIZE    (1<<(2*ALPHA))
#define MASK       (VALSIZE-1)
#define barrel(a)  ((((a)<<4)^((a)>>2)) & MASK)
 
static void ibaa(m,r,aa,bb)
     u4 *m;   /* Memory: array of SIZE ALPHA-bit terms */
     u4 *r;   /* Results: the sequence, same size as m */
     u4 *aa;  /* Accumulator: a single value */
     u4 *bb;  /* the previous result */
{
  u4 a,b,x,y,i;
 
  a = *aa; b = *bb;
  for (i=0; i<SIZE; ++i) {
    x = m[i];  
    a = (barrel(a) + m[ind(i+(SIZE/2))]) & MASK;    /* set a */
    m[i] = y = (m[ind(x)] + a + b) & MASK;          /* set m */
    r[i] = b = (m[ind(y>>ALPHA)] + x) & MASK;       /* set r */
  }
  *bb = b; *aa = a;
}

int main() 
{
  u4 m[SIZE], r[SIZE], aa = 1, bb = 1, count[VALSIZE], i, j, old=0;
  for (i=0; i<SIZE; ++i) m[i] = r[i] = 1;
  for (i=0; i<VALSIZE; ++i) count[i] = 0;
  for (i=1; i<(1<<27); ++i) {
    ibaa(m, r,&aa, &bb);
    count[(r[1]-r[0])&MASK]++;
  }
  for (i=0; i<VALSIZE; ++i) {
    printf("%.8x ", count[i]);
    if ((i&7) == 7) printf("\n");
  }
  return 0;
}
