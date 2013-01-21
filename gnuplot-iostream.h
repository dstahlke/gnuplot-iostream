// vim:foldmethod=marker

/*
Copyright (c) 2009 Daniel Stahlke

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
	FIXME/TODO:
		What version of boost is currently required?
		Use namespace.
		Verify that nested containers have consistent lengths between slices (at least along
		the column dimension, sometimes it might be okay for blocks to be different lengths).
		Move binary stuff to new infrastructure.
*/

#ifndef GNUPLOT_IOSTREAM_H
#define GNUPLOT_IOSTREAM_H

/// {{{1 Includes and defines

// C system includes
#include <stdio.h>
#ifdef GNUPLOT_ENABLE_PTY
#include <termios.h>
#include <unistd.h>
#include <pty.h>
#endif // GNUPLOT_ENABLE_PTY

// C++ system includes
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <iomanip>
#include <vector>

// library includes: double quotes make cpplint not complain
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/version.hpp>
#include <boost/utility.hpp>
#include <boost/mpl/bool.hpp>
// FIXME - this should be the only place this macro is used.  Elsewhere, just detect the blitz
// header guard.
#ifdef GNUPLOT_ENABLE_BLITZ
#include <blitz/array.h>
#endif

// This is the version of boost which has v3 of the filesystem libraries by default.
#if BOOST_VERSION >= 104600
#define GNUPLOT_USE_TMPFILE
#include <boost/filesystem.hpp>
#endif // BOOST_VERSION

// Patch for Windows by Damien Loison
// FIXME - check if already defined
#ifdef WIN32
#define PCLOSE _pclose
#define POPEN  _popen
#define FILENO _fileno
#else
#define PCLOSE pclose
#define POPEN  popen
#define FILENO fileno
#endif

/// }}}1

/// {{{1 Tmpfile helper class
#ifdef GNUPLOT_USE_TMPFILE
// RAII temporary file.  File is removed when this object goes out of scope.
class GnuplotTmpfile {
public:
	GnuplotTmpfile() :
		file(boost::filesystem::unique_path(
			boost::filesystem::temp_directory_path() /
			"tmp-gnuplot-%%%%-%%%%-%%%%-%%%%"))
	{ }

private:
	// noncopyable
	GnuplotTmpfile(const GnuplotTmpfile &);
	const GnuplotTmpfile& operator=(const GnuplotTmpfile &);

public:
	~GnuplotTmpfile() {
		// it is never good to throw exceptions from a destructor
		try {
			remove(file);
		} catch(const std::exception &e) {
			std::cerr << "Failed to remove temporary file " << file << std::endl;
		}
	}

public:
	boost::filesystem::path file;
};
#endif // GNUPLOT_USE_TMPFILE
/// }}}1

/// {{{1 Feedback helper classes
// Used for reading stuff sent from gnuplot via gnuplot's "print" function.
class GnuplotFeedback {
public:
	GnuplotFeedback() { }
	virtual ~GnuplotFeedback() { }
	virtual std::string filename() const = 0;
	virtual FILE *handle() const = 0;

private:
	// noncopyable
	GnuplotFeedback(const GnuplotFeedback &);
	const GnuplotFeedback& operator=(const GnuplotFeedback &);
};

#ifdef GNUPLOT_ENABLE_PTY
#define GNUPLOT_ENABLE_FEEDBACK
class GnuplotFeedbackPty : public GnuplotFeedback {
public:
	explicit GnuplotFeedbackPty(bool debug_messages) :
		pty_fn(),
		pty_fh(NULL),
		master_fd(-1),
		slave_fd(-1)
	{
	// adapted from http://www.gnuplot.info/files/gpReadMouseTest.c
		if(0 > openpty(&master_fd, &slave_fd, NULL, NULL, NULL)) {
			perror("openpty");
			throw std::runtime_error("openpty failed");
		}
		char pty_fn_buf[1024];
		if(ttyname_r(slave_fd, pty_fn_buf, 1024)) {
			perror("ttyname_r");
			throw std::runtime_error("ttyname failed");
		}
		pty_fn = std::string(pty_fn_buf);
		if(debug_messages) {
			std::cerr << "feedback_fn=" << pty_fn << std::endl;
		}

		// disable echo
		struct termios tios;
		if(tcgetattr(slave_fd, &tios) < 0) {
			perror("tcgetattr");
			throw std::runtime_error("tcgetattr failed");
		}
		tios.c_lflag &= ~(ECHO | ECHONL);
		if(tcsetattr(slave_fd, TCSAFLUSH, &tios) < 0) {
			perror("tcsetattr");
			throw std::runtime_error("tcsetattr failed");
		}

		pty_fh = fdopen(master_fd, "r");
		if(!pty_fh) {
			throw std::runtime_error("fdopen failed");
		}
	}

private:
	// noncopyable
	GnuplotFeedbackPty(const GnuplotFeedbackPty &);
	const GnuplotFeedbackPty& operator=(const GnuplotFeedbackPty &);

public:
	~GnuplotFeedbackPty() {
		if(pty_fh) fclose(pty_fh);
		if(master_fd > 0) ::close(master_fd);
		if(slave_fd  > 0) ::close(slave_fd);
	}

