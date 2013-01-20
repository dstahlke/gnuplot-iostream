#include <fstream>
#include <vector>
#include <stdexcept>

// for debugging
#include <typeinfo>
#include <cxxabi.h>

#include <boost/utility.hpp>
#include <boost/array.hpp>

template <typename T>
class is_like_stl_container {
    typedef char one;
    typedef long two;

    template <typename C> static one test(typename C::value_type *, typename C::const_iterator *);
    template <typename C> static two test(...);

public:
    enum { value = sizeof(test<T>(NULL, NULL)) == sizeof(char) };
};

// Error messages involving this stem from treating something that was not a container as if it
// was.
class WasNotContainer { };

template <typename T, typename Enable=void>
class ArrayTraits {
public:
	typedef WasNotContainer range_type;
	enum { is_container = false };

	static range_type get_range(const T &) { }
};

template <typename T>
class ArrayTraits<T, typename boost::enable_if<is_like_stl_container<T> >::type> {
	template <typename TI, typename TV, typename Enable=void>
	class STLIteratorRange {
	public:
		STLIteratorRange() { }
		STLIteratorRange(const TI &_it, const TI &_end) : it(_it), end(_end) { }

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

public:
	typedef STLIteratorRange<typename T::const_iterator, typename T::value_type> range_type;
	enum { is_container = true };

	static range_type get_range(const T &arg) {
		return range_type(arg.begin(), arg.end());
	}
};

template <typename T, size_t N>
class ArrayTraits<T[N]> {
	class CArrayRange {
	public:
		CArrayRange() : p(NULL), it(0) { }
		CArrayRange(const T *_p) : p(_p), it(0) { }

		typedef T value_type;
		typedef typename ArrayTraits<T>::range_type subiter_type;
		enum { is_container = ArrayTraits<T>::is_container };

		bool is_end() { return it == N; }

		void inc() { ++it; }

		value_type deref() const {
			return p[it];
		}

		subiter_type deref_subiter() const {
			return ArrayTraits<T>::get_range(p[it]);
		}

	private:
		const T *p;
		size_t it;
	};
public:
	typedef CArrayRange range_type;
	enum { is_container = true };

	static range_type get_range(const T (&arg)[N]) {
		return range_type(arg);
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
	int ai[NX];
	boost::array<int, NX> bi;

	for(int x=0; x<NX; x++) {
		vd.push_back(x+7.5);
		vi.push_back(x+7);
		ai[x] = x+7;
		bi[x] = x+70;
		for(int y=0; y<NY; y++) {
			vvd[x].push_back(100+x*10+y);
			vvi[x].push_back(200+x*10+y);
			std::vector<int> tup;
			tup.push_back(300+x*10+y);
			tup.push_back(400+x*10+y);
			vvvi[x].push_back(tup);
		}
	}

	plot(std::make_pair(vd, std::make_pair(vi, bi)));
	plot(std::make_pair(vvd, vvi));
	plot(ai);
	//plot(std::make_pair(ai, bi));
	//plot(std::make_pair(vvd, std::make_pair(vvi, vvvi)));
}
