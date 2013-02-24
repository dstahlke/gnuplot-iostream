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
#include <math.h>
// This must be included before gnuplot-iostream.h in order to support plotting blitz arrays.
#include <blitz/array.h>

#include "gnuplot-iostream.h"

// Yes, I'm including a *.cc file.  It contains main().
#include "examples-framework.cc"

void demo_blitz_basic() {
	// -persist option makes the window not disappear when your program exits
	Gnuplot gp("gnuplot -persist");

	blitz::Array<double, 2> arr(100, 100);
	{
		blitz::firstIndex i;
		blitz::secondIndex j;
		arr = (i-50) * (j-50);
	}
	gp << "set pm3d map; set palette" << std::endl;
	gp << "splot '-'" << std::endl;
	gp.send(arr);
}

void demo_blitz_waves_binary() {
	Gnuplot gp("gnuplot -persist");

	// example from Blitz manual:
	int N = 64, cycles = 3;
	double midpoint = (N-1)/2.;
	double omega = 2.0 * M_PI * cycles / double(N);
	double tau = - 10.0 / N;
	blitz::Array<double, 2> F(N, N);
	blitz::firstIndex i;
	blitz::secondIndex j;
	F = cos(omega * sqrt(pow2(i-midpoint) + pow2(j-midpoint)))
		* exp(tau * sqrt(pow2(i-midpoint) + pow2(j-midpoint)));

	gp << "splot '-' binary" << gp.binfmt(F) << "dx=10 dy=10 origin=(5,5,0) with pm3d notitle" << std::endl;
	gp.sendBinary(F);
}

void demo_blitz_sierpinski_binary() {
	Gnuplot gp("gnuplot -persist");

	int N = 256;
	blitz::Array<blitz::TinyVector<uint8_t, 4>, 2> F(N, N);
	for(int i=0; i<N; i++)
	for(int j=0; j<N; j++) {
		F(i, j)[0] = i;
		F(i, j)[1] = j;
		F(i, j)[2] = 0;
		F(i, j)[3] = (i&j) ? 0 : 255;
	}

	gp << "plot '-' binary" << gp.binfmt(F) << "with rgbalpha notitle" << std::endl;
	gp.sendBinary(F);
}

void demo_blitz_waves_binary_file() {
	Gnuplot gp("gnuplot -persist");

	// example from Blitz manual:
	int N = 64, cycles = 3;
	double midpoint = (N-1)/2.;
	double omega = 2.0 * M_PI * cycles / double(N);
	double tau = - 10.0 / N;
	blitz::Array<double, 2> F(N, N);
	blitz::firstIndex i;
	blitz::secondIndex j;
	F = cos(omega * sqrt(pow2(i-midpoint) + pow2(j-midpoint)))
		* exp(tau * sqrt(pow2(i-midpoint) + pow2(j-midpoint)));

	gp << "splot" << gp.binaryFile(F) << "dx=10 dy=10 origin=(5,5,0) with pm3d notitle" << std::endl;
}

void demo_blitz_sierpinski_binary_file() {
	Gnuplot gp("gnuplot -persist");

	int N = 256;
	blitz::Array<blitz::TinyVector<uint8_t, 4>, 2> F(N, N);
	for(int i=0; i<N; i++)
	for(int j=0; j<N; j++) {
		F(i, j)[0] = i;
		F(i, j)[1] = j;
		F(i, j)[2] = 0;
		F(i, j)[3] = (i&j) ? 0 : 255;
	}

	gp << "plot" << gp.binaryFile(F) << "with rgbalpha notitle" << std::endl;
}

void register_demos() {
	register_demo("basic",                        demo_blitz_basic);
	register_demo("waves_binary",           demo_blitz_waves_binary);
	register_demo("sierpinski_binary",      demo_blitz_sierpinski_binary);
	register_demo("waves_binary_file",      demo_blitz_waves_binary_file);
	register_demo("sierpinski_binary_file", demo_blitz_sierpinski_binary_file);
}