	std::string filename() const {
		return pty_fn;
	}

	FILE *handle() const {
		return pty_fh;
	}

private:
	std::string pty_fn;
	FILE *pty_fh;
	int master_fd, slave_fd;
};
//#elif defined GNUPLOT_USE_TMPFILE
//// Currently this doesn't work since fscanf doesn't block (need something like "tail -f")
//#define GNUPLOT_ENABLE_FEEDBACK
//class GnuplotFeedbackTmpfile : public GnuplotFeedback {
//public:
//	explicit GnuplotFeedbackTmpfile(bool debug_messages) :
//		tmp_file(),
//		fh(NULL)
//	{
//		if(debug_messages) {
//			std::cerr << "feedback_fn=" << filename() << std::endl;
//		}
//		fh = fopen(filename().c_str(), "a");
//	}
//
//	~GnuplotFeedbackTmpfile() {
//		fclose(fh);
//	}
//
//private:
//	// noncopyable
//	GnuplotFeedbackTmpfile(const GnuplotFeedbackTmpfile &);
//	const GnuplotFeedbackTmpfile& operator=(const GnuplotFeedbackTmpfile &);
//
//public:
//	std::string filename() const {
//		return tmp_file.file.string();
//	}
//
//	FILE *handle() const {
//		return fh;
//	}
//
//private:
//	GnuplotTmpfile tmp_file;
//	FILE *fh;
//};
#endif // GNUPLOT_ENABLE_PTY, GNUPLOT_USE_TMPFILE
/// }}}1

/// {{{1 Traits and printers for scalar datatypes
template <class T>
class GnuplotEntry {
public:
	static const bool is_tuple = false;

	static std::string formatCode();

	static void send(std::ostream &stream, const T &v) {
		stream << v;
	}
};

template<> std::string GnuplotEntry<   float>::formatCode() { return "%float"; }
template<> std::string GnuplotEntry<  double>::formatCode() { return "%double"; }
template<> std::string GnuplotEntry<  int8_t>::formatCode() { return "%int8"; }
template<> std::string GnuplotEntry< uint8_t>::formatCode() { return "%uint8"; }
template<> std::string GnuplotEntry< int16_t>::formatCode() { return "%int16"; }
template<> std::string GnuplotEntry<uint16_t>::formatCode() { return "%uint16"; }
template<> std::string GnuplotEntry< int32_t>::formatCode() { return "%int32"; }
template<> std::string GnuplotEntry<uint32_t>::formatCode() { return "%uint32"; }
template<> std::string GnuplotEntry< int64_t>::formatCode() { return "%int64"; }
template<> std::string GnuplotEntry<uint64_t>::formatCode() { return "%uint64"; }

template <class T, class U>
class GnuplotEntry<std::pair<T, U> > {
public:
	static const bool is_tuple = true;

	static std::string formatCode() {
		return GnuplotEntry<T>::formatCode() + GnuplotEntry<U>::formatCode();
	}

	static void send(std::ostream &stream, const std::pair<T, U> &v) {
		stream << v.first << " " << v.second;
	}
};

#ifdef GNUPLOT_ENABLE_BLITZ
template <class T, int N>
class GnuplotEntry<blitz::TinyVector<T, N> > {
public:
	static const bool is_tuple = true;

	static std::string formatCode() {
		std::ostringstream tmp;
		for(int i=0; i<N; i++) {
			tmp << GnuplotEntry<T>::formatCode();
		}
		return tmp.str();
	}

	static void send(std::ostream &stream, const blitz::TinyVector<T, N> &v) {
		for(int i=0; i<N; i++) {
			if(i) stream << " ";
			stream << v[i];
		}
	}
};
#endif // GNUPLOT_ENABLE_BLITZ
/// }}}1

/// {{{1 New array writer stuff

/// {{{2 Basic traits helpers

