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

template <typename T>
class is_boost_tuple {
    typedef char one;
    typedef long two;

    template <typename C> static one test(typename C::head_type *, typename C::tail_type *);
    template <typename C> static two test(...);

public:
    static const bool value = sizeof(test<T>(NULL, NULL)) == sizeof(char);
	typedef boost::mpl::bool_<value> type;
};

template <typename T>
struct is_boost_tuple_nulltype {
	static const bool value = false;
	typedef boost::mpl::bool_<value> type;
};

template <>
struct is_boost_tuple_nulltype<boost::tuples::null_type> {
	static const bool value = true;
	typedef boost::mpl::bool_<value> type;
};

template <typename T, typename Enable=void>
struct UnwindHelper {
	typedef T type;

	static type unwind(const T &arg) { return arg; }
};

template <typename T>
struct UnwindHelper<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			boost::mpl::not_<is_boost_tuple_nulltype<typename T::tail_type> >
		>
	>::type
> {
	typedef std::pair<
			typename UnwindHelper<typename T::head_type>::type,
			typename UnwindHelper<typename T::tail_type>::type
		> type;

	static type unwind(const T &arg) {
		return type(
			UnwindHelper<typename T::head_type>::unwind(arg.get_head()),
			UnwindHelper<typename T::tail_type>::unwind(arg.get_tail())
		);
	}
};

template <typename T>
struct UnwindHelper<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			is_boost_tuple_nulltype<typename T::tail_type>
		>
	>::type
> {
	typedef typename UnwindHelper<typename T::head_type>::type type;

	static type unwind(const T &arg) {
		return UnwindHelper<typename T::head_type>::unwind(arg.get_head());
	}
};

template <typename T>
typename UnwindHelper<T>::type unwind(const T &arg) {
	return UnwindHelper<T>::unwind(arg);
}

template <typename T>
void go(const T &arg) {
	std::cout << get_typename<typename UnwindHelper<T>::type>() << std::endl;
	std::cout << "    ";
	gnuplotio::send_entry(std::cout, unwind(arg));
	std::cout << std::endl;
}

int main() {
	boost::tuple<int, double, char> tup3(10, 123.456, 'x');
	boost::tuple<int, double> tup2(20, 3.14);
	boost::tuple<char, boost::tuple<int, double>, short> tup_tup('y', tup2, 99);
	int scalar = 5;

	go(scalar);
	go(tup2);
	go(tup3);
	go(tup_tup);
	go(std::make_pair(tup2, tup3));
}
