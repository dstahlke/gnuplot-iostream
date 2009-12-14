#include <map>
#include <vector>

#include <math.h>

#include "gnuplot-iostream.h"

int main() {
	Gnuplot gp;
	gp << "set terminal png\n";

	std::vector<double> y_pts;
	for(int i=0; i<1000; i++) {
		double y = (i/500.0-1) * (i/500.0-1);
		y_pts.push_back(y);
	}

	gp << "set output 'my_graph_1.png'\n";
	gp << "p '-' w l, sin(x/200) w l\n";
	gp.send(y_pts.begin(), y_pts.end());

	std::map<double, double> xy_pts_A;
	for(double x=-2; x<2; x+=0.01) {
		double y = x*x*x;
		xy_pts_A[x] = y;
	}

	std::map<double, double> xy_pts_B;
	for(double alpha=0; alpha<1; alpha+=1.0/24.0) {
		double theta = alpha*2.0*3.14159;
		xy_pts_B[cos(theta)] = sin(theta);
	}

	gp << "set output 'my_graph_2.png'\n";
	gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
	gp << "p '-' w l t 'cubic', '-' w p t 'circle'\n";
	gp.send(xy_pts_A.begin(), xy_pts_A.end());
	gp.send(xy_pts_B.begin(), xy_pts_B.end());
}