// FIXME - maybe use BOOST_MPL_HAS_XXX_TRAIT_DEF
template <typename T>
class is_like_stl_container {
    typedef char one;
    typedef long two;

    template <typename C> static one test(typename C::value_type *, typename C::const_iterator *);
    template <typename C> static two test(...);

public:
    static const bool value = sizeof(test<T>(NULL, NULL)) == sizeof(char);
	typedef boost::mpl::bool_<value> type;
};

// http://stackoverflow.com/a/1007175/1048959
template<typename T> struct has_attrib_n_rows {
    struct Fallback { int n_rows; };
    struct Derived : T, Fallback { };

    template<typename C, C> struct ChT;

    template<typename C> static char (&f(ChT<int Fallback::*, &C::n_rows>*))[1];
    template<typename C> static char (&f(...))[2];

    static bool const value = sizeof(f<Derived>(0)) == 2;
	typedef boost::mpl::bool_<value> type;
};

template<typename T> struct has_attrib_n_cols {
    struct Fallback { int n_cols; };
    struct Derived : T, Fallback { };

    template<typename C, C> struct ChT;

    template<typename C> static char (&f(ChT<int Fallback::*, &C::n_cols>*))[1];
    template<typename C> static char (&f(...))[2];

    static bool const value = sizeof(f<Derived>(0)) == 2;
	typedef boost::mpl::bool_<value> type;
};

template <typename T>
struct is_armadillo_mat {
	static const bool value = has_attrib_n_rows<T>::value && has_attrib_n_cols<T>::value;
	typedef boost::mpl::bool_<value> type;
};

/// }}}2

/// {{{2 ArrayTraits and Range classes

// Error messages involving this stem from treating something that was not a container as if it
// was.  This is only here to allow compiliation without errors in normal cases.
// FIXME - make use of static assertions
class WasNotContainer { };

// The unspecialized version of this class gives traits for things that are *not* arrays.
template <typename T, typename Enable=void>
class ArrayTraits {
public:
	typedef WasNotContainer value_type;
	typedef WasNotContainer range_type;
	static const bool val_is_tuple = false;
	static const bool is_container = false;
	static const size_t depth = 0;
	static const size_t ncols = 1; // FIXME - eventually remove this, it is not really needed

	static range_type get_range(const T &) {
		throw std::invalid_argument("argument was not a container");
	}
};

template <typename V>
class ArrayTraitsDefaults {
public:
	typedef V value_type;
	static const bool val_is_tuple = GnuplotEntry<V>::is_tuple;
	static const bool is_container = true;
	static const size_t depth = ArrayTraits<V>::depth + 1;
	static const size_t ncols = 1;
};

template <typename TI, typename TV>
class IteratorRange {
public:
	IteratorRange() { }
	IteratorRange(const TI &_it, const TI &_end) : it(_it), end(_end) { }

	typedef TV value_type;
	typedef typename ArrayTraits<TV>::range_type subiter_type;
	static const bool is_container = ArrayTraits<TV>::is_container;

	bool is_end() const { return it == end; }

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

template <typename T>
class ArrayTraits<T, typename boost::enable_if<
	boost::mpl::and_<
		is_like_stl_container<T>,
		boost::mpl::not_<is_armadillo_mat<T> >
	>
>::type> : public ArrayTraitsDefaults<typename T::value_type> {
public:
	typedef IteratorRange<typename T::const_iterator, typename T::value_type> range_type;

	static range_type get_range(const T &arg) {
		return range_type(arg.begin(), arg.end());
	}
};

template <typename T, size_t N>
class ArrayTraits<T[N]> : public ArrayTraitsDefaults<T> {
public:
	typedef IteratorRange<const T*, T> range_type;

	static range_type get_range(const T (&arg)[N]) {
		return range_type(arg, arg+N);
	}
};

template <typename RT, typename RU>
class PairOfRange {
	template <typename T, typename U, typename DoBinary>
	friend void deref_and_print(std::ostream &, const PairOfRange<T, U> &, DoBinary);

public:
	PairOfRange() { }
	PairOfRange(const RT &_l, const RU &_r) : l(_l), r(_r) { }

	static const bool is_container = RT::is_container && RU::is_container;

	typedef std::pair<typename RT::value_type, typename RU::value_type> value_type;
	typedef PairOfRange<typename RT::subiter_type, typename RU::subiter_type> subiter_type;

