// g++ -o zz zz.cpp gnuplot++.cpp -Wall -I/usr/lib64/blitz/include -O0 -g

#include <blitz/array.h>
#include "gnuplot++.h"

int main() {
	Gnuplot gp;
	blitz::Array<blitz::TinyVector<double, 2>, 1> arr(10);
	blitz::firstIndex i;
	arr[0] = i*i;
	arr[1] = i*i*i;
	blitz::Array<double, 1> a0(arr[0]);
	blitz::Array<double, 1> a1(arr[1]);
	//gp << "p '-' w lp, '-' w lp" << arr[0] << arr[1];
	gp << "p '-' w lp, '-' w lp" << a0 << a1;

	float mx, my;
	int mb;
	for(int i=0; i<3; i++) {
		gp.getMouse(mx, my, mb);
		printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
	}
}
