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

//#define GNUPLOT_ENABLE_BLITZ
#include "gnuplot-iostream.h"

int main() {
	Gnuplot gp;
	// For debugging or manual editing of commands:
	//Gnuplot gp("cat > plot.gp");
	// or
	//Gnuplot gp("tee plot.gp | gnuplot");

	gp << "set terminal png\n";

	std::vector<double> y_pts;
	for(int i=0; i<1000; i++) {
		double y = (i/500.0-1) * (i/500.0-1);
		y_pts.push_back(y);
	}

	gp << "set output 'my_graph_1.png'\n";
	gp << "plot '-' with lines, sin(x/200) with lines\n";
	gp.send(y_pts);
	// Or this:
	//gp.send(y_pts.begin(), y_pts.end());

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

	gp << "set output 'my_graph_2.png'\n";
	gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
	gp << "plot '-' with lines title 'cubic', '-' with points title 'circle'\n";
	gp.send(xy_pts_A).send(xy_pts_B);
	// Or this:
	//gp.send(xy_pts_A.begin(), xy_pts_A.end());
	//gp.send(xy_pts_B.begin(), xy_pts_B.end());

#ifdef GNUPLOT_ENABLE_BLITZ
	gp << "set output 'my_graph_3.png'\n";
	gp << "set auto x\n";
	gp << "set auto y\n";
	blitz::Array<double, 1> y_arr(100);
	blitz::Array<blitz::TinyVector<double, 2>, 1> xy_arr(100);
	blitz::firstIndex bi;
	y_arr = cos(bi * 0.1);
	xy_arr[0] = cos(bi * 0.1) * 100;
	xy_arr[1] = sin(bi * 0.1);
	gp << "plot '-' with lines, '-' with lines\n";
	gp.send(y_arr);
	gp.send(xy_arr);
#endif // GNUPLOT_ENABLE_BLITZ

	// If this was an interactive session (i.e. not a png terminal),
	// this would get a mouse press:
	//double mx, my;
	//int mb;
	//gp.getMouse(mx, my, mb);
	//printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
}