	bool is_end() const {
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
	RT l;
	RU r;
};

template <typename T, typename U>
class ArrayTraits<std::pair<T, U> > {
public:
	typedef PairOfRange<typename ArrayTraits<T>::range_type, typename ArrayTraits<U>::range_type> range_type;
	typedef std::pair<typename ArrayTraits<T>::value_type, typename ArrayTraits<U>::value_type> value_type;
	static const bool val_is_tuple = true;
	static const bool is_container = ArrayTraits<T>::is_container && ArrayTraits<U>::is_container;
	// It is allowed for l_depth != r_depth, for example one column could be 'double' and the
	// other column could be 'vector<double>'.
	static const size_t l_depth = ArrayTraits<T>::depth;
	static const size_t r_depth = ArrayTraits<U>::depth;
	static const size_t depth = (l_depth < r_depth) ? l_depth : r_depth;
	static const size_t ncols = ArrayTraits<T>::ncols + ArrayTraits<U>::ncols;

	static range_type get_range(const std::pair<T, U> &arg) {
		return range_type(
			ArrayTraits<T>::get_range(arg.first),
			ArrayTraits<U>::get_range(arg.second)
		);
	}
};

template <typename RT>
class VecOfRange {
	template <typename T, typename DoBinary>
	friend void deref_and_print(std::ostream &, const VecOfRange<T> &, DoBinary);

public:
	VecOfRange() { }
	VecOfRange(const std::vector<RT> &_rvec) : rvec(_rvec) { }

	static const bool is_container = RT::is_container;

	typedef std::vector<typename RT::value_type> value_type;
	typedef VecOfRange<typename RT::subiter_type> subiter_type;

	bool is_end() const {
		if(rvec.empty()) return true;
		bool ret = rvec[0].is_end();
		for(size_t i=1; i<rvec.size(); i++) {
			if(ret != rvec[i].is_end()) {
				throw std::length_error("columns were different lengths");
			}
		}
		return ret;
	}

	void inc() {
		for(size_t i=0; i<rvec.size(); i++) {
			rvec[i].inc();
		}
	}

	value_type deref() const {
		value_type ret(rvec.size());
		for(size_t i=0; i<rvec.size(); i++) {
			ret[i] = rvec[i].deref();
		}
		return ret;
	}

	subiter_type deref_subiter() const {
		std::vector<typename RT::subiter_type> subvec(rvec.size());
		for(size_t i=0; i<rvec.size(); i++) {
			subvec[i] = rvec[i].deref_subiter();
		}
		return subiter_type(subvec);
	}

private:
	std::vector<RT> rvec;
};

template <typename T>
VecOfRange<typename ArrayTraits<T>::range_type::subiter_type>
get_columns_range(const T &arg) {
	typedef typename ArrayTraits<T>::range_type::subiter_type U;
	std::vector<U> rvec;
	typename ArrayTraits<T>::range_type outer = ArrayTraits<T>::get_range(arg);
	while(!outer.is_end()) {
		rvec.push_back(outer.deref_subiter());
		outer.inc();
	}
	VecOfRange<U> ret(rvec);
	return ret;
}

/// }}}2

/// {{{2 Armadillo support

#ifdef ARMA_INCLUDES
#ifndef GNUPLOT_H_ARMA
#define GNUPLOT_H_ARMA
// FIXME - handle Row, Cube, Field

template <typename T>
class ArrayTraits<arma::Mat<T> > : public ArrayTraitsDefaults<T> {
	class ArmaMatRange {
	public:
		ArmaMatRange() : p(NULL), it(0) { }
		ArmaMatRange(const arma::Mat<T> *_p) : p(_p), it(0) { }

		typedef T value_type;
		typedef IteratorRange<typename arma::Mat<T>::const_row_iterator, T> subiter_type;
		static const bool is_container = true;

		bool is_end() const { return it == p->n_rows; }

		void inc() { ++it; }

		value_type deref() const {
			throw std::logic_error("can't call deref on an armadillo slice");
		}

		subiter_type deref_subiter() const {
			return subiter_type(p->begin_row(it), p->end_row(it));
		}

	private:
		const arma::Mat<T> *p;
		size_t it;
	};
public:
	typedef ArmaMatRange range_type;

	static range_type get_range(const arma::Mat<T> &arg) {
		//std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
		return range_type(&arg);
	}
};

template <typename T>
class ArrayTraits<arma::Col<T> > : public ArrayTraitsDefaults<T> {
public:
	typedef IteratorRange<typename arma::Col<T>::const_iterator, T> range_type;

