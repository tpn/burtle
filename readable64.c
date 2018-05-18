/*
------------------------------------------------------------------------------
readable.c: My random number generator, ISAAC.
(c) Bob Jenkins, March 1996, Public Domain
You may use this code in any way you wish, and it is free.  No warrantee.
------------------------------------------------------------------------------
*/

/* a ub8 is an unsigned 8-byte quantity */
typedef  unsigned long long  ub8;
typedef  unsigned long int   ub4;

/* external results */
ub8 randrsl[256], randcnt;

/* internal state */
static    ub8 mm[256];
static    ub8 aa=0, bb=0, cc=0;


void isaac()
{
   ub8 i,x,y;

   cc = cc + 1;    /* cc just gets incremented once per 256 results */
   bb = bb + cc;   /* then combined with bb */

   for (i=0; i<256; ++i)
   {
     x = mm[i];
     switch (i%4)
     {
     case 0: aa = ~(aa^(aa<<21)); break;
     case 1: aa = aa^(aa>>5); break;
     case 2: aa = aa^(aa<<12); break;
     case 3: aa = aa^(aa>>33); break;
     }
     aa              = mm[(i+128)%256] + aa;
     mm[i]      = y  = mm[(x>>3)%256] + aa + bb;
     randrsl[i] = bb = mm[(y>>11)%256] + x;

     /* Note that bits 3..10 are chosen from x but 11..18 are chosen
        from y.  The only important thing here is that 3..10 and 11..18
        don't overlap.  3..10 and 11..18 were then chosen for speed in
        the optimized version (isaac64.c) */
     /* See http://burtleburtle.net/bob/rand/isaac.html
        for further explanations and analysis. */
   }
}


/* if (flag==TRUE), then use the contents of randrsl[] to initialize mm[]. */
#define mix(a,b,c,d,e,f,g,h) \
{ \
   a-=e; f^=h>>9;  h+=a; \
   b-=f; g^=a<<9;  a+=b; \
   c-=g; h^=b>>23; b+=c; \
   d-=h; a^=c<<15; c+=d; \
   e-=a; b^=d>>14; d+=e; \
   f-=b; c^=e<<20; e+=f; \
   g-=c; d^=f>>17; f+=g; \
   h-=d; e^=g<<14; g+=h; \
}

void randinit(int flag)
{
   int i;
   ub8 a,b,c,d,e,f,g,h;
   aa=bb=cc=0;
   a=b=c=d=e=f=g=h=0x9e3779b97f4a7c13LL;  /* the golden ratio */

   for (i=0; i<4; ++i)          /* scramble it */
   {
     mix(a,b,c,d,e,f,g,h);
   }

   for (i=0; i<256; i+=8)   /* fill in mm[] with messy stuff */
   {
     if (flag)                  /* use all the information in the seed */
     {
       a+=randrsl[i  ]; b+=randrsl[i+1]; c+=randrsl[i+2]; d+=randrsl[i+3];
       e+=randrsl[i+4]; f+=randrsl[i+5]; g+=randrsl[i+6]; h+=randrsl[i+7];
     }
     mix(a,b,c,d,e,f,g,h);
     mm[i  ]=a; mm[i+1]=b; mm[i+2]=c; mm[i+3]=d;
     mm[i+4]=e; mm[i+5]=f; mm[i+6]=g; mm[i+7]=h;
   }

   if (flag)
   {        /* do a second pass to make all of the seed affect all of mm */
     for (i=0; i<256; i+=8)
     {
       a+=mm[i  ]; b+=mm[i+1]; c+=mm[i+2]; d+=mm[i+3];
       e+=mm[i+4]; f+=mm[i+5]; g+=mm[i+6]; h+=mm[i+7];
       mix(a,b,c,d,e,f,g,h);
       mm[i  ]=a; mm[i+1]=b; mm[i+2]=c; mm[i+3]=d;
       mm[i+4]=e; mm[i+5]=f; mm[i+6]=g; mm[i+7]=h;
     }
   }

   isaac();            /* fill in the first set of results */
   randcnt=256;        /* prepare to use the first set of results */
}


int main()
{
  ub8 i,j;
  aa=bb=cc=(ub8)0;
  for (i=0; i<256; ++i) mm[i]=randrsl[i]=(ub8)0;
  randinit(1);
  for (i=0; i<2; ++i)
  {
    isaac();
    for (j=0; j<256; ++j)
    {
      printf("%.8x%.8x",(ub4)(randrsl[j]>>32), (ub4)randrsl[j]);
      if ((j&3)==3) printf("\n");
    }
  }
}
