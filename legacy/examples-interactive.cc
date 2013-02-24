/*
Copyright (c) 2013 Daniel Stahlke

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

#include <vector>
#include <math.h>

#define GNUPLOT_ENABLE_PTY
#include "gnuplot-iostream.h"

// Yes, I'm including a *.cc file.  It contains main().
#include "examples-framework.cc"

void demo_basic() {
	Gnuplot gp;

	double mx=0, my=0;
	int mb=1;
	while(mb != 3 && mb >= 0) {
		std::vector<std::pair<double, double> > xy_pts;
		xy_pts.push_back(std::make_pair(mx, my));
		for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
			double theta = alpha*2.0*3.14159;
			xy_pts.push_back(std::make_pair(
				mx+cos(theta), my+sin(theta)));
		}

		gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
		gp << "plot '-' with points title 'circle'\n";
		gp.send(xy_pts);

		gp.getMouse(mx, my, mb, "Left click to move circle, right click to exit.");
		printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
		if(mb < 0) printf("The gnuplot window was closed.\n");
	}
}

void demo_vectors() {
	Gnuplot gp;

	double mx=0, my=0;
	int mb=1;
	while(mb != 3 && mb >= 0) {
		std::vector<std::vector<float> > vecs(4);
		for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
			double theta = alpha*2.0*3.14159;
			vecs[0].push_back(mx+cos(theta));
			vecs[1].push_back(my+sin(theta));
			vecs[2].push_back(-cos(theta)*0.1);
			vecs[3].push_back(-sin(theta)*0.1);
		}

		gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
		gp << "plot '-' with vectors title 'circle'\n";
		gp.send(vecs);

		gp.getMouse(mx, my, mb, "Left click to move circle, right click to exit.");
		printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
		if(mb < 0) printf("The gnuplot window was closed.\n");
	}
}

void register_demos() {
	register_demo("basic",                  demo_basic);
	register_demo("vectors",                demo_vectors);
}
