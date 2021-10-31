/*
Copyright (c) 2020 Daniel Stahlke

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

// This demonstrates all sorts of data types that can be plotted using send1d().  It is not
// meant as a first tutorial; for that see example-misc.cc or the project wiki.

#include <vector>
#include <complex>
#include <cmath>
#include <array>
#include <valarray>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>
#include <boost/bind.hpp>

#ifdef USE_ARMA
#include <armadillo>
#endif

#ifdef USE_EIGEN
#include <Eigen/Dense>
#endif

#ifdef USE_BLITZ
#include <blitz/array.h>
#endif

#include "gnuplot-iostream.h"

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

static constexpr int num_steps = 100;

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
    MyTriple() : x(0), y(0), z(0) { }
    MyTriple(T _x, T _y, T _z) : x(_x), y(_y), z(_z) { }

    T x, y, z;
};

// Tells gnuplot-iostream how to print objects of class MyTriple.
namespace gnuplotio {
    template<typename T>
    struct BinfmtSender<MyTriple<T>> {
        static void send(std::ostream &stream) {
            BinfmtSender<T>::send(stream);
            BinfmtSender<T>::send(stream);
            BinfmtSender<T>::send(stream);
        }
    };

    template <typename T>
    struct BinarySender<MyTriple<T>> {
        static void send(std::ostream &stream, const MyTriple<T> &v) {
            BinarySender<T>::send(stream, v.x);
            BinarySender<T>::send(stream, v.y);
            BinarySender<T>::send(stream, v.z);
        }
    };

    // We don't use text mode in this demo.  This is just here to show how it would go.
    template<typename T>
    struct TextSender<MyTriple<T>> {
        static void send(std::ostream &stream, const MyTriple<T> &v) {
            TextSender<T>::send(stream, v.x);
            stream << " ";
            TextSender<T>::send(stream, v.y);
            stream << " ";
            TextSender<T>::send(stream, v.z);
        }
    };
} // namespace gnuplotio

int main() {
    Gnuplot gp;
    // for debugging, prints to console
    //Gnuplot gp(stdout);

    // To send data as text rather than binary (slower but more compatible):
    // FIXME function doesn't exist
    //gp.transportBinary(false);
    // To use temporary files rather then sending data through gnuplot's stdin:
    //gp.useTmpFile(true);

    int num_examples = 15;
#ifdef USE_ARMA
    num_examples += 4;
#endif
#ifdef USE_EIGEN
    num_examples += 4;
#endif
#ifdef USE_BLITZ
    num_examples += 3;
#endif

    double shift = 0;

    gp << "set zrange [-1:1]\n";

    auto plots = gp.splotGroup();

    {
        std::vector<std::pair<std::pair<double, double>, double>> pts;
        for(int i=0; i<num_steps; i++) {
            pts.emplace_back(
                    std::pair(get_x(i, shift), get_y(i, shift)),
                    get_z(i, shift));
        }
        plots.add_plot1d(pts, "with lines title 'vector of nested std::pair'");
    }

    shift += 1.0/num_examples;

    {
        // complex is treated as if it were a pair
        std::vector<std::pair<std::complex<double>, double>> pts;
        for(int i=0; i<num_steps; i++) {
            pts.emplace_back(
                    std::complex<double>(get_x(i, shift), get_y(i, shift)),
                    get_z(i, shift));
        }
        plots.add_plot1d(pts, "with lines title 'vector of pair of cplx and double'");
    }

    shift += 1.0/num_examples;

    {
        std::vector<std::tuple<double, double, double>> pts;
        for(int i=0; i<num_steps; i++) {
            pts.emplace_back(
                    get_x(i, shift), get_y(i, shift), get_z(i, shift));
        }
        plots.add_plot1d(pts, "with lines title 'vector of std::tuple'");
    }

    shift += 1.0/num_examples;

    {
        std::vector<double> x_pts, y_pts, z_pts;
        for(int i=0; i<num_steps; i++) {
            x_pts.push_back(get_x(i, shift));
            y_pts.push_back(get_y(i, shift));
            z_pts.push_back(get_z(i, shift));
        }
        plots.add_plot1d(std::tuple(x_pts, y_pts, z_pts),
                "with lines title 'std::tuple of vector'");
    }

    shift += 1.0/num_examples;

    {
        std::vector<std::array<double, 3>> pts(num_steps);
        for(int i=0; i<num_steps; i++) {
            pts[i][0] = get_x(i, shift);
            pts[i][1] = get_y(i, shift);
            pts[i][2] = get_z(i, shift);
        }
        plots.add_plot1d(pts, "with lines title 'vector of std::array'");
    }

    shift += 1.0/num_examples;

    {
        std::vector<std::vector<double>> pts(num_steps);
        for(int i=0; i<num_steps; i++) {
            pts[i].push_back(get_x(i, shift));
            pts[i].push_back(get_y(i, shift));
            pts[i].push_back(get_z(i, shift));
        }
        plots.add_plot1d(pts, "with lines title 'vector of vector'");
    }

    shift += 1.0/num_examples;

    {
        std::vector<std::vector<double>> pts(3);
        for(int i=0; i<num_steps; i++) {
            pts[0].push_back(get_x(i, shift));
            pts[1].push_back(get_y(i, shift));
            pts[2].push_back(get_z(i, shift));
        }
        plots.add_plot1d_colmajor(pts, "with lines title 'vector of vector (colmajor)'");
    }

    shift += 1.0/num_examples;

    {
        std::vector<MyTriple<double>> pts;
        for(int i=0; i<num_steps; i++) {
            pts.emplace_back(get_x(i, shift), get_y(i, shift), get_z(i, shift));
        }
        plots.add_plot1d(pts, "with lines title 'vector of MyTriple'");
    }

    shift += 1.0/num_examples;

    {
        // Note: C style arrays seem to work, but are a bit fragile since they easily decay to
        // pointers, causing them to forget their lengths.  It is highly recommended that you
        // use std::array instead.  These have the same size and efficiency of C style
        // arrays, but act like STL containers.
        double pts[num_steps][3];
        for(int i=0; i<num_steps; i++) {
            pts[i][0] = get_x(i, shift);
            pts[i][1] = get_y(i, shift);
            pts[i][2] = get_z(i, shift);
        }
        plots.add_plot1d(pts, "with lines title 'double[N][3]'");
    }

    shift += 1.0/num_examples;

    {
        // Note: C style arrays seem to work, but are a bit fragile since they easily decay to
        // pointers, causing them to forget their lengths.  It is highly recommended that you
        // use std::array instead.  These have the same size and efficiency of C style
        // arrays, but act like STL containers.
        double pts[3][num_steps];
        for(int i=0; i<num_steps; i++) {
            pts[0][i] = get_x(i, shift);
            pts[1][i] = get_y(i, shift);
            pts[2][i] = get_z(i, shift);
        }
        plots.add_plot1d_colmajor(pts, "with lines title 'double[N][3] (colmajor)'");
    }

    shift += 1.0/num_examples;

    {
        // Note: C style arrays seem to work, but are a bit fragile since they easily decay to
        // pointers, causing them to forget their lengths.  It is highly recommended that you
        // use std::array instead.  These have the same size and efficiency of C style
        // arrays, but act like STL containers.
        double x_pts[num_steps];
        double y_pts[num_steps];
        double z_pts[num_steps];
        for(int i=0; i<num_steps; i++) {
            x_pts[i] = get_x(i, shift);
            y_pts[i] = get_y(i, shift);
            z_pts[i] = get_z(i, shift);
        }
        plots.add_plot1d(std::forward_as_tuple(x_pts, y_pts, z_pts),
                "with lines title 'std::tuple of double[N]'");
    }

    shift += 1.0/num_examples;

    {
        std::valarray<std::tuple<double, double, double>> pts(num_steps);
        for(int i=0; i<num_steps; i++) {
            pts[i] = std::tuple(
                    get_x(i, shift), get_y(i, shift), get_z(i, shift));
        }
        plots.add_plot1d(pts, "with lines title 'valarray of std::tuple'");
    }

#ifdef USE_ARMA
    shift += 1.0/num_examples;

    {
        arma::mat pts(num_steps, 3);
        for(int i=0; i<num_steps; i++) {
            pts(i, 0) = get_x(i, shift);
            pts(i, 1) = get_y(i, shift);
            pts(i, 2) = get_z(i, shift);
        }
        plots.add_plot1d(pts, "with lines title 'armadillo N*3'");
    }

    shift += 1.0/num_examples;

    {
        arma::mat pts(3, num_steps);
        for(int i=0; i<num_steps; i++) {
            pts(0, i) = get_x(i, shift);
            pts(1, i) = get_y(i, shift);
            pts(2, i) = get_z(i, shift);
        }
        plots.add_plot1d_colmajor(pts, "with lines title 'armadillo 3*N (colmajor)'");
    }

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
        plots.add_plot1d(std::tuple(x_pts, y_pts, z_pts),
                "with lines title 'tuple of arma Row,Col,Col'");
    }

    shift += 1.0/num_examples;

    {
        arma::field<std::tuple<double,double,double>> pts(num_steps);
        for(int i=0; i<num_steps; i++) {
            pts(i) = std::tuple(
                get_x(i, shift),
                get_y(i, shift),
                get_z(i, shift)
            );
        }
        plots.add_plot1d(pts, "with lines title 'armadillo field of tuple'");
    }
#endif // USE_ARMA

#ifdef USE_EIGEN
    shift += 1.0/num_examples;

    {
        Eigen::MatrixXf pts(num_steps, 3);
        for(int i=0; i<num_steps; i++) {
            pts(i, 0) = get_x(i, shift);
            pts(i, 1) = get_y(i, shift);
            pts(i, 2) = get_z(i, shift);
        }
        plots.add_plot1d(pts, "with lines title 'eigen N*3'");
    }

    shift += 1.0/num_examples;

    {
        Eigen::MatrixXf pts(3, num_steps);
        for(int i=0; i<num_steps; i++) {
            pts(0, i) = get_x(i, shift);
            pts(1, i) = get_y(i, shift);
            pts(2, i) = get_z(i, shift);
        }
        plots.add_plot1d_colmajor(pts, "with lines title 'eigen 3*N (colmajor)'");
    }

    shift += 1.0/num_examples;

    {
        Eigen::VectorXf x_pts(num_steps);
        Eigen::VectorXf y_pts(num_steps);
        Eigen::VectorXf z_pts(num_steps);
        for(int i=0; i<num_steps; i++) {
            x_pts(i) = get_x(i, shift);
            y_pts(i) = get_y(i, shift);
            z_pts(i) = get_z(i, shift);
        }
        plots.add_plot1d(std::tuple(x_pts, y_pts, z_pts),
                "with lines title 'tuple of eigen Col,Col,Col'");
    }

    shift += 1.0/num_examples;

    {
        Eigen::RowVectorXf x_pts(num_steps);
        Eigen::RowVectorXf y_pts(num_steps);
        Eigen::RowVectorXf z_pts(num_steps);
        for(int i=0; i<num_steps; i++) {
            x_pts(i) = get_x(i, shift);
            y_pts(i) = get_y(i, shift);
            z_pts(i) = get_z(i, shift);
        }
        plots.add_plot1d(std::tuple(x_pts, y_pts, z_pts),
                "with lines title 'tuple of eigen Row,Row,Row'");
    }
#endif // USE_EIGEN

#ifdef USE_BLITZ
    shift += 1.0/num_examples;

    {
        blitz::Array<blitz::TinyVector<double, 3>, 1> pts(num_steps);
        for(int i=0; i<num_steps; i++) {
            pts(i)[0] = get_x(i, shift);
            pts(i)[1] = get_y(i, shift);
            pts(i)[2] = get_z(i, shift);
        }
        plots.add_plot1d(pts, "with lines title 'blitz::Array<blitz::TinyVector<double, 3>, 1>'");
    }

    shift += 1.0/num_examples;

    {
        blitz::Array<double, 2> pts(num_steps, 3);
        for(int i=0; i<num_steps; i++) {
            pts(i, 0) = get_x(i, shift);
            pts(i, 1) = get_y(i, shift);
            pts(i, 2) = get_z(i, shift);
        }
        plots.add_plot1d(pts, "with lines title 'blitz<double>(N*3)'");
    }

    shift += 1.0/num_examples;

    {
        blitz::Array<double, 2> pts(3, num_steps);
        for(int i=0; i<num_steps; i++) {
            pts(0, i) = get_x(i, shift);
            pts(1, i) = get_y(i, shift);
            pts(2, i) = get_z(i, shift);
        }
        plots.add_plot1d_colmajor(pts, "with lines title 'blitz<double>(3*N) (colmajor)'");
    }
#endif // USE_BLITZ

    shift += 1.0/num_examples;

    {
        std::function<std::tuple<double,double,double>(int)> f = [&shift](int i) {
            return std::tuple(get_x(i, shift), get_y(i, shift), get_z(i, shift)); };

        auto pts = boost::irange(0, num_steps) | boost::adaptors::transformed(f);

        plots.add_plot1d(pts, "with lines title 'boost transform to tuple'");
    }

    shift += 1.0/num_examples;

    {
        auto steps = boost::irange(0, num_steps);

        plots.add_plot1d(std::tuple(
                steps | boost::adaptors::transformed(boost::bind(get_x, _1, shift)),
                steps | boost::adaptors::transformed(boost::bind(get_y, _1, shift)),
                steps | boost::adaptors::transformed(boost::bind(get_z, _1, shift))
            ), "with lines title 'tuple of boost transform'");
    }

    shift += 1.0/num_examples;

    {
        // Note: C style arrays seem to work, but are a bit fragile since they easily decay to
        // pointers, causing them to forget their lengths.  It is highly recommended that you
        // use std::array instead.  These have the same size and efficiency of C style
        // arrays, but act like STL containers.
        double x_pts[num_steps];
        double y_pts[num_steps];
        double z_pts[num_steps];
        for(int i=0; i<num_steps; i++) {
            x_pts[i] = get_x(i, shift);
            y_pts[i] = get_y(i, shift);
            z_pts[i] = get_z(i, shift);
        }
        // Note: std::tuple doesn't work here since it makes the arrays decay to pointers,
        // and as a result they forget their lengths.
        plots.add_plot1d(std::tie(x_pts, y_pts, z_pts),
                "with lines title 'std::tie of double[N]'");
    }

    gp << plots;

    shift += 1.0/num_examples;
    //std::cout << shift << std::endl;
    assert(std::fabs(shift - 1.0) < 1e-12);

#ifdef _WIN32
    // For Windows, prompt for a keystroke before the Gnuplot object goes out of scope so that
    // the gnuplot window doesn't get closed.
    std::cout << "Press enter to exit." << std::endl;
    std::cin.get();
#endif

    return 0;
}
