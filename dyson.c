#include <math.h>

main()
{
  double twopi = 6.2831853;
  double i,a,b,c,d,theta,ratio;
  int j;

  j=2;
  ratio = 0.5;
  for (i=0.0; i<0.92; i+=0.1) {
    b = i;
    a = sqrt(1.0-b*b);
    d = i*ratio;
    c = sqrt(1.0-d*d);
    for (theta=0.0; theta < twopi; theta +=twopi/32.0) {
      printf("<param name=\"moon%d\" value=\"p(%g, %g, %g) v(%g, %g, 0.0) 0.0 ffffff 0.01\">\n",
	     j++, cos(theta)*a, sin(theta)*a, b, 
	     -d*cos(theta)-c*sin(theta), c*cos(theta)-d*sin(theta));
    }
  }
}