	static range_type get_range(const arma::Col<T> &arg) {
		//std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
		return range_type(arg.begin(), arg.end());
	}
};
#endif // GNUPLOT_H_ARMA
#endif // ARMA_INCLUDES

/// }}}2

/// {{{2 Blitz support

#ifdef BZ_BLITZ_H
#ifndef GNUPLOT_H_BLITZ
#define GNUPLOT_H_BLITZ
// FIXME - raise static error if possible
class WasPartialSlice { };

template <typename T, int ArrayDim, int SliceDim>
class BlitzIterator {
public:
	BlitzIterator() : p(NULL) { }
	BlitzIterator(
		const blitz::Array<T, ArrayDim> *_p,
		const blitz::TinyVector<int, ArrayDim> _idx
	) : p(_p), idx(_idx) { }

	typedef WasPartialSlice value_type;
	typedef BlitzIterator<T, ArrayDim, SliceDim-1> subiter_type;
	static const bool is_container = true;

	// FIXME - handle one-based arrays
	bool is_end() const {
		return idx[ArrayDim-SliceDim] == p->shape()[ArrayDim-SliceDim];
	}

	void inc() {
		++idx[ArrayDim-SliceDim];
	}

	value_type deref() const {
		throw std::logic_error("cannot deref a blitz slice");
	}

	subiter_type deref_subiter() const {
		return BlitzIterator<T, ArrayDim, SliceDim-1>(p, idx);
	}

private:
	const blitz::Array<T, ArrayDim> *p;
	blitz::TinyVector<int, ArrayDim> idx;
};

template <typename T, int ArrayDim>
class BlitzIterator<T, ArrayDim, 1> {
public:
	BlitzIterator() : p(NULL) { }
	BlitzIterator(
		const blitz::Array<T, ArrayDim> *_p,
		const blitz::TinyVector<int, ArrayDim> _idx
	) : p(_p), idx(_idx) { }

	typedef T value_type;
	typedef WasNotContainer subiter_type;
	static const bool is_container = false;

	// FIXME - handle one-based arrays
	bool is_end() const {
		return idx[ArrayDim-1] == p->shape()[ArrayDim-1];
	}

	void inc() {
		++idx[ArrayDim-1];
	}

	value_type deref() const {
		return (*p)(idx);
	}

	subiter_type deref_subiter() const {
		throw std::invalid_argument("argument was not a container");
	}

private:
	const blitz::Array<T, ArrayDim> *p;
	blitz::TinyVector<int, ArrayDim> idx;
};

template <typename T, int ArrayDim>
class ArrayTraits<blitz::Array<T, ArrayDim> > : public ArrayTraitsDefaults<T> {
public:
	typedef BlitzIterator<T, ArrayDim, ArrayDim> range_type;

	static range_type get_range(const blitz::Array<T, ArrayDim> &arg) {
		blitz::TinyVector<int, ArrayDim> start_idx;
		start_idx = 0;
		return range_type(&arg, start_idx);
	}
};
#endif // GNUPLOT_H_BLITZ
#endif // BZ_BLITZ_H

/// }}}2

/// {{{2 Array printing functions

template <typename T>
void send_scalar(std::ostream &stream, const T &arg, boost::mpl::bool_<true>) {
	// FIXME
	stream << "bin(";
	GnuplotEntry<T>::send(stream, arg);
	stream << ")";
	//stream->write(reinterpret_cast<const char *>(&val), sizeof(T));
}

template <typename T>
void send_scalar(std::ostream &stream, const T &arg, boost::mpl::bool_<false>) {
	GnuplotEntry<T>::send(stream, arg);
}

template <typename T, typename DoBinary>
typename boost::disable_if_c<T::is_container>::type
deref_and_print(std::ostream &stream, const T &arg, DoBinary) {
	const typename T::value_type &v = arg.deref();
	send_scalar(stream, v, DoBinary());
}

template <typename T, typename DoBinary>
typename boost::enable_if_c<T::is_container>::type
deref_and_print(std::ostream &stream, const T &arg, DoBinary) {
	stream << "{";
	typename T::subiter_type subrange = arg.deref_subiter();
	bool first = true;
	while(!subrange.is_end()) {
		if(!first) stream << " ";
		first = false;
		deref_and_print(stream, subrange, DoBinary());
		subrange.inc();
	}
	stream << "}";
}

template <typename T, typename U, typename DoBinary>
void deref_and_print(std::ostream &stream, const PairOfRange<T, U> &arg, DoBinary) {
	deref_and_print(stream, arg.l, DoBinary());
	stream << " ";
	deref_and_print(stream, arg.r, DoBinary());
}

template <typename T, typename DoBinary>
void deref_and_print(std::ostream &stream, const VecOfRange<T> &arg, DoBinary) {
	for(size_t i=0; i<arg.rvec.size(); i++) {
		if(i) stream << " ";
		deref_and_print(stream, arg.rvec[i], DoBinary());
	}
}

template <typename T, typename DoBinary>
typename boost::disable_if_c<T::is_container>::type
print_block(std::ostream &stream, T &arg, DoBinary) {
	while(!arg.is_end()) {
		//print_entry(arg.deref());
		deref_and_print(stream, arg, DoBinary());
		stream << std::endl;
		arg.inc();
	}
}

template <typename T, typename DoBinary>
typename boost::enable_if_c<T::is_container>::type
print_block(std::ostream &stream, T &arg, DoBinary) {
	bool first = true;
	while(!arg.is_end()) {
		if(first) {
			first = false;
		} else {
			stream << std::endl;
		}
		stream << "<block>" << std::endl;
		typename T::subiter_type sub = arg.deref_subiter();
		print_block(stream, sub, DoBinary());
		arg.inc();
	}
}

// FIXME - limit the depth of descent maximum of 2D
// FIXME - do the right thing depending on val_is_tuple
template <typename T, typename DoBinary>
void send_array(std::ostream &stream, const T &arg, DoBinary) {
	typename ArrayTraits<T>::range_type range = ArrayTraits<T>::get_range(arg);
	print_block(stream, range, DoBinary());
}

template <typename T, typename DoBinary>
void send_array_cols(std::ostream &stream, const T &arg, DoBinary) {
	VecOfRange<typename ArrayTraits<T>::range_type::subiter_type> cols = get_columns_range(arg);
	print_block(stream, cols, DoBinary());
}

/// }}}2

/// }}}1

/// {{{1 Old array writer (FIXME - to be deprecated)

class GnuplotArrayWriterBase {
public:
	GnuplotArrayWriterBase() : stream(NULL) { }

