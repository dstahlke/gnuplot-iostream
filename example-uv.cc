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

const int num_u = 10;
int num_v_total;

struct MyTriple {
	MyTriple() { }
	MyTriple(double _x, double _y, double _z) : x(_x), y(_y), z(_z) { }

	double x, y, z;
};

// Tells gnuplot-iostream how to print objects of class MyTriple.
namespace gnuplotio {
	template <>
	struct BinfmtSender<MyTriple> {
		static void send(std::ostream &stream) {
			BinfmtSender<double>::send(stream);
			BinfmtSender<double>::send(stream);
			BinfmtSender<double>::send(stream);
		}
	};

	// FIXME - should implement BinarySender, with note that default works in some cases.

	// We don't use text mode in this demo.  This is just here to show how it would go.
	template <>
	struct TextSender<MyTriple> {
		static void send(std::ostream &stream, const MyTriple &v) {
			TextSender<double>::send(stream, v.x);
			stream << " ";
			TextSender<double>::send(stream, v.y);
			stream << " ";
			TextSender<double>::send(stream, v.z);
		}
	};
}

MyTriple get_point(int u, int v) {
	double a = 2.0*M_PI*u/(num_u-1);
	double b = 2.0*M_PI*v/(num_v_total-1);
	double z = 0.3*std::cos(a);
	double r = 1 + 0.3*std::sin(a);
	double x = r * std::cos(b);
	double y = r * std::sin(b);
	return MyTriple(x, y, z);
}

