#include <iostream>

#include <boost/tuple/tuple.hpp>

#include "gnuplot-iostream.h"

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

//// FIXME - why doesn't boost::mpl::size_t work?
//template<std::size_t> struct int_{};
////#define int_ boost::mpl::size_t
//
//template <typename Tuple, size_t idx>
//void print_tup(const Tuple &tup, int_<idx>) {
//	print_tup(tup, int_<idx-1>());
//	std::cout << boost::get<idx>(tup) << std::endl;
//}
//
//template <typename Tuple>
//void print_tup(const Tuple &tup, int_<0>) {
//	std::cout << boost::get<0>(tup) << std::endl;
//}

void foo(const boost::tuples::null_type&) {};

template <typename H, typename T>
void foo(boost::tuples::cons<H, T>& x) {
	std::cout << x.get_head() << std::endl;
	foo(x.get_tail());
}

template <typename T>
class TupToPair {
public:
	typedef long pair_type; // FIXME
};

template <typename H, typename TL, typename TR>
class TupToPair<boost::tuples::cons<H, boost::tuples::cons<TL, TR> > > {
public:
	typedef typename std::pair<H, typename TupToPair<boost::tuples::cons<TL, TR> >::pair_type> pair_type;
};

template <typename H, typename TL>
class TupToPair<boost::tuples::cons<H, boost::tuples::cons<TL, boost::tuples::null_type> > > {
public:
	typedef typename std::pair<H, TL> pair_type;
};

template <typename H, typename T>
void bar(boost::tuples::cons<H, T>) {
	std::cout << get_typename<typename TupToPair<boost::tuples::cons<H, T> >::pair_type>() << std::endl;
}

int main() {
	boost::tuple<int, double, char> tup;
	tup.get<0>() = 10;
	tup.get<1>() = 123.456;
	tup.get<2>() = 'x';
	//print_tup(tup, int_<2>());
	foo(tup);
	//std::cout << get_typename<TupToPair<boost::tuple<short, int, double, char>::tail_type >::pair_type >() << std::endl;
	bar(tup);
}
