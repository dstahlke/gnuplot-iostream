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

// FIXME - Currently this test is not being run and validated by the Makefile.
// I'm waiting to see the outcome of the following bug report:
// https://sourceforge.net/p/gnuplot/bugs/1500/

#include <fstream>
#include <vector>

#include "gnuplot-iostream.h"

using namespace gnuplotio;

const std::string basedir = "unittest-output";

template <typename T>
void go(Gnuplot &gp, const T &data) {
    gp << "plot '-' binary" << gp.binFmt1d(data, "record") << std::endl;
    gp.sendBinary1d(data);
    gp << "plot '-' binary" << gp.binFmt2d(data, "record") << std::endl;
    gp.sendBinary2d(data);
    gp << "plot '-'\n" << std::endl;
    gp.send1d(data);
    gp << "plot '-'\n" << std::endl;
    gp.send2d(data);
}

int main() {
    Gnuplot gp(std::fopen((basedir + "/test-empty.gnu").c_str(), "w"));

    std::vector<std::vector<std::vector<std::pair<double, int> > > > data;
    go(gp, data);

    data.resize(1);
    go(gp, data);

    data[0].resize(1);
    go(gp, data);

    data[0][0].push_back(std::make_pair(0.0, 0));
    go(gp, data);
}
