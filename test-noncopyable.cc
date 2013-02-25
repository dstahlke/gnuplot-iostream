#include <vector>

#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>

#include "gnuplot-iostream.h"

#define USE_CXX (__cplusplus >= 201103)

template <typename T>
class NonCopyable : boost::noncopyable, public std::vector<T> {
public:
	NonCopyable() { }
};

NonCopyable<double> make_nc() {
	NonCopyable<double> nc_y;
	for(int i=0; i<100; i++) {
		nc_y.push_back(i*i);
	}
	return nc_y;
}

int main() {
	Gnuplot gp("gnuplot -persist");
	NonCopyable<double> nc_x, nc_y;
	for(int i=0; i<100; i++) {
		nc_x.push_back(-i);
		nc_y.push_back(i*i);
	}
	gp << "plot '-', '-'\n";

	gp.send(nc_y);

	// These don't work because they make copies.
	//gp.send(std::make_pair(nc_x, nc_y));
	//gp.send(boost::make_tuple(nc_x, nc_y));
	// These work because they make references:
#if USE_CXX
	std::cout << "using std::tie" << std::endl;
	//gp.send(std::tie(nc_x, nc_y));
	// FIXME - doesn't work
	gp.send(std::forward_as_tuple(nc_x, make_nc()));
#else
	std::cout << "using boost::tie" << std::endl;
	gp.send(boost::tie(nc_x, nc_y));
#endif
}
