/* By Bob Jenkins, July 8 1995.  Public Domain.
** Given an input (stdin) sorted by line,
** produce an output (stdout) with no duplicate lines.
*/

#include <stdio.h>
#include <stddef.h>
#define LINELEN 128

int main()
{
   char buf[2][LINELEN];
   char *curr, *prev, *temp;

   curr = buf[0];
   prev = gets(buf[1]);
   temp = (char *)0;
   while (gets(curr))
   {
      if (strcmp(curr,prev)) printf("%s\n",prev);
      temp = curr;
      curr = prev;
      prev = temp;
   }
   if (prev) printf("%s\n",prev);
   
   return 1;
}
