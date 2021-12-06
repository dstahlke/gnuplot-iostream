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

#include <fstream>
#include <vector>
#include <map>
#include <limits>
#include <cmath>
#include <cstdio>

// Warn about use of deprecated functions.
#define GNUPLOT_DEPRECATE_WARN
#include "gnuplot-iostream.h"

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

// http://stackoverflow.com/a/1658429
#ifdef _WIN32
    #include <windows.h>
    inline void mysleep(unsigned millis) {
        ::Sleep(millis);
    }
#else
    #include <unistd.h>
    inline void mysleep(unsigned millis) {
        ::usleep(millis * 1000);
    }
#endif

void pause_if_needed() {
#ifdef _WIN32
    // For Windows, prompt for a keystroke before the Gnuplot object goes out of scope so that
    // the gnuplot window doesn't get closed.
    std::cout << "Press enter to exit." << std::endl;
    std::cin.get();
#endif
}

// Tell MSVC to not warn about using fopen.
// http://stackoverflow.com/a/4805353/1048959
#if defined(_MSC_VER) && _MSC_VER >= 1400
#pragma warning(disable:4996)
#endif

void demo_basic() {
    Gnuplot gp;
    // For debugging or manual editing of commands:
    //Gnuplot gp(std::fopen("plot.gnu", "w"));
    // or
    //Gnuplot gp("tee plot.gnu | gnuplot -persist");

    std::vector<std::pair<double, double>> xy_pts_A;
    for(double x=-2; x<2; x+=0.01) {
        double y = x*x*x;
        xy_pts_A.emplace_back(x, y);
    }

    std::vector<std::pair<double, double>> xy_pts_B;
    for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
        double theta = alpha*2.0*3.14159;
        xy_pts_B.emplace_back(cos(theta), sin(theta));
    }

    gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
    gp << "plot '-' with lines title 'cubic', '-' with points title 'circle'\n";
    gp.send1d(xy_pts_A);
    gp.send1d(xy_pts_B);

    pause_if_needed();
}

void demo_binary() {
    Gnuplot gp;

    std::vector<std::pair<double, double>> xy_pts_A;
    for(double x=-2; x<2; x+=0.01) {
        double y = x*x*x;
        xy_pts_A.emplace_back(x, y);
    }

    std::vector<std::pair<double, double>> xy_pts_B;
    for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
        double theta = alpha*2.0*3.14159;
        xy_pts_B.emplace_back(cos(theta), sin(theta));
    }

    gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
    gp << "plot '-' binary" << gp.binFmt1d(xy_pts_A, "record") << "with lines title 'cubic',"
        << "'-' binary" << gp.binFmt1d(xy_pts_B, "record") << "with points title 'circle'\n";
    gp.sendBinary1d(xy_pts_A);
    gp.sendBinary1d(xy_pts_B);

    pause_if_needed();
}

void demo_tmpfile() {
    Gnuplot gp;

    std::vector<std::pair<double, double>> xy_pts_A;
    for(double x=-2; x<2; x+=0.01) {
        double y = x*x*x;
        xy_pts_A.emplace_back(x, y);
    }

    std::vector<std::pair<double, double>> xy_pts_B;
    for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
        double theta = alpha*2.0*3.14159;
        xy_pts_B.emplace_back(cos(theta), sin(theta));
    }

    gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
    // Data will be sent via a temporary file.  These are erased when you call
    // gp.clearTmpfiles() or when gp goes out of scope.  If you pass a filename
    // (i.e. `gp.file1d(pts, "mydata.dat")`), then the named file will be created
    // and won't be deleted.
    //
    // Note: you need std::endl here in order to flush the buffer.  The send1d()
    // function flushes automatically, but we're not using that here.
    gp << "plot" << gp.file1d(xy_pts_A) << "with lines title 'cubic',"
        << gp.file1d(xy_pts_B) << "with points title 'circle'" << std::endl;

    pause_if_needed();
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
    gp.send1d(y_pts);

    std::vector<std::pair<double, double>> xy_pts_A;
    for(double x=-2; x<2; x+=0.01) {
        double y = x*x*x;
        xy_pts_A.emplace_back(x, y);
    }

    std::vector<std::pair<double, double>> xy_pts_B;
    for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
        double theta = alpha*2.0*3.14159;
        xy_pts_B.emplace_back(cos(theta), sin(theta));
    }

    std::cout << "Creating my_graph_2.png" << std::endl;
    gp << "set output 'my_graph_2.png'\n";
    gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
    gp << "plot '-' with lines title 'cubic', '-' with points title 'circle'\n";
    gp.send1d(xy_pts_A);
    gp.send1d(xy_pts_B);
}