int main() {
	// -persist option makes the window not disappear when your program exits
	Gnuplot gp("gnuplot -persist");
	// for debugging, prints to console
	//Gnuplot gp(stdout);

	int num_examples = 5 + USE_BLITZ*3;
	int num_v_each = 50 / num_examples + 1;

	num_v_total = (num_v_each-1) * num_examples + 1;
	int shift = 0;

	gp << "set zrange [-1:1]\n";
	gp << "set hidden3d nooffset\n";

	gp << "splot ";

	{
		std::vector<std::vector<MyTriple> > pts(num_u);
		for(int u=0; u<num_u; u++) {
			pts[u].resize(num_v_each);
			for(int v=0; v<num_v_each; v++) {
				pts[u][v] = get_point(u, v+shift);
			}
		}
		gp << gp.binRec2d(pts) << "with lines title 'vector of MyTriple'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		std::vector<std::vector<boost::tuple<double,double,double> > > pts(num_u);
		for(int u=0; u<num_u; u++) {
			pts[u].resize(num_v_each);
			for(int v=0; v<num_v_each; v++) {
				pts[u][v] = boost::make_tuple(
					get_point(u, v+shift).x,
					get_point(u, v+shift).y,
					get_point(u, v+shift).z
				);
			}
		}
		gp << gp.binRec2d(pts) << "with lines title 'vector of MyTriple'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		std::vector<std::vector<double> > x_pts(num_u);
		std::vector<std::vector<double> > y_pts(num_u);
		std::vector<std::vector<double> > z_pts(num_u);
		for(int u=0; u<num_u; u++) {
			x_pts[u].resize(num_v_each);
			y_pts[u].resize(num_v_each);
			z_pts[u].resize(num_v_each);
			for(int v=0; v<num_v_each; v++) {
				x_pts[u][v] = get_point(u, v+shift).x;
				y_pts[u][v] = get_point(u, v+shift).y;
				z_pts[u][v] = get_point(u, v+shift).z;
			}
		}
		gp << gp.binRec2d(boost::make_tuple(x_pts, y_pts, z_pts)) << "with lines title 'boost tuple of vector'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		std::vector<std::vector<std::vector<double> > > pts(num_u);
		for(int u=0; u<num_u; u++) {
			pts[u].resize(num_v_each);
			for(int v=0; v<num_v_each; v++) {
				pts[u][v].resize(3);
				pts[u][v][0] = get_point(u, v+shift).x;
				pts[u][v][1] = get_point(u, v+shift).y;
				pts[u][v][2] = get_point(u, v+shift).z;
			}
		}
		gp << gp.binRec2d(pts) << "with lines title 'vec vec vec'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		std::vector<std::vector<std::vector<double> > > pts(3);
		for(int i=0; i<3; i++) pts[i].resize(num_u);
		for(int u=0; u<num_u; u++) {
			for(int i=0; i<3; i++) pts[i][u].resize(num_v_each);
			for(int v=0; v<num_v_each; v++) {
				pts[0][u][v] = get_point(u, v+shift).x;
				pts[1][u][v] = get_point(u, v+shift).y;
				pts[2][u][v] = get_point(u, v+shift).z;
			}
		}
		gp << gp.binRec2d_colmajor(pts) << "with lines title 'vec vec vec (colmajor)'";
	}

#if USE_ARMA
	gp << ", ";
	shift += num_v_each-1;

	{
		arma::cube pts(num_u, num_v_each, 3);
		for(int u=0; u<num_u; u++) {
			for(int v=0; v<num_v_each; v++) {
				pts(u, v, 0) = get_point(u, v+shift).x;
				pts(u, v, 1) = get_point(u, v+shift).y;
				pts(u, v, 2) = get_point(u, v+shift).z;
			}
		}
		gp << gp.binRec2d(pts) << "with lines title 'arma::cube(U*V*3)'";
	}

	// FIXME
//	gp << ", ";
//	shift += num_v_each-1;
//
//	{
//		arma::cube pts(3, num_u, num_v_each);
//		for(int u=0; u<num_u; u++) {
//			for(int v=0; v<num_v_each; v++) {
//				pts(0, u, v) = get_point(u, v+shift).x;
//				pts(1, u, v) = get_point(u, v+shift).y;
//				pts(2, u, v) = get_point(u, v+shift).z;
//			}
//		}
//		gp << gp.binRec2d_colmajor(pts) << "with lines title 'arma::cube(3*U*V) (colmajor)'";
//	}
#endif

#if USE_BLITZ
	gp << ", ";
	shift += num_v_each-1;

	{
		blitz::Array<blitz::TinyVector<double, 3>, 2> pts(num_u, num_v_each);
		for(int u=0; u<num_u; u++) {
			for(int v=0; v<num_v_each; v++) {
				pts(u, v)[0] = get_point(u, v+shift).x;
				pts(u, v)[1] = get_point(u, v+shift).y;
				pts(u, v)[2] = get_point(u, v+shift).z;
			}
		}
		gp << gp.binRec2d(pts) << "with lines title 'blitz::Array<blitz::TinyVector<double, 3>, 2>'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		blitz::Array<double, 3> pts(num_u, num_v_each, 3);
		for(int u=0; u<num_u; u++) {
			for(int v=0; v<num_v_each; v++) {
				pts(u, v, 0) = get_point(u, v+shift).x;
				pts(u, v, 1) = get_point(u, v+shift).y;
				pts(u, v, 2) = get_point(u, v+shift).z;
			}
		}
		gp << gp.binRec2d(pts) << "with lines title 'blitz<double>(U*V*3)'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		blitz::Array<double, 3> pts(3, num_u, num_v_each);
		for(int u=0; u<num_u; u++) {
			for(int v=0; v<num_v_each; v++) {
				pts(0, u, v) = get_point(u, v+shift).x;
				pts(1, u, v) = get_point(u, v+shift).y;
				pts(2, u, v) = get_point(u, v+shift).z;
			}
		}
		gp << gp.binRec2d_colmajor(pts) << "with lines title 'blitz<double>(3*U*V) (colmajor)'";
	}
#endif

	gp << std::endl;

	std::cout << shift+num_v_each << "," << num_v_total << std::endl;
	assert(shift+num_v_each == num_v_total);

	return 0;
}
