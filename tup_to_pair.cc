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

void foo(const boost::tuples::null_type&) {};

template <typename H, typename T>
void foo(boost::tuples::cons<H, T>& x) {
	std::cout << x.get_head() << std::endl;
	foo(x.get_tail());
}

template <typename T>
class BoostTupToPairHelper {
public:
	typedef long pair_type; // FIXME
};

template <typename H, typename TL, typename TR>
class BoostTupToPairHelper<boost::tuples::cons<H, boost::tuples::cons<TL, TR> > > {
public:
	typedef typename std::pair<H, typename BoostTupToPairHelper<boost::tuples::cons<TL, TR> >::pair_type> pair_type;
};

template <typename H, typename TL>
class BoostTupToPairHelper<boost::tuples::cons<H, boost::tuples::cons<TL, boost::tuples::null_type> > > {
public:
	typedef typename std::pair<H, TL> pair_type;
};

template <typename T>
typename boost::disable_if<is_boost_tuple<T>, T>::type
tup_unwind(const T &arg) {
	return arg;
}

template <typename H, typename TL>
typename std::pair<H, TL>
tup_unwind(const boost::tuples::cons<H, boost::tuples::cons<TL, boost::tuples::null_type> > &arg) {
	typedef typename std::pair<H, TL> RetType;
	return RetType(arg.get_head(), arg.get_tail().get_head());
}

template <typename H, typename TL, typename TR>
typename BoostTupToPairHelper<boost::tuples::cons<H, boost::tuples::cons<TL, TR> > >::pair_type
tup_unwind(const boost::tuples::cons<H, boost::tuples::cons<TL, TR> > &arg) {
	typedef typename BoostTupToPairHelper<boost::tuples::cons<H, boost::tuples::cons<TL, TR> > >::pair_type RetType;
	return RetType(arg.get_head(), tup_unwind(arg.get_tail()));
}

int main() {
	boost::tuple<int, double, char> tup3(10, 123.456, 'x');
	boost::tuple<int, double> tup2(20, 3.14);
	boost::tuple<char, boost::tuple<int, double>, short> tup_tup('y', tup2, 99);
	int scalar = 5;
	//foo(tup3);
	print_typeof(tup_unwind(scalar));
	print_typeof(tup_unwind(tup2));
	gnuplotio::send_entry(std::cout, tup_unwind(tup2)); std::cout << std::endl;
	print_typeof(tup_unwind(tup3));
	gnuplotio::send_entry(std::cout, tup_unwind(tup3)); std::cout << std::endl;
	print_typeof(tup_unwind(tup_tup));
	//gnuplotio::send_entry(std::cout, tup_unwind(tup_tup)); std::cout << std::endl;
}