	virtual ~GnuplotArrayWriterBase() { }

	template <class U>
	void sendEntry(const U &v) {
		GnuplotEntry<U>::send(*stream, v);
	}

	template <class U>
	void sendIter(U p, U last) {
		while(p != last) {
			sendEntry(*p);
			*stream << "\n";
			++p;
		}
	}

	std::ostream *stream;
};

// generic container supporting iterators
template <class T, class Enable = void>
class GnuplotArrayWriter : public GnuplotArrayWriterBase {
public:
	void send(const T &arr) {
		sendIter(arr.begin(), arr.end());
	}
};

// C style array
template <typename T, std::size_t N>
class GnuplotArrayWriter<T[N]> : public GnuplotArrayWriterBase {
public:
	void send(const T (&arr)[N]) {
		sendIter(arr, arr+N);
	}
};

// vector containing data points
template <class T>
class GnuplotArrayWriter<std::vector<T> > : public GnuplotArrayWriterBase {
public:
	void send(const std::vector<T> &arr) {
		sendIter(arr.begin(), arr.end());
	}

	std::string binfmt(const std::vector<T> &arr) {
		std::ostringstream tmp;
		tmp << " format='" << GnuplotEntry<T>::formatCode() << "'";
		tmp << " array=(" << arr.size() << ")";
		tmp << " ";
		return tmp.str();
	}

	void sendBinary(const std::vector<T> &arr) {
		stream->write(reinterpret_cast<const char *>(&arr[0]), arr.size() * sizeof(T));
	}
};

// vector of vectors containing data points
template <class T>
class GnuplotArrayWriter<std::vector<std::vector <T> > > : public GnuplotArrayWriterBase {
public:
	void send(const std::vector<std::vector <T> > &vectors) {
		// all vectors need to have the same size
		assert(vectors.size() > 0);
		for(size_t i=1; i<vectors.size(); i++) {
			assert(vectors[i].size() == vectors[i-1].size());
		}

		for(size_t i=0; i<vectors[0].size(); i++) {
			for(size_t j=0; j<vectors.size(); j++) {
				sendEntry(vectors[j][i]);
			}
			*stream << "\n";
		}
	}

	std::string binfmt(const std::vector<std::vector<T> > &arr) {
		assert(arr.size() > 0);
		std::ostringstream tmp;
		tmp << " format='";
		for(size_t i=0; i<arr.size(); i++) {
			tmp << GnuplotEntry<T>::formatCode();
		}
		tmp << "' array=(" << arr[0].size() << ")";
		tmp << " ";
		return tmp.str();
	}

