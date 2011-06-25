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
//#define GNUPLOT_ENABLE_PTY
#include "gnuplot-iostream.h"

int main() {
	#ifdef GNUPLOT_ENABLE_PTY
	Gnuplot gp;
	#else
	// -persist option makes the window not disappear when your program exits
	Gnuplot gp("gnuplot -persist");
	#endif

	blitz::Array<double, 2> arr(100, 100);
	{
		blitz::firstIndex i;
		blitz::secondIndex j;
		arr = (i-50) * (j-50);
	}
	gp << "set pm3d map; set palette" << std::endl;
	gp << "splot '-'" << std::endl;
	gp.send(arr);

	#ifdef GNUPLOT_ENABLE_PTY
	Gnuplot gp2;
	for(;;) {
		double mx, my;
		int mb;
		gp.getMouse(mx, my, mb);
		printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
		if(mb == 3) break;

		blitz::Array<blitz::TinyVector<double, 2>, 2> arr2(50, 50);
		{
			blitz::firstIndex i;
			blitz::secondIndex j;
			arr2[0] = pow(i*2-mx, 4);
			arr2[1] = pow(j*2-my, 4);
		}
		//gp2 << "set pm3d" << std::endl;
		//gp2 << "set palette" << std::endl;
		gp2 << "set hidden3d" << std::endl;
		gp2 << "splot '-' using (log($1*$2+0.1)) with lines\n";
		gp2.send(arr2);
	}
	#endif
}
