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

#include <vector>

#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>

#include "gnuplot-iostream.h"

#define USE_CXX (__cplusplus >= 201103)

template <typename T>
class NonCopyable : boost::noncopyable, public std::vector<T> {
public:
    NonCopyable() { }
};

int main() {
    Gnuplot gp;
    NonCopyable<double> nc_x, nc_y;
    for(int i=0; i<100; i++) {
        nc_x.push_back(-i);
        nc_y.push_back(i*i);
    }
    gp << "plot '-', '-'\n";

    // These don't work because they make copies.
    //gp.send1d(std::make_pair(nc_x, nc_y));
    //gp.send1d(boost::make_tuple(nc_x, nc_y));
    // These work because they make references:
#if USE_CXX
    std::cout << "using std::tie" << std::endl;
    gp.send1d(std::tie(nc_y));
    gp.send1d(std::forward_as_tuple(nc_x, std::move(nc_y)));
#else
    std::cout << "using boost::tie" << std::endl;
    gp.send1d(nc_y);
    gp.send1d(boost::tie(nc_x, nc_y));
#endif
}
