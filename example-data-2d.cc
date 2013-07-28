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

// This demonstrates all sorts of data types that can be plotted using send2d().  It is not
// meant as a first tutorial; for that see example-misc.cc or the project wiki.

#define USE_CXX (__cplusplus >= 201103)

#include <vector>
#include <complex>
#include <cmath>

#include <boost/tuple/tuple.hpp>
#include <boost/array.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>
#include <boost/bind.hpp>

#ifdef USE_ARMA
#include <armadillo>
#endif

#ifdef USE_BLITZ
#include <blitz/array.h>
#endif

#include "gnuplot-iostream.h"

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

// The number of axial points of the torus.
const int num_u = 10;
// The total number of longitudinal points of the torus.  This is set at the beginning of
// main().
int num_v_total;

// This doesn't have to be a template.  It's just a template to show that such things are
// possible.
template <typename T>
struct MyTriple {
	MyTriple() : x(0), y(0), z(0) { }
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

	template <typename T>
	struct BinarySender<MyTriple<T> > {
		static void send(std::ostream &stream, const MyTriple<T> &v) {
			BinarySender<T>::send(stream, v.x);
			BinarySender<T>::send(stream, v.y);
			BinarySender<T>::send(stream, v.z);
		}
	};

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

MyTriple<double> get_point(int u, int v) {
	double a = 2.0*M_PI*u/(num_u-1);
	double b = 2.0*M_PI*v/(num_v_total-1);
	double z = 0.3*std::cos(a);
	double r = 1 + 0.3*std::sin(a);
	double x = r * std::cos(b);
	double y = r * std::sin(b);
	return MyTriple<double>(x, y, z);
}

int main() {
	Gnuplot gp;
	// for debugging, prints to console
	//Gnuplot gp(stdout);

	int num_examples = 7;
#ifdef USE_ARMA
	num_examples += 3;
#endif
#ifdef USE_BLITZ
	num_examples += 3;
#endif

	int num_v_each = 50 / num_examples + 1;

	num_v_total = (num_v_each-1) * num_examples + 1;
	int shift = 0;

	gp << "set zrange [-1:1]\n";
	gp << "set hidden3d nooffset\n";

	// I use temporary files rather than stdin because the syntax ends up being easier when
	// plotting several datasets.  With the stdin method you have to give the full plot
	// command, then all the data.  But I would rather give the portion of the plot command for
	// the first dataset, then give the data, then the command for the second dataset, then the
	// data, etc.

	gp << "splot ";

	{
		std::vector<std::vector<MyTriple<double> > > pts(num_u);
		for(int u=0; u<num_u; u++) {
			pts[u].resize(num_v_each);
			for(int v=0; v<num_v_each; v++) {
				pts[u][v] = get_point(u, v+shift);
			}
		}
		gp << gp.binFile2d(pts, "record") << "with lines title 'vec of vec of MyTriple'";
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
		gp << gp.binFile2d(pts, "record") << "with lines title 'vec of vec of boost::tuple'";
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
		gp << gp.binFile2d(boost::make_tuple(x_pts, y_pts, z_pts), "record") <<
			"with lines title 'boost::tuple of vec of vec'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		std::vector<boost::tuple<
				std::vector<double>,
				std::vector<double>,
				std::vector<double>
			> > pts;
		for(int u=0; u<num_u; u++) {
			std::vector<double> x_pts(num_v_each);
			std::vector<double> y_pts(num_v_each);
			std::vector<double> z_pts(num_v_each);
			for(int v=0; v<num_v_each; v++) {
				x_pts[v] = get_point(u, v+shift).x;
				y_pts[v] = get_point(u, v+shift).y;
				z_pts[v] = get_point(u, v+shift).z;
			}
			pts.push_back(boost::make_tuple(x_pts, y_pts, z_pts));
		}
		gp << gp.binFile2d(pts, "record") <<
			"with lines title 'vec of boost::tuple of vec'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		std::vector<std::vector<double> > x_pts(num_u);
		std::vector<std::vector<std::pair<double, double> > > yz_pts(num_u);
		for(int u=0; u<num_u; u++) {
			x_pts[u].resize(num_v_each);
			yz_pts[u].resize(num_v_each);
			for(int v=0; v<num_v_each; v++) {
				x_pts [u][v] = get_point(u, v+shift).x;
				yz_pts[u][v] = std::make_pair(
					get_point(u, v+shift).y,
					get_point(u, v+shift).z);
			}
		}
		gp << gp.binFile2d(std::make_pair(x_pts, yz_pts), "record") <<
			"with lines title 'pair(vec(vec(dbl)),vec(vec(pair(dbl,dbl))))'";
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
		gp << gp.binFile2d(pts, "record") << "with lines title 'vec vec vec'";
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
		gp << gp.binFile2d_colmajor(pts, "record") << "with lines title 'vec vec vec (colmajor)'";
	}

#ifdef USE_ARMA
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
		gp << gp.file2d(pts) << "with lines title 'arma::cube(U*V*3)'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		arma::cube pts(3, num_u, num_v_each);
		for(int u=0; u<num_u; u++) {
			for(int v=0; v<num_v_each; v++) {
				pts(0, u, v) = get_point(u, v+shift).x;
				pts(1, u, v) = get_point(u, v+shift).y;
				pts(2, u, v) = get_point(u, v+shift).z;
			}
		}
		gp << gp.binFile2d_colmajor(pts, "record") << "with lines title 'arma::cube(3*U*V) (colmajor)'";
	}

	gp << ", ";
	shift += num_v_each-1;

	{
		arma::field<MyTriple<double> > pts(num_u, num_v_each);
		for(int u=0; u<num_u; u++) {
			for(int v=0; v<num_v_each; v++) {
				pts(u, v) = get_point(u, v+shift);
			}
		}
		gp << gp.binFile2d(pts, "record") << "with lines title 'arma::field'";
	}
#endif

#ifdef USE_BLITZ
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
		gp << gp.binFile2d(pts, "record") << "with lines title 'blitz::Array<blitz::TinyVector<double, 3>, 2>'";
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
		gp << gp.binFile2d(pts, "record") << "with lines title 'blitz<double>(U*V*3)'";
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
		gp << gp.binFile2d_colmajor(pts, "record") << "with lines title 'blitz<double>(3*U*V) (colmajor)'";
	}
#endif

	gp << std::endl;

	std::cout << shift+num_v_each << "," << num_v_total << std::endl;
	assert(shift+num_v_each == num_v_total);

#ifdef _WIN32
	// For Windows, prompt for a keystroke before the Gnuplot object goes out of scope so that
	// the gnuplot window doesn't get closed.
	std::cout << "Press enter to exit." << std::endl;
	std::cin.get();
#endif

	return 0;
}
