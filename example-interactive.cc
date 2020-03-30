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
#include <cmath>
#include <cstdlib>

#include <boost/tuple/tuple.hpp>

// This must be defined before the first time that "gnuplot-iostream.h" is included.
#define GNUPLOT_ENABLE_PTY
#include "gnuplot-iostream.h"

int main() {
    Gnuplot gp;

    // Create field of arrows at random locations.
    std::vector<std::tuple<double,double,double,double>> arrows;
    for(size_t i=0; i<100; i++) {
        double x = rand() / static_cast<double>(RAND_MAX);
        double y = rand() / static_cast<double>(RAND_MAX);
        arrows.emplace_back(x, y, 0, 0);
    }

    double mx=0.5, my=0.5;
    int mb=1;
    while(mb != 3 && mb >= 0) {
        // Make the arrows point towards the mouse click.
        for(auto &[x, y, dx, dy] : arrows) {
            dx = (mx-x) * 0.1;
            dy = (my-y) * 0.1;
        }

        gp << "plot '-' with vectors notitle\n";
        gp.send1d(arrows);

        gp.getMouse(mx, my, mb, "Left click to aim arrows, right click to exit.");
        printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
        if(mb < 0) printf("The gnuplot window was closed.\n");
    }
}
