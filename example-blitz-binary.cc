/*
Copyright (c) 2009 Daniel Stahlke

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

// NOTE: this example requires blitz++

#include <blitz/array.h>
#include <math.h>
#define GNUPLOT_ENABLE_BLITZ
#include "gnuplot-iostream.h"

void test_waves() {
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

void test_sierpinski() {
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

int main() {
	test_waves();
	test_sierpinski();
}
