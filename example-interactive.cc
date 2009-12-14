// g++ -o example-interactive example-interactive.cc gnuplot-iostream.cc -Wall -Wextra -I/usr/lib64/blitz/include -O0 -g -lutil -lboost_iostreams

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
