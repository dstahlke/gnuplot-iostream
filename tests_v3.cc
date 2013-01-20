#include <fstream>
#include <vector>
#include <stdexcept>

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

template <typename T>
class ArrayTraits;

template <typename TI, typename TV, typename Enable=void>
class RangeBase {
public:
	RangeBase() { }
	RangeBase(const TI &_it, const TI &_end) : it(_it), end(_end) { }

	typedef TV value_type;
	typedef typename ArrayTraits<TV>::range_type subiter_type;
	enum { is_container = ArrayTraits<TV>::is_container };

	bool is_end() { return it == end; }

	void inc() { ++it; }

	value_type deref() const {
		return *it;
	}

	subiter_type deref_subiter() const {
		return ArrayTraits<TV>::get_range(*it);
	}

private:
	TI it, end;
};

template <typename TI, typename TV>
class Range : public RangeBase<TI, TV> {
public:
	Range() { }
	Range(const TI &_it, const TI &_end) : RangeBase<TI, TV>(_it, _end) { }
};

template <typename T>
class ArrayTraits {
public:
	typedef void range_type;
	enum { is_container = false };

	static void get_range(const T &) { }
};

template <typename T>
class ArrayTraits<std::vector<T> > {
public:
	typedef Range<typename std::vector<T>::const_iterator, T> range_type;
	enum { is_container = true };

	static range_type get_range(const std::vector<T> &arg) {
		return range_type(arg.begin(), arg.end());
	}
};

template <typename T, typename U>
class PairOfRange {
public:
	PairOfRange() { }
	PairOfRange(const T &_l, const U &_r) : l(_l), r(_r) { }

	enum { is_container = T::is_container && U::is_container };

	typedef std::pair<typename T::value_type, typename U::value_type> value_type;
	typedef PairOfRange<typename T::subiter_type, typename U::subiter_type> subiter_type;

	bool is_end() {
		bool el = l.is_end();
		bool er = r.is_end();
		if(el != er) {
			throw std::length_error("columns were different lengths");
		}
		return el;
	}

	void inc() {
		l.inc();
		r.inc();
	}

	value_type deref() const {
		return std::make_pair(l.deref(), r.deref());
	}

	subiter_type deref_subiter() const {
		return subiter_type(l.deref_subiter(), r.deref_subiter());
	}

private:
	T l;
	U r;
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
	typedef PairOfRange<typename ColumnsRangeGetter<T>::range_type, typename ColumnsRangeGetter<U>::range_type> range_type;
	enum { num_cols = ColumnsRangeGetter<T>::num_cols + ColumnsRangeGetter<U>::num_cols };

	static range_type get_range(const std::pair<T, U> &arg) {
		return range_type(
			ColumnsRangeGetter<T>::get_range(arg.first),
			ColumnsRangeGetter<U>::get_range(arg.second)
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
typename boost::disable_if_c<T::is_container>::type
print_block(T arg) {
	while(!arg.is_end()) {
		print_entry(arg.deref());
		std::cout << std::endl;
		arg.inc();
	}
}

template <typename T>
typename boost::enable_if_c<T::is_container>::type
print_block(T arg) {
	while(!arg.is_end()) {
		std::cout << "<block>" << std::endl;
		typename T::subiter_type sub = arg.deref_subiter();
		print_block(sub);
		std::cout << std::endl;
		arg.inc();
	}
}

template <typename T>
void plot(const T &arg) {
	std::cout << "ncols=" << ColumnsRangeGetter<T>::num_cols << std::endl;
	std::cout << "range_type=" << get_typename<typename ColumnsRangeGetter<T>::range_type>() << std::endl;
	typename ColumnsRangeGetter<T>::range_type range = ColumnsRangeGetter<T>::get_range(arg);
	print_block(range);
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
	plot(std::make_pair(vvd, vvi));
	//plot(std::make_pair(vvd, std::make_pair(vvi, vvvi)));
}
