#include <map>
#include <vector>

#include "gnuplot-iostream.h"

int main() {
	Gnuplot gp;

	std::vector<double> y_pts;
	for(int i=0; i<1000; i++) {
		double y = (i/500.0-1) * (i/500.0-1);
		y_pts.push_back(y);
	}

	gp << "set terminal png\n";
	gp << "set output 'my_graph_1.png'\n";
	gp << "p '-' w l, sin(x/200) w l\n";
	gp.send(y_pts.begin(), y_pts.end());

	std::map<double, double> xy_pts;
	for(int i=0; i<1000; i++) {
		double x = (i/500.0-1);
		double y = x*x*x;
		xy_pts[x] = y;
	}

	gp << "set output 'my_graph_2.png'\n";
	gp << "p '-' w l, sin(x*2*3.14159) w l\n";
	gp.send(xy_pts.begin(), xy_pts.end());
}
