#include <fstream>
#include <vector>
#include <math.h>

// for debugging
#include <typeinfo>
#include <cxxabi.h>

#include <boost/utility.hpp>
#include <boost/array.hpp>

template <typename T>
class is_container {
    typedef char one;
    typedef long two;

    template <typename C> static one test(typename C::value_type *, typename C::const_iterator *);
    template <typename C> static two test(...);

public:
    enum { value = sizeof(test<T>(NULL, NULL)) == sizeof(char) };
};

template <class T>
typename boost::enable_if<is_container<T> >::type
a(const T &) {
	std::cout << "a:cont" << std::endl;
}

template <class T>
typename boost::enable_if_c<!is_container<T>::value>::type
a(const T &) {
	std::cout << "a:flat" << std::endl;
}

template <typename TI, typename TV>
class Range {
public:
	typedef TV value_type;

	bool is_end() { return it == end; }

	TI it, end;
};

template <typename T>
class ArrayTraits {
public:
	enum { is_container = false };
};

template <typename T>
class ArrayTraits<std::vector<T> > {
public:
	typedef Range<typename std::vector<T>::const_iterator, T> range_type;
	enum { is_container = true };

	static range_type get_range(const std::vector<T> &arg) {
		range_type ret;
		ret.it = arg.begin();
		ret.end = arg.end();
		return ret;
	}
};

template <typename T>
class ColumnsRangeGetter {
public:
	typedef typename ArrayTraits<T>::range_type range_type;
	enum { num_cols = 1 };

	static range_type get_range(const T &arg) {
		return ArrayTraits<T>::get_range(arg);
	}
};

template <typename T, typename U>
class ColumnsRangeGetter<std::pair<T, U> > {
public:
	typedef typename std::pair<typename ColumnsRangeGetter<T>::range_type, typename ColumnsRangeGetter<U>::range_type> range_type;
	enum { num_cols = ColumnsRangeGetter<T>::num_cols + ColumnsRangeGetter<U>::num_cols };

	static range_type get_range(const std::pair<T, U> &arg) {
		return std::make_pair(
			ColumnsRangeGetter<T>::get_range(arg.first),
			ColumnsRangeGetter<U>::get_range(arg.second));
	}
};

template <typename T>
class ColumnsRangeMethods {
public:
	typedef typename T::value_type value_type;

	static value_type deref(const T &arg) {
		return *(arg.it);
	}
};

template <typename T, typename U>
class ColumnsRangeMethods<std::pair<T, U> > {
public:
	typedef typename std::pair<typename ColumnsRangeMethods<T>::value_type, typename ColumnsRangeMethods<U>::value_type> value_type;

	static value_type deref(const std::pair<T, U> &arg) {
		return std::make_pair(
			ColumnsRangeMethods<T>::deref(arg.first),
			ColumnsRangeMethods<U>::deref(arg.second)
		);
	}
};

template <typename T>
std::string get_typename() {
	int status;
	char *name;
	name = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
	assert(!status);
	return std::string(name);
}

template <typename T>
void print_entry(const T &arg) {
	std::cout << "[" << get_typename<T>() << ":" << arg << "]";
}

template <typename T, typename U>
void print_entry(const std::pair<T, U> &arg) {
	print_entry(arg.first);
	std::cout << " ";
	print_entry(arg.second);
}

template <typename T>
void print_row(const T &arg) {
	typename ColumnsRangeMethods<T>::value_type row = ColumnsRangeMethods<T>::deref(arg);
	print_entry(row);
	std::cout << std::endl;
}

template <typename T>
void plot(const T &arg) {
	std::cout << "ncols=" << ColumnsRangeGetter<T>::num_cols << std::endl;
	std::cout << "range_type=" << get_typename<typename ColumnsRangeGetter<T>::range_type>() << std::endl;
	typename ColumnsRangeGetter<T>::range_type range = ColumnsRangeGetter<T>::get_range(arg);
	print_row(range);
}

int main() {
	const int NX=3, NY=4;
	std::vector<double> vd;
	std::vector<int> vi;
	std::vector<std::vector<double> > vvd(NX);
	std::vector<std::vector<int> > vvi(NX);
	std::vector<std::vector<std::vector<int> > > vvvi(NX);

	for(int x=0; x<NX; x++) {
		vd.push_back(x+7.5);
		vi.push_back(x+7);
		for(int y=0; y<NY; y++) {
			vvd[x].push_back(100+x*10+y);
			vvi[x].push_back(200+x*10+y);
			std::vector<int> tup;
			tup.push_back(300+x*10+y);
			tup.push_back(400+x*10+y);
			vvvi[x].push_back(tup);
		}
	}

	plot(std::make_pair(vd, vi));
	//plot(std::make_pair(vvd, std::make_pair(vvi, vvvi)));
}
