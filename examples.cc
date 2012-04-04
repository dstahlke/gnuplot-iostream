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
#include "boost/foreach.hpp"

// FIXME - detect availability
#define GNUPLOT_ENABLE_PTY
// FIXME - detect availability
#define GNUPLOT_ENABLE_BLITZ

#ifdef GNUPLOT_ENABLE_BLITZ
#include <blitz/array.h>
#endif // GNUPLOT_ENABLE_BLITZ

#include "gnuplot-iostream.h"

void demo_basic() {
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

	gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
	gp << "plot '-' with lines title 'cubic', '-' with points title 'circle'\n";
	gp.send(xy_pts_A).send(xy_pts_B);
}

#ifdef GNUPLOT_ENABLE_PTY
void demo_interactive() {
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
#endif // GNUPLOT_ENABLE_PTY

void demo_png() {
	Gnuplot gp;

	gp << "set terminal png\n";

	std::vector<double> y_pts;
	for(int i=0; i<1000; i++) {
		double y = (i/500.0-1) * (i/500.0-1);
		y_pts.push_back(y);
	}

	std::cout << "Creating my_graph_1.png" << std::endl;
	gp << "set output 'my_graph_1.png'\n";
	gp << "plot '-' with lines, sin(x/200) with lines\n";
	gp.send(y_pts);

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

	std::cout << "Creating my_graph_2.png" << std::endl;
	gp << "set output 'my_graph_2.png'\n";
	gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
	gp << "plot '-' with lines title 'cubic', '-' with points title 'circle'\n";
	gp.send(xy_pts_A).send(xy_pts_B);
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

#ifdef GNUPLOT_ENABLE_PTY
		gp.getMouse(mx, my, mb, "Left click to move circle, right click to exit.");
		printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
		if(mb < 0) printf("The gnuplot window was closed.\n");
#else
		break;
#endif // GNUPLOT_ENABLE_PTY
	}
}

#ifdef GNUPLOT_ENABLE_BLITZ
void demo_blitz() {
	// -persist option makes the window not disappear when your program exits
	Gnuplot gp("gnuplot -persist");

	blitz::Array<double, 2> arr(100, 100);
	{
		blitz::firstIndex i;
		blitz::secondIndex j;
		arr = (i-50) * (j-50);
	}
	gp << "set pm3d map; set palette" << std::endl;
	gp << "splot '-'" << std::endl;
	gp.send(arr);
}
#endif // GNUPLOT_ENABLE_BLITZ

int main(int argc, char **argv) {
	std::map<std::string, void (*)(void)> demos;
	demos["basic"] = demo_basic;
#ifdef GNUPLOT_ENABLE_PTY
	demos["interactive"] = demo_interactive;
#endif
	demos["png"] = demo_png;
	demos["vectors"] = demo_vectors;
#ifdef GNUPLOT_ENABLE_BLITZ
	demos["blitz"] = demo_blitz;
#endif // GNUPLOT_ENABLE_BLITZ

	if(argc < 2) {
		printf("Usage: %s <demo_name>\n", argv[0]);
		printf("Choose one of the following demos:\n");
		typedef std::pair<std::string, void (*)(void)> demo_pair;
		BOOST_FOREACH(const demo_pair &pair, demos) {
			printf("    %s\n", pair.first.c_str());
		}
		return 0;
	}

	std::string arg(argv[1]);
	if(!demos.count(arg)) {
		printf("No such demo '%s'\n", arg.c_str());
		return 1;
	}

	demos[arg]();
}
