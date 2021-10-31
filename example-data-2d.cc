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

// This demonstrates all sorts of data types that can be plotted using send2d().  It is not
// meant as a first tutorial; for that see example-misc.cc or the project wiki.

#include <vector>
#include <complex>
#include <cmath>
#include <valarray>

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

    // To send data as text rather than binary (slower but more compatible):
    //gp.transportBinary(false);
    // To use temporary files rather then sending data through gnuplot's stdin:
    //gp.useTmpFile(true);

    int num_examples = 8;
#ifdef USE_ARMA
    num_examples += 3;
#endif
#ifdef USE_EIGEN
    num_examples += 1;
#endif
#ifdef USE_BLITZ
    num_examples += 3;
#endif

    int num_v_each = 50 / num_examples + 1;

    num_v_total = (num_v_each-1) * num_examples + 1;
    int shift = 0;

    gp << "set zrange [-1:1]\n";
    gp << "set hidden3d nooffset\n";

    auto plots = gp.splotGroup();

    {
        std::vector<std::vector<MyTriple<double>>> pts(num_u);
        for(int u=0; u<num_u; u++) {
            pts[u].resize(num_v_each);
            for(int v=0; v<num_v_each; v++) {
                pts[u][v] = get_point(u, v+shift);
            }
        }
        plots.add_plot2d(pts, "with lines title 'vec of vec of MyTriple'");
    }

    shift += num_v_each-1;

    {
        std::vector<std::vector<std::tuple<double,double,double>>> pts(num_u);
        for(int u=0; u<num_u; u++) {
            pts[u].resize(num_v_each);
            for(int v=0; v<num_v_each; v++) {
                pts[u][v] = std::make_tuple(
                    get_point(u, v+shift).x,
                    get_point(u, v+shift).y,
                    get_point(u, v+shift).z
                );
            }
        }
        plots.add_plot2d(pts, "with lines title 'vec of vec of std::tuple'");
    }

    shift += num_v_each-1;

    {
        std::vector<std::vector<double>> x_pts(num_u);
        std::vector<std::vector<double>> y_pts(num_u);
        std::vector<std::vector<double>> z_pts(num_u);
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
        plots.add_plot2d(std::make_tuple(x_pts, y_pts, z_pts),
                "with lines title 'std::tuple of vec of vec'");
    }

    shift += num_v_each-1;

    {
        std::vector<std::tuple<
                std::vector<double>,
                std::vector<double>,
                std::vector<double>
            >> pts;
        for(int u=0; u<num_u; u++) {
            std::vector<double> x_pts(num_v_each);
            std::vector<double> y_pts(num_v_each);
            std::vector<double> z_pts(num_v_each);
            for(int v=0; v<num_v_each; v++) {
                x_pts[v] = get_point(u, v+shift).x;
                y_pts[v] = get_point(u, v+shift).y;
                z_pts[v] = get_point(u, v+shift).z;
            }
            pts.emplace_back(x_pts, y_pts, z_pts);
        }
        plots.add_plot2d(pts, "with lines title 'vec of std::tuple of vec'");
    }

    shift += num_v_each-1;

    {
        std::vector<std::vector<double>> x_pts(num_u);
        std::vector<std::vector<std::pair<double, double>>> yz_pts(num_u);
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
        plots.add_plot2d(std::make_pair(x_pts, yz_pts),
                "with lines title 'pair(vec(vec(dbl)),vec(vec(pair(dbl,dbl))))'");
    }

    shift += num_v_each-1;

    {
        std::vector<std::vector<std::vector<double>>> pts(num_u);
        for(int u=0; u<num_u; u++) {
            pts[u].resize(num_v_each);
            for(int v=0; v<num_v_each; v++) {
                pts[u][v].resize(3);
                pts[u][v][0] = get_point(u, v+shift).x;
                pts[u][v][1] = get_point(u, v+shift).y;
                pts[u][v][2] = get_point(u, v+shift).z;
            }
        }
        plots.add_plot2d(pts, "with lines title 'vec vec vec'");
    }

    shift += num_v_each-1;

    {
        std::vector<std::vector<std::vector<double>>> pts(3);
        for(int i=0; i<3; i++) pts[i].resize(num_u);
        for(int u=0; u<num_u; u++) {
            for(int i=0; i<3; i++) pts[i][u].resize(num_v_each);
            for(int v=0; v<num_v_each; v++) {
                pts[0][u][v] = get_point(u, v+shift).x;
                pts[1][u][v] = get_point(u, v+shift).y;
                pts[2][u][v] = get_point(u, v+shift).z;
            }
        }
        plots.add_plot2d_colmajor(pts, "with lines title 'vec vec vec (colmajor)'");
    }

    shift += num_v_each-1;

    {
        std::valarray<std::tuple<
                std::valarray<double>,
                std::valarray<double>,
                std::valarray<double>
            >> pts(num_u);
        for(int u=0; u<num_u; u++) {
            std::get<0>(pts[u]).resize(num_v_each);
            std::get<1>(pts[u]).resize(num_v_each);
            std::get<2>(pts[u]).resize(num_v_each);
            for(int v=0; v<num_v_each; v++) {
                std::get<0>(pts[u])[v] = get_point(u, v+shift).x;
                std::get<1>(pts[u])[v] = get_point(u, v+shift).y;
                std::get<2>(pts[u])[v] = get_point(u, v+shift).z;
            }
        }
        plots.add_plot2d(pts, "with lines title 'valarray of std::tuple of valarray'");
    }

#ifdef USE_ARMA
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
        plots.add_plot2d(pts, "with lines title 'arma::cube(U*V*3)'");
    }

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
        plots.add_plot2d_colmajor(pts, "with lines title 'arma::cube(3*U*V) (colmajor)'");
    }

    shift += num_v_each-1;

    {
        arma::field<MyTriple<double>> pts(num_u, num_v_each);
        for(int u=0; u<num_u; u++) {
            for(int v=0; v<num_v_each; v++) {
                pts(u, v) = get_point(u, v+shift);
            }
        }
        plots.add_plot2d(pts, "with lines title 'arma::field'");
    }
#endif // USE_ARMA

#ifdef USE_EIGEN
    shift += num_v_each-1;

    {
        Eigen::MatrixXd ptsx(num_u, num_v_each);
        Eigen::MatrixXd ptsy(num_u, num_v_each);
        Eigen::MatrixXd ptsz(num_u, num_v_each);
        for(int u=0; u<num_u; u++) {
            for(int v=0; v<num_v_each; v++) {
                ptsx(u, v) = get_point(u, v+shift).x;
                ptsy(u, v) = get_point(u, v+shift).y;
                ptsz(u, v) = get_point(u, v+shift).z;
            }
        }
        plots.add_plot2d(std::tuple{ptsx, ptsy, ptsz}, "with lines title 'tuple<eigen, eigen, eigen>'");
    }
#endif // USE_EIGEN

#ifdef USE_BLITZ
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
        plots.add_plot2d(pts, "with lines title 'blitz::Array<blitz::TinyVector<double, 3>, 2>'");
    }

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
        plots.add_plot2d(pts, "with lines title 'blitz<double>(U*V*3)'");
    }

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
        plots.add_plot2d_colmajor(pts, "with lines title 'blitz<double>(3*U*V) (colmajor)'");
    }
#endif // USE_BLITZ

    gp << plots;

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
