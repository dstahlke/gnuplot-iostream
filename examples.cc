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

#include <vector>
#include <math.h>

#include "gnuplot-iostream.h"

// Yes, I'm including a *.cc file.  It contains main().
#include "examples-framework.cc"

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
	Gnuplot gp("gnuplot -persist");

	std::vector<std::vector<float> > vecs(4);
	for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
		double theta = alpha*2.0*3.14159;
		vecs[0].push_back( cos(theta));
		vecs[1].push_back( sin(theta));
		vecs[2].push_back(-cos(theta)*0.1);
		vecs[3].push_back(-sin(theta)*0.1);
	}

	gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
	gp << "plot '-' with vectors title 'circle'\n";
	gp.send(vecs);
}

// FIXME - do without blitz
//void demo_waves_binary_file() {
//	Gnuplot gp("gnuplot -persist");
//
//	// example from Blitz manual:
//	int N = 64, cycles = 3;
//	double midpoint = (N-1)/2.;
//	double omega = 2.0 * M_PI * cycles / double(N);
//	double tau = - 10.0 / N;
//	blitz::Array<double, 2> F(N, N);
//	blitz::firstIndex i;
//	blitz::secondIndex j;
//	F = cos(omega * sqrt(pow2(i-midpoint) + pow2(j-midpoint)))
//		* exp(tau * sqrt(pow2(i-midpoint) + pow2(j-midpoint)));
//
//	gp << "splot" << gp.binary_file(F) << "dx=10 dy=10 origin=(5,5,0) with pm3d notitle" << std::endl;
//}

void register_demos() {
	register_demo("basic",                  demo_basic);
	register_demo("png",                    demo_png);
	register_demo("vectors",                demo_vectors);
	//register_demo("waves_binary_file",      demo_waves_binary_file);
}
