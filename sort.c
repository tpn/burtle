/*
----------------------------------------------------------------------------
 By Bob Jenkins, July 8 1995.  Sort any length file by line.  Public Domain

 This would be 4x faster if it merged 16 subfiles at a time
 rather than just two.
----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stddef.h>

#define FNAMLEN   16
#define LINELEN  128
#define LSORTMAX 255
#define MAXDEP    31

void merge(a,b,c)
FILE *a;  /* first input file */
FILE *b;  /* second input file */
FILE *c;  /* third input file */
{
   char ax[LINELEN],bx[LINELEN];
   register char *ay,*by;

   ay = fgets(ax,LINELEN,a);
   by = fgets(bx,LINELEN,b);
   while (ay && by)
   {
      if (strcmp(ay,by) < 0)
      {
         fprintf(c,"%s",ay);
         if (!fgets(ay,LINELEN,a)) ay = (char *)0;
      }
      else
      {
         fprintf(c,"%s",by);
         if (!fgets(by,LINELEN,b)) by = (char *)0;
      }
   }

   if (!ay)
   {
      do {fprintf(c,"%s",by);} while (fgets(by,LINELEN,b));
   }
   else
   {
      do {fprintf(c,"%s",ay);} while (fgets(ay,LINELEN,a));
   }
}

/* read LSORTMAX lines and write them, sorted, to another file. */
/* return TRUE if the end of the input is reached. */
int littlesort(inp,oup)
FILE *inp;  /* opened file to read unsorted lines in from */
FILE *oup;  /* opened file to write sorted lines out to */
{
   register int  i,j,k,lines;
   char  lbuf[LSORTMAX][LINELEN],*last;
   char *l[LSORTMAX];

   for (lines=0;
        (lines<LSORTMAX) && (last=fgets(lbuf[lines],LINELEN,inp));
        ++lines)
      ;

   for (i=0; i<lines; ++i) l[i] = lbuf[i];

   for (i=LSORTMAX>>1; i; i>>=1)
   {
      for (j=0; j<i; ++j)
      {
         register int swap = 1;
         while (swap)
         {
            swap = 0;
            for (k=j+i; k<lines; k+=i)
            {
               if (strcmp(l[k],l[k-i]) < 0)
               {
                  char *temp = l[k];
                  l[k] = l[k-i];
                  l[k-i] = temp;
                  swap = 1;
               }
            }
         }
      }
   }

   for (i=0; i<lines; ++i) fprintf(oup,"%s",l[i]);

   return (!last);
}

static char fntmpl[] = "tmp%dx%d.dat";

int main()
{
   FILE              *f[MAXDEP];
   char               fname[MAXDEP][2][FNAMLEN];
   char              *tempname;
   char               s[LINELEN];
   FILE              *a;
   FILE              *b;
   FILE              *c;
   unsigned long int  i,j;
   int                done;

   /* make up names for the temporary files, two at each level */
   for (i=0; i<MAXDEP; ++i)
   {
      sprintf(fname[i][0],fntmpl,i,0);
      sprintf(fname[i][1],fntmpl,i,1);
   }

   /* read in the input and sort it */
   for (i=0, done=0; !done; ++i)
   {
      int mergepairs;

      /* write the zero-level sorted file */
      a = fopen(fname[0][i&1],"w");
      done=littlesort(stdin,a);
      fclose(a);

      /* find out how many complete pairs of files we have now */
      for (mergepairs=0, j=i; j&1; j>>=1, ++mergepairs)
         ;
      /* merge all the pairs we have */
      for (j=0; j<mergepairs; ++j)
      {
         a = fopen(fname[j][0],"r");
         b = fopen(fname[j][1],"r");
         c = fopen(fname[j+1][!!(i&(1<<(j+1)))],"w");
         merge(a,b,c);
         fclose(a);
         fclose(b);
         fclose(c);
      }
   }

   /* we may have some uncompleted pairs left to merge */
   for (j=0, tempname=(char *)0; (((unsigned long int)1)<<j)<i; ++j)
   {
      if (i&(1<<j))
      {
         if (!tempname) tempname = fname[j][0];
         else
         {
            a = fopen(tempname,"r");
            b = fopen(fname[j][0],"r");
            c = fopen(fname[j][1],"w");
            merge(a,b,c);
            fclose(a);
            fclose(b);
            fclose(c);
            tempname = fname[j][1];
         }
      }
   }
   /* if i was 7 (111), the only file is under 8 (1000) */
   if (!tempname) tempname = fname[j][0];

   /* report the results to the user */
   a = fopen(tempname,"r");
   while (fgets(s,LINELEN,a)) fprintf(stdout,"%s",s);
   fclose(a);

   return 1;
}
