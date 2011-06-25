/*
Copyright (c) 2009 Daniel Stahlke

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <map>
#include <vector>

#include <math.h>

// If this is defined (must be done before the #include), this demo
// will get the coordinates of a mouse click before exiting.
//#define GNUPLOT_ENABLE_PTY

#include "gnuplot-iostream.h"

int main() {
	// -persist option makes the window not disappear when your program exits
	Gnuplot gp("gnuplot -persist");
	// For debugging or manual editing of commands:
	//Gnuplot gp("cat > plot.gp");
	// or
	//Gnuplot gp("tee plot.gp | gnuplot -persist");

	// NOTE: we can use map here because the X values are intended to be
	// sorted.  If this was not the case, vector<pair<double,double>> could be
	// used instead.

	std::map<double, double> xy_pts_A;
	for(double x=-2; x<2; x+=0.01) {
		double y = x*x*x;
		xy_pts_A[x] = y;
	}

	std::map<double, double> xy_pts_B;
	for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
		double theta = alpha*2.0*3.14159;
		xy_pts_B[cos(theta)] = sin(theta);
	}

	// NOTE: mouse zoom functions don't work because gnuplot doesn't hold
	// on to the data.  If you are using gnuplot 4.4 or newer you can use
	// the 'volatile' keyword here to fix this.
	gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
	gp << "plot '-' with lines title 'cubic', '-' with points title 'circle'\n";
	gp.send(xy_pts_A).send(xy_pts_B);
	// Or this:
	//gp.send(xy_pts_A.begin(), xy_pts_A.end());
	//gp.send(xy_pts_B.begin(), xy_pts_B.end());

	#ifdef GNUPLOT_ENABLE_PTY
	double mx, my;
	int mb;
	gp.getMouse(mx, my, mb);
	printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
	#endif
}
