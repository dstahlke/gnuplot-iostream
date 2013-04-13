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

int main() {
	Gnuplot gp;
	NonCopyable<double> nc_x, nc_y;
	for(int i=0; i<100; i++) {
		nc_x.push_back(-i);
		nc_y.push_back(i*i);
	}
	gp << "plot '-', '-'\n";

	// These don't work because they make copies.
	//gp.send1d(std::make_pair(nc_x, nc_y));
	//gp.send1d(boost::make_tuple(nc_x, nc_y));
	// These work because they make references:
#if USE_CXX
	std::cout << "using std::tie" << std::endl;
	gp.send1d(std::tie(nc_y));
	gp.send1d(std::forward_as_tuple(nc_x, std::move(nc_y)));
#else
	std::cout << "using boost::tie" << std::endl;
	gp.send1d(nc_y);
	gp.send1d(boost::tie(nc_x, nc_y));
#endif
}