void demo_vectors() {
    Gnuplot gp;

    std::vector<std::tuple<double, double, double, double>> vecs;
    for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
        double theta = alpha*2.0*3.14159;
        vecs.emplace_back(
             cos(theta),      sin(theta),
            -cos(theta)*0.1, -sin(theta)*0.1
        );
    }

    gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
    gp << "plot '-' with vectors title 'circle'\n";
    gp.send1d(vecs);

    pause_if_needed();
}

std::vector<std::tuple<double, double, double>> get_trefoil() {
    std::vector<std::tuple<double, double, double>> vecs;
    for(double alpha=0; alpha<1; alpha+=1.0/120.0) {
        double theta = alpha*2.0*3.14159;
        vecs.emplace_back(
            (2+cos(3*theta))*cos(2*theta),
            (2+cos(3*theta))*sin(2*theta),
            sin(3*theta)
        );
    }
    return vecs;
}

void demo_inline_text() {
    std::cout << "Creating inline_text.gnu" << std::endl;
    // This file handle will be closed automatically when gp goes out of scope.
    Gnuplot gp(std::fopen("inline_text.gnu", "w"));

    std::vector<std::tuple<double, double, double>> vecs = get_trefoil();

    gp << "splot '-' with lines notitle\n";
    gp.send1d(vecs);

    std::cout << "Now run 'gnuplot -persist inline_text.gnu'.\n";
}

void demo_inline_binary() {
    std::cout << "Creating inline_binary.gnu" << std::endl;
    // This file handle will be closed automatically when gp goes out of scope.
    Gnuplot gp(std::fopen("inline_binary.gnu", "wb"));

    std::vector<std::tuple<double, double, double>> vecs = get_trefoil();

    gp << "splot '-' binary" << gp.binFmt1d(vecs, "record") << "with lines notitle\n";
    gp.sendBinary1d(vecs);

    std::cout << "Now run 'gnuplot -persist inline_binary.gnu'.\n";
}

void demo_external_text() {
    std::cout << "Creating external_text.gnu" << std::endl;
    // This file handle will be closed automatically when gp goes out of scope.
    Gnuplot gp(std::fopen("external_text.gnu", "w"));

    std::vector<std::tuple<double, double, double>> vecs = get_trefoil();

    std::cout << "Creating external_text.dat" << std::endl;
    gp << "splot" << gp.file1d(vecs, "external_text.dat") << "with lines notitle\n";

    std::cout << "Now run 'gnuplot -persist external_text.gnu'.\n";
}

void demo_external_binary() {
    std::cout << "Creating external_binary.gnu" << std::endl;
    // This file handle will be closed automatically when gp goes out of scope.
    Gnuplot gp(std::fopen("external_binary.gnu", "w"));

    std::vector<std::tuple<double, double, double>> vecs = get_trefoil();

    std::cout << "Creating external_binary.dat" << std::endl;
    gp << "splot" << gp.binFile1d(vecs, "record", "external_binary.dat")
        << "with lines notitle\n";

    std::cout << "Now run 'gnuplot -persist external_binary.gnu'.\n";
}

