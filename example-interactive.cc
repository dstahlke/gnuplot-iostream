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

int main() {
	Gnuplot gp;

	blitz::Array<double, 2> arr(100, 100);
	{
		blitz::firstIndex i;
		blitz::secondIndex j;
		arr = (i-50) * (j-50);
	}
	gp << "set pm3d map; set palette" << std::endl;
	gp << "splot '-'" << std::endl;
	gp.send(arr);

	Gnuplot gp2;
	for(;;) {
		double mx, my;
		int mb;
		gp.getMouse(mx, my, mb);
		printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
		if(mb == 3) break;

		blitz::Array<float, 2> arr2(20, 20);
		{
			blitz::firstIndex i;
			blitz::secondIndex j;
			arr2 = pow(pow(i*5-mx, 4) + pow(j*5-my, 4), 0.25);
		}
		//gp2 << "set pm3d" << std::endl;
		//gp2 << "set palette" << std::endl;
		gp2 << "set hidden3d" << std::endl;
		gp2 << "splot '-' w l\n";
		gp2.send(arr2);
	}
}
