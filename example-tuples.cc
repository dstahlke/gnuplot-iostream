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

// FIXME - disable by default
#define USE_ARMA 1
#define USE_BLITZ 1
#define USE_CXX (__cplusplus >= 201103)

#include <vector>
#include <complex>
#include <cmath>

#include <boost/tuple/tuple.hpp>
#include <boost/array.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>
#include <boost/bind.hpp>
// FIXME - needed?  gcc or vc?
//#define BOOST_RESULT_OF_USE_DECLTYPE

#if USE_ARMA
#include <armadillo>
#endif

#if USE_BLITZ
#include <blitz/array.h>
#endif

#include "gnuplot-iostream.h"

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

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

// This doesn't have to be a template.  It's just a template to show that such things are
// possible.
template <typename T>
struct MyTriple {
	MyTriple(T _x, T _y, T _z) : x(_x), y(_y), z(_z) { }

	T x, y, z;
};

// Tells gnuplot-iostream how to print objects of class MyTriple.
namespace gnuplotio {
	template<typename T>
	struct BinfmtSender<MyTriple<T> > {
		static void send(std::ostream &stream) {
			BinfmtSender<T>::send(stream);
			BinfmtSender<T>::send(stream);
			BinfmtSender<T>::send(stream);
		}
	};

	// FIXME - should implement BinarySender, with note that default works in some cases.

	// We don't use text mode in this demo.  This is just here to show how it would go.
	template<typename T>
	struct TextSender<MyTriple<T> > {
		static void send(std::ostream &stream, const MyTriple<T> &v) {
			TextSender<T>::send(stream, v.x);
			stream << " ";
			TextSender<T>::send(stream, v.y);
			stream << " ";
			TextSender<T>::send(stream, v.z);
		}
	};
}

