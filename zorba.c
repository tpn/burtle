/*
-------------------------------------------------------------------------
A noncryptographic hash function producing up to 128 bit results.

There are three interfaces:
  * hash(), which just hashes a byte array
  * keyhash(), which hashes a byte array plus a key
  * init(), update(), final(), which let a message be hashed in pieces
The all produce the same values.

This hash is pretty specific to little-endian machines with SSE 
instructions, that is, recent x86 compatible chips.  For large messages 
it runs at about 3 bytes/cycle.

This implements 3 separate hashes: one for inputs 0..15 bytes long, one 
for 16..LARGE, and one beyond that.  The 16..LARGE uses 16-byte blocks and a 
16-byte internal state.  It xors the ith block to the state at steps 2i, 
2(i+6-4(i%2))+1, and 2(i+10)+1.  The macro CHURN() does two steps.  One 
step adds the state to itself a left shifted, the other shuffles the 
state and xors it to itself right shifted.  Once all uses of all blocks 
have been made, the 16-byte internal state is mixed until every output 
bit is affected.  The 0..15 hash is just that final mixing of a 16-byte 
state.  The LARGE+ hash is four independent copies of the 16..LARGE hash 
(operating on 16*4=64 byte chunks), with a different final mixing that 
mixes 4 16-byte states together until one 16-byte state has all output 
bits affected.

It's basically a space-time tradeoff.  By keeping a huge state so I can
use each block three times at staggered intervals, I can get away with
doing about two shifts per 16-byte block.  My cryptographic hash function
Maraca, and the way it was broken, were an exploration of that tradeoff.

By Bob Jenkins, Jan 1 2009, public domain
-------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stddef.h>
#include <windows.h>
#include "zorba.h"

#define LARGE 766

#define xor(a,b) _mm_xor_si128(a, b)
#define add(a,b) _mm_add_epi64(a, b)
#define shuf(a,b) _mm_shuffle_epi32(a, b)
#define lshift(a,b) _mm_slli_epi64(a, b)
#define rshift(a,b) _mm_srli_epi64(a, b)
#define read(x) _mm_load_si128((void *)x)

/* churn a state */
#define CHURN(s,first,second,third) \
{ \
  second=xor(first,second);		\
  s = xor(shuf(s,0x39),xor(rshift(s,5),first));	\
  s = xor(add(lshift(s,8),s),third);		   \
}

/* churn s, with second and third use of a block */
#define TAIL1(s,third) \
{ \
  s = xor(shuf(s,0x39), rshift(s,5)); \
  s = add(s, xor(lshift(s,8), third)); \
}

/* strength: I had to test this without the first or last line */
#define FINAL1(s) \
{ \
  s = xor(shuf(s,0x1b), rshift(s,1)); \
  s = add(s, lshift(s,17)); \
  s = xor(shuf(s,0x1b), rshift(s,13)); \
  s = add(s, lshift(s,8)); \
  s = xor(shuf(s,0x1b), rshift(s,2)); \
  s = add(s, lshift(s,28)); \
  s = xor(shuf(s,0x1b), rshift(s,16)); \
  s = add(s, lshift(s,4)); \
  s = xor(shuf(s,0x1b), rshift(s,6)); \
  s = add(s, lshift(s,9)); \
}

#define FINAL4(s) \
{ \
  s1 = xor(shuf(s1,0x1b), s2); \
  s3 = xor(shuf(s3,0x1b), s0); \
  s2 = add(shuf(s2,0x39), lshift(s1,6)); \
  s0 = add(shuf(s0,0x39), lshift(s3,6)); \
  s1 = xor(shuf(s1,0x1b), s0); \
  s2 = xor(shuf(s2,0x1b), s3); \
  s3 = add(shuf(s3,0x39), lshift(s2,19)); \
  s0 = add(shuf(s0,0x39), lshift(s1,19)); \
  s1 = xor(shuf(s1,0x1b), s3); \
  s2 = xor(shuf(s2,0x1b), s0); \
  s3 = add(shuf(s3,0x39), lshift(s1,9)); \
  s0 = add(shuf(s0,0x39), lshift(s2,9)); \
  s1 = xor(shuf(s1,0x1b), s2); \
  s3 = xor(shuf(s3,0x1b), s0); \
  s0 = add(shuf(s0,0x39), lshift(s3,5)); \
  s1 = xor(shuf(s1,0x1b), s0); \
  s0 = add(shuf(s0,0x39), s1); \
}

