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

template <typename T>
void print_typeof(const T &) {
	std::cout << get_typename<T>() << std::endl;
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

template <typename Tuple, size_t idx>
struct TupUnwinder {
	typedef std::pair<
		typename TupUnwinder<Tuple, idx-1>::type,
		typename std::tuple_element<idx, Tuple>::type
	> type;
};

template <typename Tuple>
struct TupUnwinder<Tuple, 0> {
	typedef typename std::tuple_element<0, Tuple>::type type;
};

template <typename T>
void go(const T &arg) {
	std::cout << get_typename<typename TupUnwinder<T, 2>::type>() << std::endl;
}

int main() {
	std::tuple<int, double, char> tup3(10, 123.456, 'x');
	go(tup3);
}