void demo_animation() {
#ifdef _WIN32
    // No animation demo for Windows.  The problem is that every time the plot
    // is updated, the gnuplot window grabs focus.  So you can't ever focus the
    // terminal window to press Ctrl-C.  The only way to quit is to right-click
    // the terminal window on the task bar and close it from there.  Other than
    // that, it seems to work.
    std::cout << "Sorry, the animation demo doesn't work in Windows." << std::endl;
    return;
#endif

    Gnuplot gp;

    std::cout << "Press Ctrl-C to quit (closing gnuplot window doesn't quit)." << std::endl;

    gp << "set yrange [-1:1]\n";

    const int N = 1000;
    std::vector<double> pts(N);

    double theta = 0;
    while(1) {
        for(int i=0; i<N; i++) {
            double alpha = (static_cast<double>(i)/N-0.5) * 10;
            pts[i] = sin(alpha*8.0 + theta) * exp(-alpha*alpha/2.0);
        }

        gp << "plot '-' binary" << gp.binFmt1d(pts, "array") << "with lines notitle\n";
        gp.sendBinary1d(pts);
        gp.flush();

        theta += 0.2;
        mysleep(100);
    }
}

void demo_NaN() {
    // Demo of NaN (not-a-number) usage.  Plot a circle that has half the coordinates replaced
    // by NaN values.

    double nan = std::numeric_limits<double>::quiet_NaN();

    Gnuplot gp;

    std::vector<std::pair<double, double>> xy_pts;
    for(int i=0; i<100; i++) {
        double theta = static_cast<double>(i)/100*2*M_PI;
        if((i/5)%2) {
            xy_pts.emplace_back(
                    std::cos(theta), std::sin(theta)
                );
        } else {
            xy_pts.emplace_back(nan, nan);
        }
    }

    // You need to tell gnuplot that 'nan' should be treated as missing data (otherwise it just
    // gives an error).
    gp << "set datafile missing 'nan'\n";
    gp << "plot '-' with linespoints\n";
    gp.send1d(xy_pts);

    // This works too.  But the strange thing is that with text data the segments are joined by
    // lines and with binary data the segments are not joined.
    //gp << "plot '-' binary" << gp.binFmt1d(xy_pts, "record") << "with linespoints\n";
    //gp.sendBinary1d(xy_pts);

    pause_if_needed();
}

void demo_segments() {
    // Demo of disconnected segments.  Plot a circle with some pieces missing.

    Gnuplot gp;

    std::vector<std::vector<std::pair<double, double>>> all_segments;
    for(int j=0; j<10; j++) {
        std::vector<std::pair<double, double>> segment;
        for(int i=0; i<5; i++) {
            double theta = static_cast<double>(j*10+i)/100*2*M_PI;
            segment.emplace_back(
                    std::cos(theta), std::sin(theta)
                );
        }
        all_segments.push_back(segment);
    }

    gp << "plot '-' with linespoints\n";
    // NOTE: send2d is used here, rather than send1d.  This puts a blank line between segments.
    gp.send2d(all_segments);

    pause_if_needed();
}

void demo_image() {
    // Example of plotting an image.  Of course you are free (and encouraged) to
    // use Blitz or Armadillo rather than std::vector in these situations.

    Gnuplot gp;

    std::vector<std::vector<double>> image;
    for(int j=0; j<100; j++) {
        std::vector<double> row;
        for(int i=0; i<100; i++) {
            double x = (i-50.0)/5.0;
            double y = (j-50.0)/5.0;
            double z = std::cos(sqrt(x*x+y*y));
            row.push_back(z);
        }
        image.push_back(row);
    }

    // It may seem counterintuitive that send1d should be used rather than
    // send2d.  The explanation is as follows.  The "send2d" method puts each
    // value on its own line, with blank lines between rows.  This is what is
    // expected by the splot command.  The two "dimensions" here are the lines
    // and the blank-line-delimited blocks.  The "send1d" method doesn't group
    // things into blocks.  So the elements of each row are printed as columns,
    // as expected by Gnuplot's "matrix with image" command.  But images
    // typically have lots of pixels, so sending as text is not the most
    // efficient (although, it's not really that bad in the case of this
    // example).  See the binary version below.
    //
    //gp << "plot '-' matrix with image\n";
    //gp.send1d(image);

    // To be honest, Gnuplot's documentation for "binary" and for "image" are
    // both unclear to me.  The following example comes by trial-and-error.
    gp << "plot '-' binary" << gp.binFmt2d(image, "array") << "with image\n";
    gp.sendBinary2d(image);

    pause_if_needed();
}

