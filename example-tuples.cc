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
#include <cmath>

#include <boost/tuple/tuple.hpp>
#include <boost/array.hpp>

#include "gnuplot-iostream.h"

const int num_steps = 100;

double get_x(int step, double shift) {
	double theta = 2.0*M_PI*step/(num_steps-1);
	return std::cos(theta) * (1 + 0.3*std::cos(3.0*theta+2.0*M_PI*shift));
}

double get_y(int step, double shift) {
	double theta = 2.0*M_PI*step/(num_steps-1);
	return std::sin(theta) * (1 + 0.3*std::cos(3.0*theta+2.0*M_PI*shift));
}

double get_z(int step, double shift) {
	double theta = 2.0*M_PI*step/(num_steps-1);
	return 0.3*std::sin(3.0*theta+2.0*M_PI*shift);
}

int main() {
	// -persist option makes the window not disappear when your program exits
	Gnuplot gp("gnuplot -persist");
	// for debugging, prints to console
	//Gnuplot gp(stdout);

	int num_cords = 3;
	double shift = 0;

	gp << "set zrange [-1:1]\n";

	gp << "splot ";
	for(int i=0; i<num_cords; i++) {
		if(i) gp << ", ";
		gp << "'-' with lines";
	}
	gp << "\n";

	// vector of boost::tuple
	{
		std::vector<boost::tuple<double, double, double> > pts;
		for(int i=0; i<num_steps; i++) {
			pts.push_back(boost::make_tuple(get_x(i, shift), get_y(i, shift), get_z(i, shift)));
		}
		gp.send1d(pts);
	}
	shift += 1.0/num_cords;

	// boost::tuple of vectors
	{
		std::vector<double> x_pts, y_pts, z_pts;
		for(int i=0; i<num_steps; i++) {
			x_pts.push_back(get_x(i, shift));
			y_pts.push_back(get_y(i, shift));
			z_pts.push_back(get_z(i, shift));
		}
		gp.send1d(boost::make_tuple(x_pts, y_pts, z_pts));
	}
	shift += 1.0/num_cords;

	// vector of boost::array
	{
		std::vector<boost::array<double, 3> > pts(num_steps);
		for(int i=0; i<num_steps; i++) {
			pts[i][0] = get_x(i, shift);
			pts[i][1] = get_y(i, shift);
			pts[i][2] = get_z(i, shift);
		}
		gp.send1d(pts);
	}
	shift += 1.0/num_cords;

	return 0;
}
