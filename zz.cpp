// g++ -o zz zz.cpp gnuplot++.cpp -Wall -I/usr/lib64/blitz/include -O0 -g

#include <blitz/array.h>
#include <math.h>
#include "gnuplot++.h"

int main() {
	Gnuplot gp;

//	blitz::Array<blitz::TinyVector<double, 2>, 1> arr(10);
//	blitz::firstIndex i;
//	arr[0] = i*i;
//	arr[1] = i*i*i;
//	blitz::Array<double, 1> a0(arr[0]);
//	blitz::Array<double, 1> a1(arr[1]);
//	//gp << "p '-' w lp, '-' w lp" << arr[0] << arr[1];
//	gp << "p '-' w lp, '-' w lp" << a0 << a1;

	blitz::Array<double, 2> arr(100, 100);
	blitz::firstIndex i;
	blitz::secondIndex j;
	arr = (i-50) * (j-50);
	gp << "set pm3d map; set palette";
	gp << "splot '-'" << arr;

//	blitz::Array<blitz::TinyVector<double, 2>, 2> arr(10);
//	blitz::firstIndex i;
//	blitz::secondIndex j;
//	arr[0] = (i-5)*(i-5) + (j-5)*(j-5);
//	arr[1] = i;
//	gp << "set pm3d map; set palette";
//	gp << "splot '-' w pm3d" << arr;

	Gnuplot gp2;
	for(;;) {
		float mx, my;
		int mb;
		gp.getMouse(mx, my, mb);
		printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);

		blitz::Array<double, 2> arr2(20, 20);
		blitz::firstIndex i;
		blitz::secondIndex j;
		arr2 = pow(pow(i*5-mx, 4) + pow(j*5-my, 4), 0.25);
		gp2 << "set pm3d; set palette";
		gp2 << "splot '-'" << arr2;
	}
}