	void sendBinary(const std::vector<std::vector <T> > &vectors) {
		// all vectors need to have the same size
		assert(vectors.size() > 0);
		for(size_t i=1; i<vectors.size(); i++) {
			assert(vectors[i].size() == vectors[i-1].size());
		}

		for(size_t i=0; i<vectors[0].size(); i++) {
			for(size_t j=0; j<vectors.size(); j++) {
				const T &val = vectors[j][i];
				stream->write(reinterpret_cast<const char *>(&val), sizeof(T));
			}
		}
	}
};

#ifdef GNUPLOT_ENABLE_BLITZ
// Note: T could be either a scalar or a blitz::TinyVector.
template <class T>
class GnuplotArrayWriter<blitz::Array<T, 1> > : public GnuplotArrayWriterBase {
public:
	void send(const blitz::Array<T, 1> &arr) {
		sendIter(arr.begin(), arr.end());
	}

	void sendBinary(const blitz::Array<T, 1> &arr) {
		stream->write(reinterpret_cast<const char *>(arr.data()), arr.size() * sizeof(T));
	}

	std::string binfmt(const blitz::Array<T, 1> &arr) {
		std::ostringstream tmp;
		tmp << " format='" << GnuplotEntry<T>::formatCode() << "'";
		tmp << " array=(" << arr.extent(0) << ")";
		tmp << " ";
		return tmp.str();
	}
};

// Note: T could be either a scalar or a blitz::TinyVector.
template <class T>
class GnuplotArrayWriter<blitz::Array<T, 2> > : public GnuplotArrayWriterBase {
public:
	void send(const blitz::Array<T, 2> &a) {
		for(int i=a.lbound(0); i<=a.ubound(0); i++) {
			for(int j=a.lbound(1); j<=a.ubound(1); j++) {
				sendEntry(a(i, j));
				*stream << "\n";
			}
			*stream << "\n"; // double blank between blocks
		}
	}

	void sendBinary(const blitz::Array<T, 2> &arr) {
		stream->write(reinterpret_cast<const char *>(arr.data()), arr.size() * sizeof(T));
	}

	std::string binfmt(const blitz::Array<T, 2> &arr) {
		std::ostringstream tmp;
		tmp << " format='" << GnuplotEntry<T>::formatCode() << "'";
		tmp << " array=(" << arr.extent(0) << "," << arr.extent(1) << ")";
		if(arr.isMajorRank(0)) tmp << " scan=yx"; // i.e. C-style ordering
		tmp << " ";
		return tmp.str();
	}
};
#endif // GNUPLOT_ENABLE_BLITZ

/// }}}1

/// {{{1 Main class

class Gnuplot : public boost::iostreams::stream<
	boost::iostreams::file_descriptor_sink>
{
public:
	explicit Gnuplot(const std::string &cmd = "gnuplot") :
		boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
			FILENO(pout = POPEN(cmd.c_str(), "w")),
			boost::iostreams::never_close_handle
		),
		pout(pout), // keeps '-Weff++' quiet
		is_pipe(true),
		feedback(NULL),
		tmp_files(),
		debug_messages(false)
	{
		*this << std::scientific << std::setprecision(18);  // refer <iomanip>
	}

	explicit Gnuplot(FILE *fh) :
		boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
			FILENO(pout = fh),
			boost::iostreams::never_close_handle
		),
		pout(pout), // keeps '-Weff++' quiet
		is_pipe(false),
		feedback(NULL),
		tmp_files(),
		debug_messages(false)
	{
		*this << std::scientific << std::setprecision(18);  // refer <iomanip>
	}

private:
	// noncopyable
	Gnuplot(const Gnuplot &);
	const Gnuplot& operator=(const Gnuplot &);

public:
	~Gnuplot() {
		if(debug_messages) {
			std::cerr << "ending gnuplot session" << std::endl;
		}

		// FIXME - boost's close method calls close() on the file descriptor, but
		// we need to use pclose instead.  For now, just skip calling boost's close
		// and use flush just in case.
		*this << std::flush;
		fflush(pout);
		// Wish boost had a pclose method...
		//close();

		if(is_pipe) {
			if(PCLOSE(pout)) {
				std::cerr << "pclose returned error" << std::endl;
			}
		} else {
			if(fclose(pout)) {
				std::cerr << "fclose returned error" << std::endl;
			}
		}

		if(feedback) delete(feedback);
	}

private:
	// FIXME - not ideal to have this here
	template <class U>
	void sendEntry(const U &v) {
		GnuplotEntry<U>::send(*this, v);
	}

