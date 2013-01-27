#include <fstream>
#include <vector>
#if __cplusplus >= 201103
#include <tuple>
#endif

#include <boost/array.hpp>
#include <boost/tuple/tuple.hpp>

#include "gnuplot-iostream.h"

// for debugging
#include <typeinfo>
#include <cxxabi.h>

template <typename T>
std::string get_typename() {
	int status;
	char *name;
	name = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
	assert(!status);
	return std::string(name);
}

template <typename T>
void runtest(std::string header, const T &arg) {
	Gnuplot gp("cat");
	std::cout << "--- " << header << " -------------------------------------" << std::endl;
	std::cout << "ncols=" << gnuplotio::ArrayTraits<T>::ncols << std::endl;
	std::cout << "depth=" << gnuplotio::ArrayTraits<T>::depth << std::endl;
	std::cout << "val=" << get_typename<typename gnuplotio::ArrayTraits<T>::value_type>() << std::endl;
	std::cout << "bintype=[" << gp.binfmt(arg) << "]" << std::endl;
	std::cout << "range_type=" << get_typename<typename gnuplotio::ArrayTraits<T>::range_type>() << std::endl;
	gp.send(arg);
	gp.file(arg, "unittest-output/"+header+".txt");
	gp.binaryFile(arg, "unittest-output/"+header+".bin");
}

template <typename T>
void go(const T &arg) {
	auto range = gnuplotio::ArrayTraits<T>::get_range(arg);
	auto val = range.deref();
	gnuplotio::send_entry(std::cout, val);
	std::cout << std::endl;
}

int main() {
	gnuplotio::set_debug_array_print(true);

	const int NX=3;
	std::vector<double> vd;
	std::vector<int> vi;
	std::vector<float> vf;

	for(int x=0; x<NX; x++) {
		vd.push_back(x+7.1);
		vi.push_back(x+7.2);
		vf.push_back(x+7.3);
	}

	//go(boost::make_tuple(vd, vi));
	//runtest("vd,vi,vf", boost::make_tuple(vd, vi));

	runtest("pair{vf,btup{vd,pair{vi,vi},vf}}", std::make_pair(vf, boost::make_tuple(vd, std::make_pair(vi, vi), vf)));
#if __cplusplus >= 201103
	//runtest("pair{vf,btup{vd,pair{vi,vi},vf}}", std::make_pair(vf, std::make_tuple(vd, std::make_pair(vi, vi), vf)));
	go(std::make_pair(vf, std::make_tuple(vd, std::make_pair(vi, vi), vf)));
#endif

	std::cout << __cplusplus << std::endl;
}
