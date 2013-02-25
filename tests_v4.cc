#include <fstream>
#include <vector>
#if __cplusplus >= 201103
#include <tuple>
#endif

#include <boost/array.hpp>
#include <boost/tuple/tuple.hpp>

// my debug routines:
#include <gcc-features.h>

#include "gnuplot-iostream.h"

using namespace gnuplotio;

template <typename T>
void go(const T &) {
	typedef typename ArrayTraits<int (&) [10]>::range_type U;
	std::cout << get_typename<T>() << std::endl;
	std::cout << get_typename<U>() << std::endl;
}

int main() {
	//int x[10], y[10];
	//auto t = std::tie(x, y);
	//go(t);

	typedef typename ArrayTraits<int (&) [10]>::range_type U;
	std::cout << get_typename<U>() << std::endl;
}