int main() {
	// -persist option makes the window not disappear when your program exits
	Gnuplot gp("gnuplot -persist");
	// for debugging, prints to console
	//Gnuplot gp(stdout);

	int num_examples = 10 + 4*USE_ARMA + 3*USE_BLITZ + 2*USE_CXX;
	double shift = 0;

	gp << "set zrange [-1:1]\n";

	gp << "splot ";

	{
		std::vector<std::pair<std::pair<double, double>, double> > pts;
		for(int i=0; i<num_steps; i++) {
			pts.push_back(std::make_pair(std::make_pair(get_x(i, shift), get_y(i, shift)), get_z(i, shift)));
		}
		gp << gp.binRec1d(pts) << "with lines title 'vector of nested std::pair'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		// complex is treated as if it were a pair
		std::vector<std::pair<std::complex<double>, double> > pts;
		for(int i=0; i<num_steps; i++) {
			pts.push_back(std::make_pair(std::complex<double>(get_x(i, shift), get_y(i, shift)), get_z(i, shift)));
		}
		gp << gp.binRec1d(pts) << "with lines title 'vector of pair of cplx and double'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		std::vector<boost::tuple<double, double, double> > pts;
		for(int i=0; i<num_steps; i++) {
			pts.push_back(boost::make_tuple(get_x(i, shift), get_y(i, shift), get_z(i, shift)));
		}
		gp << gp.binRec1d(pts) << "with lines title 'vector of boost::tuple'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		std::vector<double> x_pts, y_pts, z_pts;
		for(int i=0; i<num_steps; i++) {
			x_pts.push_back(get_x(i, shift));
			y_pts.push_back(get_y(i, shift));
			z_pts.push_back(get_z(i, shift));
		}
		gp << gp.binRec1d(boost::make_tuple(x_pts, y_pts, z_pts)) << "with lines title 'boost::tuple of vector'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		std::vector<boost::array<double, 3> > pts(num_steps);
		for(int i=0; i<num_steps; i++) {
			pts[i][0] = get_x(i, shift);
			pts[i][1] = get_y(i, shift);
			pts[i][2] = get_z(i, shift);
		}
		gp << gp.binRec1d(pts) << "with lines title 'vector of boost::array'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		std::vector<std::vector<double> > pts(num_steps);
		for(int i=0; i<num_steps; i++) {
			pts[i].push_back(get_x(i, shift));
			pts[i].push_back(get_y(i, shift));
			pts[i].push_back(get_z(i, shift));
		}
		gp << gp.binRec1d(pts) << "with lines title 'vector of vector'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		std::vector<std::vector<double> > pts(3);
		for(int i=0; i<num_steps; i++) {
			pts[0].push_back(get_x(i, shift));
			pts[1].push_back(get_y(i, shift));
			pts[2].push_back(get_z(i, shift));
		}
		gp << gp.binRec1d_colmajor(pts) << "with lines title 'vector of vector (colmajor)'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		std::vector<MyTriple<double> > pts;
		for(int i=0; i<num_steps; i++) {
			pts.push_back(MyTriple<double>(get_x(i, shift), get_y(i, shift), get_z(i, shift)));
		}
		gp << gp.binRec1d(pts) << "with lines title 'vector of MyTriple'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		// FIXME - note warning against using C arrays
		double pts[num_steps][3];
		for(int i=0; i<num_steps; i++) {
			pts[i][0] = get_x(i, shift);
			pts[i][1] = get_y(i, shift);
			pts[i][2] = get_z(i, shift);
		}
		gp << gp.binRec1d(pts) << "with lines title 'double[N][3]'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		// FIXME - note warning against using C arrays
		double pts[3][num_steps];
		for(int i=0; i<num_steps; i++) {
			pts[0][i] = get_x(i, shift);
			pts[1][i] = get_y(i, shift);
			pts[2][i] = get_z(i, shift);
		}
		gp << gp.binRec1d_colmajor(pts) << "with lines title 'double[N][3] (colmajor)'";
	}

#if USE_ARMA
	gp << ", ";
	shift += 1.0/num_examples;

	{
		arma::mat pts(num_steps, 3);
		for(int i=0; i<num_steps; i++) {
			pts(i, 0) = get_x(i, shift);
			pts(i, 1) = get_y(i, shift);
			pts(i, 2) = get_z(i, shift);
		}
		gp << gp.binRec1d(pts) << "with lines title 'armadillo N*3'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		arma::mat pts(3, num_steps);
		for(int i=0; i<num_steps; i++) {
			pts(0, i) = get_x(i, shift);
			pts(1, i) = get_y(i, shift);
			pts(2, i) = get_z(i, shift);
		}
		gp << gp.binRec1d_colmajor(pts) << "with lines title 'armadillo 3*N (colmajor)'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		arma::Row<double> x_pts(num_steps);
		arma::Col<double> y_pts(num_steps);
		arma::Col<double> z_pts(num_steps);
		for(int i=0; i<num_steps; i++) {
			x_pts(i) = get_x(i, shift);
			y_pts(i) = get_y(i, shift);
			z_pts(i) = get_z(i, shift);
		}
		gp << gp.binRec1d(boost::make_tuple(x_pts, y_pts, z_pts))
			<< "with lines title 'boost tuple of arma Row,Col,Col'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		arma::field<boost::tuple<double,double,double> > pts(num_steps);
		for(int i=0; i<num_steps; i++) {
			pts(i) = boost::make_tuple(
				get_x(i, shift),
				get_y(i, shift),
				get_z(i, shift)
			);
		}
		gp << gp.binRec1d(pts) << "with lines title 'armadillo field of boost tuple'";
	}
#endif

#if USE_BLITZ
	gp << ", ";
	shift += 1.0/num_examples;

	{
		blitz::Array<blitz::TinyVector<double, 3>, 1> pts(num_steps);
		for(int i=0; i<num_steps; i++) {
			pts(i)[0] = get_x(i, shift);
			pts(i)[1] = get_y(i, shift);
			pts(i)[2] = get_z(i, shift);
		}
		gp << gp.binRec1d(pts) << "with lines title 'blitz::Array<blitz::TinyVector<double, 3>, 1>'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		blitz::Array<double, 2> pts(num_steps, 3);
		for(int i=0; i<num_steps; i++) {
			pts(i, 0) = get_x(i, shift);
			pts(i, 1) = get_y(i, shift);
			pts(i, 2) = get_z(i, shift);
		}
		gp << gp.binRec1d(pts) << "with lines title 'blitz<double>(N*3)'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		blitz::Array<double, 2> pts(3, num_steps);
		for(int i=0; i<num_steps; i++) {
			pts(0, i) = get_x(i, shift);
			pts(1, i) = get_y(i, shift);
			pts(2, i) = get_z(i, shift);
		}
		gp << gp.binRec1d_colmajor(pts) << "with lines title 'blitz<double>(3*N) (colmajor)'";
	}
#endif

#if USE_CXX
	gp << ", ";
	shift += 1.0/num_examples;

	{
		std::function<boost::tuple<double,double,double>(int)> f = [&shift](int i) {
			return boost::make_tuple(get_x(i, shift), get_y(i, shift), get_z(i, shift)); };

		auto pts = boost::irange(0, num_steps) | boost::adaptors::transformed(f);

		gp << gp.binRec1d(pts) << "with lines title 'boost transform to tuple'";
	}

	gp << ", ";
	shift += 1.0/num_examples;

	{
		auto steps = boost::irange(0, num_steps);

		gp << gp.binRec1d(boost::make_tuple(
				steps | boost::adaptors::transformed(boost::bind(get_x, _1, shift)),
				steps | boost::adaptors::transformed(boost::bind(get_y, _1, shift)),
				steps | boost::adaptors::transformed(boost::bind(get_z, _1, shift))
			)) << "with lines title 'tuple of boost transform'";
	}
#endif

	gp << std::endl;

	shift += 1.0/num_examples;
	//std::cout << shift << std::endl;
	assert(std::fabs(shift - 1.0) < 1e-12);

	return 0;
}