	template <class T>
	GnuplotArrayWriter<T> make_array_writer(std::ostream *stream) {
		GnuplotArrayWriter<T> ret;
		ret.stream = stream;
		return ret;
	}

public:
	template <class T>
	Gnuplot &send(const T &arg) {
		send_array(*this, arg, boost::mpl::bool_<false>());
		*this << "e" << std::endl; // gnuplot's "end of array" token
		return *this;
	}

	// Iterator.  I wish I had named this sendIter, but I didn't.
	// Now I can't use the two argument send function for anything else.
	template <class T>
	Gnuplot &send(T p, T last) {
		while(p != last) {
			sendEntry(*p);
			*this << "\n";
			++p;
		}
		*this << "e" << std::endl; // gnuplot's "end of array" token
		return *this;
	}

	template <class T, class U>
	Gnuplot &send(T x, T x_last, U y, U y_last) {
		while(x != x_last && y != y_last) {
			sendEntry(*x);
			*this <<  " ";
			sendEntry(*y);
			*this << "\n";
			++x;
			++y;
		}
		// assert inputs same size
		assert(x==x_last && y==y_last);
		*this << "e" << std::endl; // gnuplot's "end of array" token
		return *this;
	}

	template <class T>
	Gnuplot &sendBinary(const T &arg) {
		make_array_writer<T>(this).sendBinary(arg);
		return *this;
	}

	template <class T>
	std::string binfmt(const T &) {
		// FIXME
		return "TODO";
		//return make_array_writer<T>(this).binfmt(arg);
	}

private:
	std::string make_tmpfile() {
#ifdef GNUPLOT_USE_TMPFILE
		boost::shared_ptr<GnuplotTmpfile> tmp_file(new GnuplotTmpfile());
		// The file will be removed once the pointer is removed from the
		// tmp_files container.
		tmp_files.push_back(tmp_file);
		return tmp_file->file.string();
#else
		throw(std::logic_error("no filename given and temporary files not enabled"));
#endif // GNUPLOT_USE_TMPFILE
	}

public:
	// NOTE: empty filename makes temporary file
	template <class T>
	std::string file(const T &arg, std::string filename="") {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out);
		send_array(tmp_stream, arg, boost::mpl::bool_<false>());
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' ";
		return cmdline.str();
	}

	// NOTE: empty filename makes temporary file
	template <class T>
	std::string binaryFile(const T &arg, std::string filename="") {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out | std::fstream::binary);
		send_array(tmp_stream, arg, boost::mpl::bool_<true>());
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' binary" << binfmt(arg);
		return cmdline.str();
	}

	void clearTmpfiles() {
		// destructors will cause deletion
		tmp_files.clear();
	}

#ifdef GNUPLOT_ENABLE_FEEDBACK
	// Input variables are set to the mouse position and button.  If the gnuplot
	// window is closed, button -1 is returned.  The msg parameter is the prompt
	// that is printed to the console.
	void getMouse(
		double &mx, double &my, int &mb,
		std::string msg="Click Mouse!"
	) {
		allocFeedback();

		*this << "set mouse" << std::endl;
		*this << "pause mouse \"" << msg << "\\n\"" << std::endl;
		*this << "if (exists(\"MOUSE_X\")) print MOUSE_X, MOUSE_Y, MOUSE_BUTTON; else print 0, 0, -1;" << std::endl;
		if(debug_messages) {
			std::cerr << "begin scanf" << std::endl;
		}
		if(3 != fscanf(feedback->handle(), "%50lf %50lf %50d", &mx, &my, &mb)) {
			throw std::runtime_error("could not parse reply");
		}
		if(debug_messages) {
			std::cerr << "end scanf" << std::endl;
		}
	}

	void allocFeedback() {
		if(!feedback) {
#ifdef GNUPLOT_ENABLE_PTY
			feedback = new GnuplotFeedbackPty(debug_messages);
//#elif defined GNUPLOT_USE_TMPFILE
//// Currently this doesn't work since fscanf doesn't block (need something like "tail -f")
//			feedback = new GnuplotFeedbackTmpfile(debug_messages);
#endif
			*this << "set print \"" << feedback->filename() << "\"" << std::endl;
		}
	}
#endif // GNUPLOT_ENABLE_FEEDBACK

private:
	FILE *pout;
	bool is_pipe;
	GnuplotFeedback *feedback;
#ifdef GNUPLOT_USE_TMPFILE
	std::vector<boost::shared_ptr<GnuplotTmpfile> > tmp_files;
#else
	// just a placeholder
	std::vector<int> tmp_files;
#endif // GNUPLOT_USE_TMPFILE

public:
	bool debug_messages;
};

/// }}}1

#endif // GNUPLOT_IOSTREAM_H
