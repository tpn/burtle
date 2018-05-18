/*
--------------------------------------------------------------------
checksum.c, by Bob Jenkins, 1996, Public Domain
hash(), hash2(), and mix() are the only externally useful functions.
Routines to test the hash are included if SELF_TEST is defined.
You can use this free for any purpose.  It has no warranty.
--------------------------------------------------------------------
*/
#define SELF_TEST

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;

#define HASHSTATE 8
#define HASHLEN   HASHSTATE

/*
--------------------------------------------------------------------
Mix -- mix 8 4-bit values as quickly and thoroughly as possible.
Repeating mix() three times achieves avalanche.
Repeating mix() four times eliminates all known funnels.
--------------------------------------------------------------------
*/
#define mix(a,b,c,d,e,f,g,h) \
{ \
   a^=b<<11; d+=a; b+=c; \
   b^=c>>2;  e+=b; c+=d; \
   c^=d<<8;  f+=c; d+=e; \
   d^=e>>16; g+=d; e+=f; \
   e^=f<<10; h+=e; f+=g; \
   f^=g>>4;  a+=f; g+=h; \
   g^=h<<8;  b+=g; h+=a; \
   h^=a>>9;  c+=h; a+=b; \
}

/*
--------------------------------------------------------------------
hash() -- hash a variable-length key into a 256-bit value
  k     : the key (the unaligned variable-length array of bytes)
  len   : the length of the key, counting by bytes
  state : an array of HASHSTATE 4-byte values (256 bits)
The state is the checksum.  Every bit of the key affects every bit of
the state.  There are no funnels.  About 112+6.875len instructions.

If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0; i<8; ++i) state[i] = 0x9e3779b9;
  for (i=0, h=0; i<n; ++i) hash( k[i], len[i], state);

See http://burtleburtle.net/bob/hash/evahash.html
Use to detect changes between revisions of documents, assuming nobody
is trying to cause collisions.  Do NOT use for cryptography.
--------------------------------------------------------------------
*/
void  hash( k, len, state)
register ub1 *k;
register ub4  len;
register ub4 *state;
{
   register ub4 a,b,c,d,e,f,g,h,length;

   /* Use the length and level; add in the golden ratio. */
   length = len;
   a=state[0]; b=state[1]; c=state[2]; d=state[3];
   e=state[4]; f=state[5]; g=state[6]; h=state[7];

   /*---------------------------------------- handle most of the key */
   while (len >= 32)
   {
      a += (k[0] +(k[1]<<8) +(k[2]<<16) +(k[3]<<24));
      b += (k[4] +(k[5]<<8) +(k[6]<<16) +(k[7]<<24));
      c += (k[8] +(k[9]<<8) +(k[10]<<16)+(k[11]<<24));
      d += (k[12]+(k[13]<<8)+(k[14]<<16)+(k[15]<<24));
      e += (k[16]+(k[17]<<8)+(k[18]<<16)+(k[19]<<24));
      f += (k[20]+(k[21]<<8)+(k[22]<<16)+(k[23]<<24));
      g += (k[24]+(k[25]<<8)+(k[26]<<16)+(k[27]<<24));
      h += (k[28]+(k[29]<<8)+(k[30]<<16)+(k[31]<<24));
      mix(a,b,c,d,e,f,g,h);
      mix(a,b,c,d,e,f,g,h);
      mix(a,b,c,d,e,f,g,h);
      mix(a,b,c,d,e,f,g,h);
      k += 32; len -= 32;
   }

   /*------------------------------------- handle the last 31 bytes */
   h += length;
   switch(len)
   {
   case 31: h+=(k[30]<<24);
   case 30: h+=(k[29]<<16);
   case 29: h+=(k[28]<<8);
   case 28: g+=(k[27]<<24);
   case 27: g+=(k[26]<<16);
   case 26: g+=(k[25]<<8);
   case 25: g+=k[24];
   case 24: f+=(k[23]<<24);
   case 23: f+=(k[22]<<16);
   case 22: f+=(k[21]<<8);
   case 21: f+=k[20];
   case 20: e+=(k[19]<<24);
   case 19: e+=(k[18]<<16);
   case 18: e+=(k[17]<<8);
   case 17: e+=k[16];
   case 16: d+=(k[15]<<24);
   case 15: d+=(k[14]<<16);
   case 14: d+=(k[13]<<8);
   case 13: d+=k[12];
   case 12: c+=(k[11]<<24);
   case 11: c+=(k[10]<<16);
   case 10: c+=(k[9]<<8);
   case 9 : c+=k[8];
   case 8 : b+=(k[7]<<24);
   case 7 : b+=(k[6]<<16);
   case 6 : b+=(k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=(k[3]<<24);
   case 3 : a+=(k[2]<<16);
   case 2 : a+=(k[1]<<8);
   case 1 : a+=k[0];
   }
   mix(a,b,c,d,e,f,g,h);
   mix(a,b,c,d,e,f,g,h);
   mix(a,b,c,d,e,f,g,h);
   mix(a,b,c,d,e,f,g,h);

   /*-------------------------------------------- report the result */
   state[0]=a; state[1]=b; state[2]=c; state[3]=d;
   state[4]=e; state[5]=f; state[6]=g; state[7]=h;
}


/*
--------------------------------------------------------------------
 This works on all machines, is identical to hash() on little-endian 
 machines, and it is much faster than hash(), but it requires
 -- that buffers be aligned, that is, ASSERT(((ub4)k)&3 == 0), and
 -- that all your machines have the same endianness.
--------------------------------------------------------------------
*/
void  hash2( k, len, state)
register ub1 *k;
register ub4  len;
register ub4 *state;
{
   register ub4 a,b,c,d,e,f,g,h,length;

   /* Use the length and level; add in the golden ratio. */
   length = len;
   a=state[0]; b=state[1]; c=state[2]; d=state[3];
   e=state[4]; f=state[5]; g=state[6]; h=state[7];

   /*---------------------------------------- handle most of the key */
   while (len >= 32)
   {
      a += *(ub4 *)(k+0);
      b += *(ub4 *)(k+4);
      c += *(ub4 *)(k+8);
      d += *(ub4 *)(k+12);
      e += *(ub4 *)(k+16);
      f += *(ub4 *)(k+20);
      g += *(ub4 *)(k+24);
      h += *(ub4 *)(k+28);
      mix(a,b,c,d,e,f,g,h);
      mix(a,b,c,d,e,f,g,h);
      mix(a,b,c,d,e,f,g,h);
      mix(a,b,c,d,e,f,g,h);
      k += 32; len -= 32;
   }

   /*------------------------------------- handle the last 31 bytes */
   h += length;
   switch(len)
   {
   case 31: h+=(k[30]<<24);
   case 30: h+=(k[29]<<16);
   case 29: h+=(k[28]<<8);
   case 28: g+=(k[27]<<24);
   case 27: g+=(k[26]<<16);
   case 26: g+=(k[25]<<8);
   case 25: g+=k[24];
   case 24: f+=(k[23]<<24);
   case 23: f+=(k[22]<<16);
   case 22: f+=(k[21]<<8);
   case 21: f+=k[20];
   case 20: e+=(k[19]<<24);
   case 19: e+=(k[18]<<16);
   case 18: e+=(k[17]<<8);
   case 17: e+=k[16];
   case 16: d+=(k[15]<<24);
   case 15: d+=(k[14]<<16);
   case 14: d+=(k[13]<<8);
   case 13: d+=k[12];
   case 12: c+=(k[11]<<24);
   case 11: c+=(k[10]<<16);
   case 10: c+=(k[9]<<8);
   case 9 : c+=k[8];
   case 8 : b+=(k[7]<<24);
   case 7 : b+=(k[6]<<16);
   case 6 : b+=(k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=(k[3]<<24);
   case 3 : a+=(k[2]<<16);
   case 2 : a+=(k[1]<<8);
   case 1 : a+=k[0];
   }
   mix(a,b,c,d,e,f,g,h);
   mix(a,b,c,d,e,f,g,h);
   mix(a,b,c,d,e,f,g,h);
   mix(a,b,c,d,e,f,g,h);

   /*-------------------------------------------- report the result */
   state[0]=a; state[1]=b; state[2]=c; state[3]=d;
   state[4]=e; state[5]=f; state[6]=g; state[7]=h;
}

#ifdef SELF_TEST

/* check that every input bit changes every output bit half the time */
#define MAXPAIR  80
#define MAXLEN   70
/* used for timings */
void driver1()
{
  ub1 buf[256];
  ub4 i;
  ub4 state[8];

  for (i=0; i<256; ++i) 
  {
    hash(buf,i,state);
  }
}

void driver2()
{
  ub1 qa[MAXLEN+1], qb[MAXLEN+2], *a = &qa[0], *b = &qb[1];
  ub4 c[HASHSTATE], d[HASHSTATE], i, j=0, k, l, m, z;
  ub4 e[HASHSTATE],f[HASHSTATE],g[HASHSTATE],h[HASHSTATE];
  ub4 x[HASHSTATE],y[HASHSTATE];
  ub4 hlen;

  printf("No more than %d trials should ever be needed \n",MAXPAIR/2);
  for (hlen=0; hlen < MAXLEN; ++hlen)
  {
    z=0;
    for (i=0; i<hlen; ++i)  /*----------------------- for each byte, */
    {
      for (j=0; j<8; ++j)   /*------------------------ for each bit, */
      {
	for (m=1; m<8; ++m) /*-------- for serveral possible levels, */
	{
	  for (l=0; l<HASHSTATE; ++l) e[l]=f[l]=g[l]=h[l]=x[l]=y[l]=~((ub4)0);

      	  /*---- check that every input bit affects every output bit */
	  for (k=0; k<MAXPAIR; k+=2)
	  { 
	    ub4 finished=1;
	    /* keys have one bit different */
	    for (l=0; l<hlen+1; ++l) {a[l] = b[l] = (ub1)0;}
	    /* have a and b be two keys differing in only one bit */
	    for (l=0; l<HASHSTATE; ++l) {c[l] = d[l] = m;}
	    a[i] ^= (k<<j);
	    a[i] ^= (k>>(8-j));
	    hash(a, hlen, c);
	    b[i] ^= ((k+1)<<j);
	    b[i] ^= ((k+1)>>(8-j));
	    hash(b, hlen, d);
	    /* check every bit is 1, 0, set, and not set at least once */
	    for (l=0; l<HASHSTATE; ++l)
	    {
	      e[l] &= (c[l]^d[l]);
	      f[l] &= ~(c[l]^d[l]);
	      g[l] &= c[l];
	      h[l] &= ~c[l];
	      x[l] &= d[l];
	      y[l] &= ~d[l];
	      if (e[l]|f[l]|g[l]|h[l]|x[l]|y[l]) finished=0;
	    }
	    if (finished) break;
	  }
	  if (k>z) z=k;
	  if (k==MAXPAIR) 
	  {
	    ub4 *bob;
	    for (l=0; l<HASHSTATE; ++l)
            {
              if (e[l]|f[l]|g[l]|h[l]|x[l]|y[l]) break;
            }
	    bob = e[l]?e:f[l]?f:g[l]?g:h[l]?h:x[l]?x:y;
            printf("Some bit didn't change: ");
	    for (l=0; l<HASHSTATE; ++l) printf("%.8lx ",bob[l]);
            printf("  i %ld j %ld len %ld\n",i,j,hlen);
	  }
	  if (z==MAXPAIR) goto done;
	}
      }
    }
   done:
    if (z < MAXPAIR)
    {
      printf("Mix success  %2ld bytes  %2ld levels  ",i,j);
      printf("required  %ld  trials\n",z/2);
    }
  }
}

/* Check for reading beyond the end of the buffer and alignment problems */
void driver3()
{
  ub1 buf[MAXLEN+20], *b;
  ub4 len;
  ub1 q[] = "This is the time for all good men to come to the aid of their country";
  ub1 qq[] = "xThis is the time for all good men to come to the aid of their country";
  ub1 qqq[] = "xxThis is the time for all good men to come to the aid of their country";
  ub1 qqqq[] = "xxxThis is the time for all good men to come to the aid of their country";
  ub4 h,i,j,ref[HASHSTATE],x[HASHSTATE],y[HASHSTATE];

  printf("Endianness.  These should all be the same:\n");
  for (j=0; j<HASHSTATE; ++j) ref[j] = (ub4)0;
  hash(q, sizeof(q)-1, ref);
  for (j=0; j<HASHSTATE; ++j) printf("%.8lx",ref[j]);
  printf("\n");
  for (j=0; j<HASHSTATE; ++j) ref[j] = (ub4)0;
  hash(qq+1, sizeof(q)-1, ref);
  for (j=0; j<HASHSTATE; ++j) printf("%.8lx",ref[j]);
  printf("\n");
  for (j=0; j<HASHSTATE; ++j) ref[j] = (ub4)0;
  hash(qqq+2, sizeof(q)-1, ref);
  for (j=0; j<HASHSTATE; ++j) printf("%.8lx",ref[j]);
  printf("\n");
  for (j=0; j<HASHSTATE; ++j) ref[j] = (ub4)0;
  hash(qqqq+3, sizeof(q)-1, ref);
  for (j=0; j<HASHSTATE; ++j) printf("%.8lx",ref[j]);
  printf("\n\n");

  for (h=0, b=buf+1; h<8; ++h, ++b)
  {
    for (i=0; i<MAXLEN; ++i)
    {
      len = i;
      for (j=0; j<MAXLEN+4; ++j) b[j]=0;

      /* these should all be equal */
      for (j=0; j<HASHSTATE; ++j) ref[j] = x[j] = y[j] = (ub4)0;
      hash(b, len, ref);
      b[i]=(ub1)~0;
      hash(b, len, x);
      hash(b, len, y);
      for (j=0; j<HASHSTATE; ++j)
      {
        if ((ref[j] != x[j]) || (ref[j] != y[j])) 
        {
  	  printf("alignment error: %.8lx %.8lx %.8lx %ld %ld\n",
                 ref[j],x[j],y[j],h,i);
        }
      }
    }
  }
}

/* check for problems with nulls */
 void driver4()
{
  ub1 buf[1];
  ub4 i,j,state[HASHSTATE];


  buf[0] = ~0;
  for (i=0; i<HASHSTATE; ++i) state[i] = 1;
  printf("These should all be different\n");
  for (i=0; i<8; ++i)
  {
    hash(buf, (ub4)0, state);
    printf("%2ld  strings  ",i);
    for (j=0; j<HASHSTATE; ++j) printf("%.8lx", state[j]);
    printf("\n");
  }
  printf("\n");
}


int main()
{
  driver1();   /* test that the key is hashed: used for timings */
  driver2();   /* test that whole key is hashed thoroughly */
  driver3();   /* test that nothing but the key is hashed */
  driver4();   /* test hashing multiple buffers (all buffers are null) */
  return 1;
}

#endif  /* SELF_TEST */