/* churn four independent states */
#define CHURN4(data,i,q,s,x,y,z) \
{ \
    x##0 = read(&data[(i)+(q)  ]); \
    x##1 = read(&data[(i)+(q)+1]); \
    x##2 = read(&data[(i)+(q)+2]); \
    x##3 = read(&data[(i)+(q)+3]); \
    CHURN(s##0,x##0,y##0,z##0); \
    CHURN(s##1,x##1,y##1,z##1); \
    CHURN(s##2,x##2,y##2,z##2); \
    CHURN(s##3,x##3,y##3,z##3); \
 }

/* churn one independent state */
#define CHURN1(data,i,q,s,x,y,z) \
{ \
    x = read(&data[(i)+(q)]); \
    CHURN(s,x,y,z); \
}

#define TAIL4(s,third) { \
    TAIL1(s##0,third##0); \
    TAIL1(s##1,third##1); \
    TAIL1(s##2,third##2); \
    TAIL1(s##3,third##3); \
}

#define TO_REG(a,m,i) { \
    a##0 = m[i  ].h; \
    a##1 = m[i+1].h; \
    a##2 = m[i+2].h; \
    a##3 = m[i+3].h; \
}

#define FROM_REG(a,m,i) { \
    m[i  ].h = a##0; \
    m[i+1].h = a##1; \
    m[i+2].h = a##2; \
    m[i+3].h = a##3; \
}

#define REG_REG(a,b) { \
  a##0 = b##0; \
  a##1 = b##1; \
  a##2 = b##2; \
  a##3 = b##3; \
}


/* initialize a zorba state */
void init(zorba *z)
{
  memset(z->s, 0x55, sizeof(z->s));
  memset(z->accum, 0, sizeof(z->accum));
  z->datalen = 0;
  z->messagelen = (u8)0;
}


/* hash a piece of a message */
void update(zorba *z, const void *data, size_t len)
{
  int counter;
  int oldlen = z->datalen;
  size_t total = oldlen + len;
  __m128i s0,a0,b0,c0,d0,e0,f0,g0,h0,i0,j0,k0,l0;
  __m128i s1,a1,b1,c1,d1,e1,f1,g1,h1,i1,j1,k1,l1;
  __m128i s2,a2,b2,c2,d2,e2,f2,g2,h2,i2,j2,k2,l2;
  __m128i s3,a3,b3,c3,d3,e3,f3,g3,h3,i3,j3,k3,l3;

  /* exit early if we don't have a complete block */
  z->messagelen += len;
  if (total < BUFFERED) {
    memcpy(((char *)z->data)+oldlen, data, len);
    z->datalen = total;
    return;
  }

  /* load the registers */
  TO_REG(s,z->s,0);
  TO_REG(l,z->accum,0);
  TO_REG(k,z->accum,4);
  TO_REG(j,z->accum,8);
  TO_REG(i,z->accum,12);
  TO_REG(h,z->accum,16);
  TO_REG(g,z->accum,20);
  TO_REG(f,z->accum,24);
  TO_REG(e,z->accum,28);
  TO_REG(d,z->accum,32);
  TO_REG(c,z->accum,36);

  /* use the first cached block, if any */
  if (oldlen) {
    __m128i *cache = (__m128i *)z->data;
    int piece = BUFFERED-oldlen;
    memcpy(((char *)cache)+oldlen, data, piece);
    CHURN4(cache,0, 0,s,b,h,l);
    CHURN4(cache,0, 4,s,a,c,k);
    CHURN4(cache,0, 8,s,l,f,j);
    CHURN4(cache,0,12,s,k,a,i);
    CHURN4(cache,0,16,s,j,d,h);
    CHURN4(cache,0,20,s,i,k,g);
    CHURN4(cache,0,24,s,h,b,f);
    CHURN4(cache,0,28,s,g,i,e);
    CHURN4(cache,0,32,s,f,l,d);
    CHURN4(cache,0,36,s,e,g,c);
    CHURN4(cache,0,40,s,d,j,b);
    CHURN4(cache,0,44,s,c,e,a);
    len -= piece;
  }

  /* use any other complete blocks */
  if ((((size_t)data) & 15) == 0) {
    const __m128i *aligned_data = (const __m128i *)data;
    int len2 = len/16;
    for (counter=BLOCK; counter<=len2; counter+=BLOCK) {
      CHURN4(aligned_data,counter,-48,s,b,h,l);
      CHURN4(aligned_data,counter,-44,s,a,i,k);
      CHURN4(aligned_data,counter,-40,s,l,f,j);
      CHURN4(aligned_data,counter,-36,s,k,g,i);
      CHURN4(aligned_data,counter,-32,s,j,d,h);
      CHURN4(aligned_data,counter,-28,s,i,e,g);
      CHURN4(aligned_data,counter,-24,s,h,b,f);
      CHURN4(aligned_data,counter,-20,s,g,c,e);
      CHURN4(aligned_data,counter,-16,s,f,l,d);
      CHURN4(aligned_data,counter,-12,s,e,a,c);
      CHURN4(aligned_data,counter, -8,s,d,j,b);
      CHURN4(aligned_data,counter, -4,s,c,k,a);
    }
    counter *= 16;
  } else {
    for (counter=BUFFERED; counter<=len; counter+=BUFFERED) {
      __m128i *cache = (__m128i *)z->data;
      memcpy(cache, ((const char *)data)+(counter-BUFFERED), BUFFERED);
      CHURN4(cache,0, 0,s,b,h,l);
      CHURN4(cache,0, 4,s,a,i,k);
      CHURN4(cache,0, 8,s,l,f,j);
      CHURN4(cache,0,12,s,k,g,i);
      CHURN4(cache,0,16,s,j,d,h);
      CHURN4(cache,0,20,s,i,e,g);
      CHURN4(cache,0,24,s,h,b,f);
      CHURN4(cache,0,28,s,g,c,e);
      CHURN4(cache,0,32,s,f,l,d);
      CHURN4(cache,0,36,s,e,a,c);
      CHURN4(cache,0,40,s,d,j,b);
      CHURN4(cache,0,44,s,c,k,a);
    }
  }

  /* store the registers */  
  FROM_REG(s,z->s,0);
  FROM_REG(l,z->accum,0);
  FROM_REG(k,z->accum,4);
  FROM_REG(j,z->accum,8);
  FROM_REG(i,z->accum,12);
  FROM_REG(h,z->accum,16);
  FROM_REG(g,z->accum,20);
  FROM_REG(f,z->accum,24);
  FROM_REG(e,z->accum,28);
  FROM_REG(d,z->accum,32);
  FROM_REG(c,z->accum,36);

  /* cache the last partial block, if any */
  len = BUFFERED + len - counter;
  if (len) {
    memcpy(z->data, ((const char *)data)+counter-BUFFERED, len);
    z->datalen = len;
  }

}

/* midhash assumes the message is aligned to a 16-byte boundary */
z128 midhash( const void *message, size_t mlen, const void *key, size_t klen)
{
  __m128i s,a,b,c,d,e,f,g,h,i,j,k,l;
  __m128i *cache = (__m128i *)message;
  z128 val;
  int total = mlen + klen;
  int counter;
  
  if (klen > 16) {
    printf("key length must be 16 bytes or less\n");
    exit(1);
  }

  /* add the key and length; pad to a multiple of 16 bytes */
  memcpy(((char *)cache)+mlen, key, klen);
  ((char *)cache)[total] = klen;
  ((char *)cache)[total+1] = mlen+1;
  total += 2;
  if (total&15) {
    int tail = 16 - (total&15);
    memset(((char *)cache)+total, 0, tail);
    total += tail;
  }
  
  /* hash 32-byte blocks */
  s = _mm_set_epi32(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
  c=d=e=f=g=h=i=j=k=l=s; 
  for (counter=2; counter<=total/16; counter+=2) {
    CHURN1(cache,counter,-2,s,b,h,l);
    CHURN1(cache,counter,-1,s,a,i,k);
    l=j; k=i; j=h; i=g; h=f; g=e; f=d; e=c; d=b; c=a;
  }
  
  /* possibly handle trailing 16-byte block, then use up accumulators */
  if (counter != total/16) {
    CHURN1(cache,counter,-2,s,b,h,l);
    TAIL1(s,k);
    TAIL1(s,j);
    TAIL1(s,i);
    TAIL1(s,h);
    TAIL1(s,g);
    TAIL1(s,f);
    TAIL1(s,e);
    TAIL1(s,d);
    TAIL1(s,c);
    TAIL1(s,b);
  } else {
    TAIL1(s,l);
    TAIL1(s,k);
    TAIL1(s,j);
    TAIL1(s,i);
    TAIL1(s,h);
    TAIL1(s,g);
    TAIL1(s,f);
    TAIL1(s,e);
    TAIL1(s,d);
    TAIL1(s,c);
  }
  
  /* final mixing */
  FINAL1(s);
  val.h = s;
  return val;
  
}

/*
 * Compute a hash for the total message
 * Computing the hash and using the key are done only privately
 * This: init() update() final(key1) final(key2) final(key3)
 *   is the same as: init() update() final(key3)
 */
z128 final(zorba *z, const void *key, size_t klen)
{
  u8 total;
  unsigned short lengths;
  unsigned short int field;
  z128 val;

  /* use the key and the lengths */
  total = z->messagelen + klen;
  if (total <= 15) {

    /* one state, just finalization */
    val.h = _mm_set_epi32(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
    memcpy(((char *)val.x), (char *)z->data, z->datalen);
    memcpy(((char *)val.x)+z->datalen, key, klen);
    ((char *)val.x)[15] = 1 + z->datalen + (klen << 4);
    FINAL1(val.h);
    return val;

  } 

  else if (total <= LARGE) {

    return midhash(z->data, z->datalen, key, klen);

  } else {

    /* four states, churn in parallel, finalization mixes those 4 states */
    __m128i s0,a0,b0,c0,d0,e0,f0,g0,h0,i0,j0,k0,l0;
    __m128i s1,a1,b1,c1,d1,e1,f1,g1,h1,i1,j1,k1,l1;
    __m128i s2,a2,b2,c2,d2,e2,f2,g2,h2,i2,j2,k2,l2;
    __m128i s3,a3,b3,c3,d3,e3,f3,g3,h3,i3,j3,k3,l3;
    __m128i *cache = (__m128i *)z->data;
    int counter;

    if (klen > 16) {
      printf("key length must be 16 bytes or less\n");
      exit(1);
    }

    total = z->datalen + klen;

    /* load the registers */
    TO_REG(s,z->s,0);
    TO_REG(l,z->accum,0);
    TO_REG(k,z->accum,4);
    TO_REG(j,z->accum,8);
    TO_REG(i,z->accum,12);
    TO_REG(h,z->accum,16);
    TO_REG(g,z->accum,20);
    TO_REG(f,z->accum,24);
    TO_REG(e,z->accum,28);
    TO_REG(d,z->accum,32);
    TO_REG(c,z->accum,36);
    
    /* add the key and length; pad to a multiple of 64 bytes */
    memcpy(((char *)z->data)+z->datalen, key, klen);
    ((char *)z->data)[total] = klen;
    ((char *)z->data)[total+1] = (z->datalen&63)+1;
    total += 2;
    if (total&63) {
      int tail = 64 - (total&63);
      memset(((char *)z->data)+total, 0, tail);
      total += tail;
    }

    /* consume all remaining 128-byte blocks */

    for (counter=8; counter<=total/16; counter += 8) {
      CHURN4(cache,counter,-8,s,b,h,l);
      CHURN4(cache,counter,-4,s,a,i,k);
      REG_REG(l,j); REG_REG(k,i); REG_REG(j,h); REG_REG(i,g); REG_REG(h,f);
      REG_REG(g,e); REG_REG(f,d); REG_REG(e,c); REG_REG(d,b); REG_REG(c,a);
    }
    
    /* possibly another 64-byte chunk, then consume accumulators */
    if (counter != total/16) {
      CHURN4(cache,counter,-8,s,b,h,l);
      TAIL4(s,k);
      TAIL4(s,j);
      TAIL4(s,i);
      TAIL4(s,h);
      TAIL4(s,g);
      TAIL4(s,f);
      TAIL4(s,e);
      TAIL4(s,d);
      TAIL4(s,c);
      TAIL4(s,b);
    } else {
      TAIL4(s,l);
      TAIL4(s,k);
      TAIL4(s,j);
      TAIL4(s,i);
      TAIL4(s,h);
      TAIL4(s,g);
      TAIL4(s,f);
      TAIL4(s,e);
      TAIL4(s,d);
      TAIL4(s,c);
    }
    
    /* mix s0..s3 to produce the final result */
    FINAL4(s);
    val.h = s0;
    return val;

  }

}



/* hash a message all at once, with a key */
z128 keyhash( const void *message, size_t mlen, const void *key, size_t klen)
{
  size_t len = mlen + klen;
  if (len <= 15) {

    /* really short message */
    z128 val;
    val.h = _mm_set_epi32(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
    memcpy(((char *)val.x), message, mlen);
    memcpy(((char *)val.x)+mlen, key, klen);
    ((char *)val.x)[15] = 1 + mlen + (klen << 4);
    FINAL1(val.h);
    return val;

  } else if (mlen <= LARGE) {
    
    __m128i buf[BLOCK];
    buf[mlen/16] = _mm_setzero_si128();
    memcpy(buf, message, mlen);
    return midhash( buf, mlen, key, klen);

  } else {

    /* long message: do it the normal way */
    zorba z;
    init(&z);
    update(&z, message, mlen);
    return final(&z, key, klen);

  }
}



/* hash a message all at once, without a key */
z128 hash( const void *message, size_t mlen)
{
  if (mlen <= 15) {

    /* really short message */
    z128 val;
    val.h = _mm_set_epi32(0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef);
    memcpy(((char *)val.x), message, mlen);
    ((char *)val.x)[15] = 1 + mlen;
    FINAL1(val.h);
    return val;

  } else if (mlen <= LARGE) {
    
    char key[1];
    __m128i buf[BLOCK];
    buf[mlen/16] = _mm_setzero_si128();
    memcpy(buf, message, mlen);
    return midhash( buf, mlen, key, 0);

  } else {

    /* long message: do it the normal way */
    zorba z;
    char key[1];
    init(&z);
    update(&z, message, mlen);
    return final(&z, key, 0);

  }
}

/* check that every input bit changes every output bit half the time */
#define HASHSTATE 1
#define MAXPAIR 40
#define MAXLEN  1000
void driver2()
{
  u1 qa[MAXLEN+1], qb[MAXLEN+2], *a = &qa[0], *b = &qb[1], m=0;
  u8 c[HASHSTATE], d[HASHSTATE], i=0, j=0, k, l, z;
  u8 e[HASHSTATE],f[HASHSTATE],g[HASHSTATE],h[HASHSTATE];
  u8 x[HASHSTATE],y[HASHSTATE];
  u4 hlen;

  printf("No more than %d trials should ever be needed \n",MAXPAIR);
  for (hlen=0; hlen < MAXLEN; ++hlen)
  {
    z=0;
    for (i=0; i<hlen; ++i)  /*----------------------- for each input byte, */
    {
      for (j=0; j<8; ++j)   /*------------------------ for each input bit, */
      {
	for (m=1; m<8; ++m) /*------------ for serveral possible initvals, */
	{
	  for (l=0; l<HASHSTATE; ++l)
	    e[l]=f[l]=g[l]=h[l]=x[l]=y[l]=~((u8)0);

      	  /*---- check that every output bit is affected by that input bit */
	  for (k=0; k<2*MAXPAIR; k+=2)
	  { 
	    u4 finished=1;
	    /* keys have one bit different */
	    for (l=0; l<hlen+1; ++l) {a[l] = b[l] = (u1)0;}
	    /* have a and b be two keys differing in only one bit */
	    a[i] ^= (k<<j);
	    a[i] ^= (k>>(8-j));
	    c[0] = keyhash64(a, hlen, &m, 1);
	    b[i] ^= ((k+1)<<j);
	    b[i] ^= ((k+1)>>(8-j));
	    d[0] = keyhash64(b, hlen, &m, 1);
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
	  if (k==2*MAXPAIR) 
	  {
	     printf("Some bit didn't change: ");
	     printf("%.16llx %.16llx %.16llx %.16llx %.16llx %.16llx  ",
	            e[0],f[0],g[0],h[0],x[0],y[0]);
	     printf("i %d j %d m %d len %d\n", (u4)i, (u4)j, (u4)m, (u4)hlen);
	  }
	  if (z==2*MAXPAIR) goto done;
	}
      }
    }
   done:
    if (z < 2*MAXPAIR)
    {
      printf("Mix success  %2d bytes  %2d initvals  ",i,m);
      printf("required  %d  trials\n", z/2);
    }
  }
  printf("\n");
}



#define HASH_LITTLE_ENDIAN 1
#define hashsize(n) ((u4)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) ^ ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

u4 hashlittle( const void *key, size_t length, u4 initval)
{
  u4 a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((u4)length) + initval;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const u4 *k = key;                           /* read 32-bit chunks */
    const u1  *k8;

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }


  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const u2 *k = key;                           /* read 16-bit chunks */
    const u1  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((u4)k[1])<<16);
      b += k[2] + (((u4)k[3])<<16);
      c += k[4] + (((u4)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const u1 *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((u4)k[5])<<16);
             b+=k[2]+(((u4)k[3])<<16);
             a+=k[0]+(((u4)k[1])<<16);
             break;
    case 11: c+=((u4)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((u4)k[3])<<16);
             a+=k[0]+(((u4)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((u4)k[3])<<16);
             a+=k[0]+(((u4)k[1])<<16);
             break;
    case 7 : b+=((u4)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((u4)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((u4)k[1])<<16);
             break;
    case 3 : a+=((u4)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : return c;                     /* zero length requires no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const u1 *k = key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((u4)k[1])<<8;
      a += ((u4)k[2])<<16;
      a += ((u4)k[3])<<24;
      b += k[4];
      b += ((u4)k[5])<<8;
      b += ((u4)k[6])<<16;
      b += ((u4)k[7])<<24;
      c += k[8];
      c += ((u4)k[9])<<8;
      c += ((u4)k[10])<<16;
      c += ((u4)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((u4)k[11])<<24;
    case 11: c+=((u4)k[10])<<16;
    case 10: c+=((u4)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((u4)k[7])<<24;
    case 7 : b+=((u4)k[6])<<16;
    case 6 : b+=((u4)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((u4)k[3])<<24;
    case 3 : a+=((u4)k[2])<<16;
    case 2 : a+=((u4)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}




#define MSIZE (1<<12)
int main()
{
  int i;
  u8 a,z;
  u8 val;
  u4 val4;
  __m128i message[MSIZE+1];
  zorba zzz;

  memset(message, 42, sizeof(message));
  a = GetTickCount();
  for (i=0; i<(1<<20); ++i) {
    val4 += hashlittle(message, 200, 0);
  }
#ifdef NEVER
  init(&zzz);
  for (i=0; i<(1<<18); ++i) {
    update(&zzz, &((char *)message)[0], 16*MSIZE);
  }
  val = final(&zzz, message, 0);
  for (i=0; i<MSIZE; ++i) {
    ((int *)message)[i] = i;
  }
  val = hash(message, 770);
  driver2();
#endif
  z = GetTickCount();
  printf("hi bob %d %.8lx\n", 
	 (u4)(z-a), val4);
}