void demo_plotgroup() {
    Gnuplot gp;
    // For debugging or manual editing of commands:
    //Gnuplot gp(std::fopen("plot.gnu", "w"));
    // or
    //Gnuplot gp("tee plot.gnu | gnuplot -persist");

    // FIXME do these work?
    // To send data as text rather than binary (slower but more compatible):
    //gp.transportBinary(false);
    // To use temporary files rather then sending data through gnuplot's stdin:
    //gp.useTmpFile(true);

    std::vector<std::pair<double, double>> xy_pts_A;
    for(double x=-2; x<2; x+=0.01) {
        double y = x*x*x;
        xy_pts_A.emplace_back(x, y);
    }

    std::vector<std::pair<double, double>> xy_pts_B;
    for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
        double theta = alpha*2.0*3.14159;
        xy_pts_B.emplace_back(cos(theta), sin(theta));
    }

    gp << "set xrange [-2:2]\nset yrange [-2:2]\n";

    if(0) {
        // One way to do it:
        auto plots = gp.plotGroup();
        plots.add_plot1d(xy_pts_A, "with lines title 'cubic'");
        // For this one, save the data in a file and plot that file (rather than sending
        // directly to gnuplot's stdin).
        plots.add_plot1d(xy_pts_B, "with points title 'circle'").file("circle.dat");
        plots.add_plot("sin(x)");
        gp << plots;
    } else {
        // Alternative way:
        gp << gp.plotGroup()
            .add_plot1d(xy_pts_A, "with lines title 'cubic'")
            // For this one, save the data in a file and plot that file (rather than sending
            // directly to gnuplot's stdin).
            .add_plot1d(xy_pts_B, "with points title 'circle'").file("circle.dat")
            .add_plot("sin(x)");
    }

    pause_if_needed();
}

void demo_fit() {
    Gnuplot gp;

    std::vector<std::pair<double, double>> xy_pts;
    for(double x=-2; x<2; x+=0.5) {
        double y = 0.12 + 0.34*x + 0.56*x*x;
        xy_pts.emplace_back(x, y);
    }

    gp << "set xrange [-2:2]\n";
    gp << "f(x) = a + b*x + c*x*x\n";
    gp << "fit f(x) '-' via a,b,c\n";
    gp.send1d(xy_pts);
    gp << "plot '-' with points title 'input', f(x) with lines title 'fit'\n";
    gp.send1d(xy_pts);

    pause_if_needed();
}

int main(int argc, char **argv) {
    std::map<std::string, void (*)(void)> demos;

    demos["basic"]                  = demo_basic;
    demos["binary"]                 = demo_binary;
    demos["tmpfile"]                = demo_tmpfile;
    demos["png"]                    = demo_png;
    demos["vectors"]                = demo_vectors;
    demos["script_inline_text"]     = demo_inline_text;
    demos["script_inline_binary"]   = demo_inline_binary;
    demos["script_external_text"]   = demo_external_text;
    demos["script_external_binary"] = demo_external_binary;
    demos["animation"]              = demo_animation;
    demos["nan"]                    = demo_NaN;
    demos["segments"]               = demo_segments;
    demos["image"]                  = demo_image;
    demos["plotgroup"]              = demo_plotgroup;
    demos["fit"]                    = demo_fit;

    if(argc < 2) {
        printf("Usage: %s <demo_name>\n", argv[0]);
        printf("Choose one of the following demos:\n");
        typedef std::pair<std::string, void (*)(void)> demo_pair;
        for(const demo_pair &pair : demos) {
            printf("    %s\n", pair.first.c_str());
        }
        return 0;
    }

    std::string arg(argv[1]);
    if(!demos.count(arg)) {
        printf("No such demo '%s'\n", arg.c_str());
        return 1;
    }

    demos[arg]();
}
