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
		Verify that nested containers have consistent lengths between slices (at least along
		the column dimension, sometimes it might be okay for blocks to be different lengths).
		Support std::tuple and boost::tuple.
		Static asserts for non-containers or not enough depth.

	ChangeLog:
		send() for iterators has been removed
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
// FIXME - which of these are really needed?
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
// FIXME - what's the best way here?
#ifdef _WIN32
#include <windows.h>
#define PCLOSE _pclose
#define POPEN  _popen
#define FILENO _fileno
#else
#define PCLOSE pclose
#define POPEN  popen
#define FILENO fileno
#endif

/// }}}1

namespace gnuplotio {

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

/// {{{1 Traits and printers for entry datatypes

/// {{{2 Basic entry datatypes

template <class T>
void send_entry(std::ostream &stream, const T &v) {
	stream << v;
}

template <class T>
void send_entry_bin(std::ostream &stream, const T &v) {
	stream.write(reinterpret_cast<const char *>(&v), sizeof(T));
}

void format_code(std::ostream &stream, const    float &) { stream << "%float"; }
void format_code(std::ostream &stream, const   double &) { stream << "%double"; }
void format_code(std::ostream &stream, const   int8_t &) { stream << "%int8"; }
void format_code(std::ostream &stream, const  uint8_t &) { stream << "%uint8"; }
void format_code(std::ostream &stream, const  int16_t &) { stream << "%int16"; }
void format_code(std::ostream &stream, const uint16_t &) { stream << "%uint16"; }
void format_code(std::ostream &stream, const  int32_t &) { stream << "%int32"; }
void format_code(std::ostream &stream, const uint32_t &) { stream << "%uint32"; }
void format_code(std::ostream &stream, const  int64_t &) { stream << "%int64"; }
void format_code(std::ostream &stream, const uint64_t &) { stream << "%uint64"; }

template <class T, class U>
void format_code(std::ostream &stream, const std::pair<T, U> &v) {
	format_code(stream, v.first);
	format_code(stream, v.second);
}

template <class T, class U>
void send_entry(std::ostream &stream, const std::pair<T, U> &v) {
	send_entry(stream, v.first);
	stream << " ";
	send_entry(stream, v.second);
}

template <class T, class U>
void send_entry_bin(std::ostream &stream, const std::pair<T, U> &v) {
	send_entry_bin(stream, v.first);
	send_entry_bin(stream, v.second);
}

/// }}}2

/// {{{2 Blitz support

// FIXME - put this outside the main header guard, so that it can be picked up if this header
// is included a second time, after blitz header is loaded.  It will then require its own
// header guard.
#ifdef BZ_BLITZ_H
template <class T, int N>
void format_code(std::ostream &stream, const blitz::TinyVector<T, N> &v) {
	for(int i=0; i<N; i++) {
		format_code(stream, v[i]);
	}
}

template <class T, int N>
void send_entry(std::ostream &stream, const blitz::TinyVector<T, N> &v) {
	for(int i=0; i<N; i++) {
		if(i) stream << " ";
		stream << v[i];
	}
}
#endif // BZ_BLITZ_H

/// }}}2

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
struct WasNotContainer {
	// FIXME - this is just here to make VC++ happy.
	typedef void subiter_type;
};

// The unspecialized version of this class gives traits for things that are *not* arrays.
template <typename T, typename Enable=void>
class ArrayTraits {
public:
	typedef WasNotContainer value_type;
	typedef WasNotContainer range_type;
	static const bool is_container = false;
	static const bool allow_colwrap = false;
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

	static const bool is_container = true;
	static const bool allow_colwrap = true;
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
	template <typename T, typename U, typename PrintMode>
	friend void deref_and_print(std::ostream &, const PairOfRange<T, U> &, PrintMode);

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
	static const bool is_container = ArrayTraits<T>::is_container && ArrayTraits<U>::is_container;
	// Don't allow colwrap since it's already wrapped.
	static const bool allow_colwrap = false;
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
	template <typename T, typename PrintMode>
	friend void deref_and_print(std::ostream &, const VecOfRange<T> &, PrintMode);

public:
	VecOfRange() { }
	explicit VecOfRange(const std::vector<RT> &_rvec) : rvec(_rvec) { }

	static const bool is_container = RT::is_container;
	// Don't allow colwrap since it's already wrapped.
	static const bool allow_colwrap = false;

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

// FIXME - put this outside the main header guard, so that it can be picked up if this header
// is included a second time, after blitz header is loaded.  It will then require its own
// header guard.
#ifdef ARMA_INCLUDES
// FIXME - handle Row, Cube, Field

template <typename T>
class ArrayTraits<arma::Mat<T> > : public ArrayTraitsDefaults<T> {
	class ArmaMatRange {
	public:
		ArmaMatRange() : p(NULL), it(0) { }
		explicit ArmaMatRange(const arma::Mat<T> *_p) : p(_p), it(0) { }

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
	static const bool allow_colwrap = false;
	static const size_t depth = ArrayTraits<T>::depth + 2;

	typedef ArmaMatRange range_type;

	static range_type get_range(const arma::Mat<T> &arg) {
		//std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
		return range_type(&arg);
	}
};

template <typename T>
class ArrayTraits<arma::Col<T> > : public ArrayTraitsDefaults<T> {
public:
	static const bool allow_colwrap = false;

	typedef IteratorRange<typename arma::Col<T>::const_iterator, T> range_type;

	static range_type get_range(const arma::Col<T> &arg) {
		//std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
		return range_type(arg.begin(), arg.end());
	}
};
#endif // ARMA_INCLUDES

/// }}}2

/// {{{2 Blitz support

// FIXME - put this outside the main header guard, so that it can be picked up if this header
// is included a second time, after blitz header is loaded.  It will then require its own
// header guard.
#ifdef BZ_BLITZ_H
// FIXME - raise static error if possible
class WasBlitzPartialSlice { };

template <typename T, int ArrayDim, int SliceDim>
class BlitzIterator {
public:
	BlitzIterator() : p(NULL) { }
	BlitzIterator(
		const blitz::Array<T, ArrayDim> *_p,
		const blitz::TinyVector<int, ArrayDim> _idx
	) : p(_p), idx(_idx) { }

	typedef WasBlitzPartialSlice value_type;
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
	static const bool allow_colwrap = false;
	static const size_t depth = ArrayTraits<T>::depth + ArrayDim;

	typedef BlitzIterator<T, ArrayDim, ArrayDim> range_type;

	static range_type get_range(const blitz::Array<T, ArrayDim> &arg) {
		blitz::TinyVector<int, ArrayDim> start_idx;
		start_idx = 0;
		return range_type(&arg, start_idx);
	}
};
#endif // BZ_BLITZ_H

/// }}}2

/// {{{2 Array printing functions

static bool debug_array_print = 0;
void set_debug_array_print(bool v) { debug_array_print = v; }

struct ModeText   { static const bool is_text = 1; static const bool is_binfmt = 0; static const bool is_size = 0; };
struct ModeBinary { static const bool is_text = 0; static const bool is_binfmt = 0; static const bool is_size = 0; };
struct ModeBinfmt { static const bool is_text = 0; static const bool is_binfmt = 1; static const bool is_size = 0; };
struct ModeSize   { static const bool is_text = 0; static const bool is_binfmt = 0; static const bool is_size = 1; };

struct ColunwrapNo  { };
struct ColunwrapYes { };

struct Mode1D { };
struct Mode2D { };
struct Mode1DUnwrap { };
struct Mode2DUnwrap { };
struct ModeAuto { };

template <typename T>
size_t get_range_size(const T &arg) {
	// FIXME - not the fastest way.  Implement a size() method for range.
	size_t ret = 0;
	for(T i=arg; !i.is_end(); i.inc()) ++ret;
	return ret;
}

template <typename T>
void send_scalar(std::ostream &stream, const T &arg, ModeText) {
	send_entry(stream, arg);
}

template <typename T>
void send_scalar(std::ostream &stream, const T &arg, ModeBinary) {
	send_entry_bin(stream, arg);
}

template <typename T>
void send_scalar(std::ostream &stream, const T &arg, ModeBinfmt) {
	format_code(stream, arg);
}

template <typename T, typename PrintMode>
typename boost::disable_if_c<T::is_container>::type
deref_and_print(std::ostream &stream, const T &arg, PrintMode) {
	const typename T::value_type &v = arg.deref();
	send_scalar(stream, v, PrintMode());
}

template <typename T, typename PrintMode>
typename boost::enable_if_c<T::is_container>::type
deref_and_print(std::ostream &stream, const T &arg, PrintMode) {
	if(debug_array_print && PrintMode::is_text) stream << "{";
	typename T::subiter_type subrange = arg.deref_subiter();
	bool first = true;
	while(!subrange.is_end()) {
		if(!first && PrintMode::is_text) stream << " ";
		first = false;
		deref_and_print(stream, subrange, PrintMode());
		subrange.inc();
	}
	if(debug_array_print && PrintMode::is_text) stream << "}";
}

template <typename T, typename U, typename PrintMode>
void deref_and_print(std::ostream &stream, const PairOfRange<T, U> &arg, PrintMode) {
	deref_and_print(stream, arg.l, PrintMode());
	if(PrintMode::is_text) stream << " ";
	deref_and_print(stream, arg.r, PrintMode());
}

template <typename T, typename PrintMode>
void deref_and_print(std::ostream &stream, const VecOfRange<T> &arg, PrintMode) {
	for(size_t i=0; i<arg.rvec.size(); i++) {
		if(i && PrintMode::is_text) stream << " ";
		deref_and_print(stream, arg.rvec[i], PrintMode());
	}
}

template <size_t Depth, typename T, typename PrintMode>
typename boost::enable_if_c<(Depth==1) && !PrintMode::is_size>::type
print_block(std::ostream &stream, T &arg, PrintMode) {
	while(!arg.is_end()) {
		//print_entry(arg.deref());
		deref_and_print(stream, arg, PrintMode());
		if(PrintMode::is_binfmt) break;
		if(PrintMode::is_text) stream << std::endl;
		arg.inc();
	}
}

template <size_t Depth, typename T, typename PrintMode>
typename boost::enable_if_c<(Depth>1) && !PrintMode::is_size>::type
print_block(std::ostream &stream, T &arg, PrintMode) {
	bool first = true;
	while(!arg.is_end()) {
		if(first) {
			first = false;
		} else {
			if(PrintMode::is_text) stream << std::endl;
		}
		if(debug_array_print && PrintMode::is_text) stream << "<block>" << std::endl;
		typename T::subiter_type sub = arg.deref_subiter();
		print_block<Depth-1>(stream, sub, PrintMode());
		if(PrintMode::is_binfmt) break;
		arg.inc();
	}
}

template <size_t Depth, typename T, typename PrintMode>
typename boost::enable_if_c<(Depth==1) && PrintMode::is_size>::type
print_block(std::ostream &stream, T &arg, PrintMode) {
	stream << get_range_size(arg);
}

template <size_t Depth, typename T, typename PrintMode>
typename boost::enable_if_c<(Depth>1) && PrintMode::is_size>::type
print_block(std::ostream &stream, T &arg, PrintMode) {
	stream << get_range_size(arg) << ",";
	typename T::subiter_type sub = arg.deref_subiter();
	print_block<Depth-1>(stream, sub, PrintMode());
}

template <size_t Depth, typename T, typename PrintMode>
void generic_sender_level2(std::ostream &stream, T &arg, PrintMode) {
	print_block<Depth>(stream, arg, PrintMode());
}

template <size_t Depth, typename T, typename PrintMode>
void generic_sender_level1(std::ostream &stream, const T &arg, ColunwrapNo, PrintMode) {
	typename ArrayTraits<T>::range_type range = ArrayTraits<T>::get_range(arg);
	generic_sender_level2<Depth>(stream, range, PrintMode());
}

template <size_t Depth, typename T, typename PrintMode>
void generic_sender_level1(std::ostream &stream, const T &arg, ColunwrapYes, PrintMode) {
	VecOfRange<typename ArrayTraits<T>::range_type::subiter_type> cols = get_columns_range(arg);
	generic_sender_level2<Depth>(stream, cols, PrintMode());
}

template <typename T, typename PrintMode>
void generic_sender_level0(std::ostream &stream, const T &arg, Mode1D, PrintMode) {
	generic_sender_level1<1>(stream, arg, ColunwrapNo(), PrintMode());
}

template <typename T, typename PrintMode>
void generic_sender_level0(std::ostream &stream, const T &arg, Mode2D, PrintMode) {
	generic_sender_level1<2>(stream, arg, ColunwrapNo(), PrintMode());
}

template <typename T, typename PrintMode>
void generic_sender_level0(std::ostream &stream, const T &arg, Mode1DUnwrap, PrintMode) {
	generic_sender_level1<1>(stream, arg, ColunwrapYes(), PrintMode());
}

template <typename T, typename PrintMode>
void generic_sender_level0(std::ostream &stream, const T &arg, Mode2DUnwrap, PrintMode) {
	generic_sender_level1<2>(stream, arg, ColunwrapYes(), PrintMode());
}

template <typename T, typename PrintMode>
typename boost::enable_if_c<
	(ArrayTraits<T>::depth == 1)
>::type
generic_sender_level0(std::ostream &stream, const T &arg, ModeAuto, PrintMode) {
	//send_array_nocolwrap<1>(stream, arg, PrintMode());
	generic_sender_level0(stream, arg, Mode1D(), PrintMode());
}

template <typename T, typename PrintMode>
typename boost::enable_if_c<
	(ArrayTraits<T>::depth == 2) &&
	!ArrayTraits<T>::allow_colwrap
>::type
generic_sender_level0(std::ostream &stream, const T &arg, ModeAuto, PrintMode) {
	//send_array_nocolwrap<2>(stream, arg, PrintMode());
	generic_sender_level0(stream, arg, Mode2D(), PrintMode());
}

template <typename T, typename PrintMode>
typename boost::enable_if_c<
	(ArrayTraits<T>::depth == 2) &&
	ArrayTraits<T>::allow_colwrap
>::type
generic_sender_level0(std::ostream &stream, const T &arg, ModeAuto, PrintMode) {
	//send_array_colwrap<1>(stream, arg, PrintMode());
	generic_sender_level0(stream, arg, Mode1DUnwrap(), PrintMode());
}

template <typename T, typename PrintMode>
typename boost::enable_if_c<
	(ArrayTraits<T>::depth > 2) &&
	ArrayTraits<T>::allow_colwrap
>::type
generic_sender_level0(std::ostream &stream, const T &arg, ModeAuto, PrintMode) {
	//send_array_colwrap<2>(stream, arg, PrintMode());
	generic_sender_level0(stream, arg, Mode2DUnwrap(), PrintMode());
}

template <typename T, typename PrintMode>
typename boost::enable_if_c<
	(ArrayTraits<T>::depth > 2) &&
	!ArrayTraits<T>::allow_colwrap
>::type
generic_sender_level0(std::ostream &stream, const T &arg, ModeAuto, PrintMode) {
	//send_array_nocolwrap<2>(stream, arg, PrintMode());
	generic_sender_level0(stream, arg, Mode2D(), PrintMode());
}

/// }}}2

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

	void clearTmpfiles() {
		// destructors will cause deletion
		tmp_files.clear();
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
	template <typename T, typename ArrayMode>
	Gnuplot &send(const T &arg, ArrayMode) {
		generic_sender_level0(*this, arg, ArrayMode(), ModeText());
		*this << "e" << std::endl; // gnuplot's "end of array" token
		return *this;
	}

	template <class T, typename ArrayMode>
	Gnuplot &sendBinary(const T &arg, ArrayMode) {
		generic_sender_level0(*this, arg, ModeAuto(), ModeBinary());
		return *this;
	}

	template <class T, typename ArrayMode>
	std::string binfmt(const T &arg, const std::string &arr_or_rec, ArrayMode) {
		std::ostringstream tmp;
		tmp << " format='";
		generic_sender_level0(tmp, arg, ModeAuto(), ModeBinfmt());
		tmp << "' " << arr_or_rec << "=(";
		generic_sender_level0(tmp, arg, ModeAuto(), ModeSize());
		tmp << ")";
		tmp << " ";
		return tmp.str();
	}

public:
	// NOTE: empty filename makes temporary file
	template <class T, typename ArrayMode>
	std::string file(const T &arg, std::string filename, ArrayMode) {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out);
		generic_sender_level0(tmp_stream, arg, ModeAuto(), ModeText());
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' ";
		return cmdline.str();
	}

	// NOTE: empty filename makes temporary file
	template <class T, typename ArrayMode>
	std::string binaryFile(const T &arg, std::string filename, const std::string &arr_or_rec, ArrayMode) {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out | std::fstream::binary);
		generic_sender_level0(tmp_stream, arg, ModeAuto(), ModeBinary());
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' binary" << binfmt(arg, arr_or_rec, ArrayMode());
		return cmdline.str();
	}

	// FIXME - maybe this is better than the below
	template <typename ArrayMode, typename T>
	Gnuplot &foobar(const T &arg) { return send(arg, ArrayMode()); }
	template <typename T>
	Gnuplot &foobar(const T &arg) { return send(arg, ModeAuto()); }

	// FIXME - there's gotta be a better way...
	template <class T> Gnuplot &send         (const T &arg) { return send(arg, ModeAuto    ()); }
	template <class T> Gnuplot &send1d       (const T &arg) { return send(arg, Mode1D      ()); }
	template <class T> Gnuplot &send2d       (const T &arg) { return send(arg, Mode2D      ()); }
	template <class T> Gnuplot &send1d_unwrap(const T &arg) { return send(arg, Mode1DUnwrap()); }
	template <class T> Gnuplot &send2d_unwrap(const T &arg) { return send(arg, Mode2DUnwrap()); }

	template <class T> Gnuplot &sendBinary         (const T &arg) { return sendBinary(arg, ModeAuto    ()); }
	template <class T> Gnuplot &sendBinary1d       (const T &arg) { return sendBinary(arg, Mode1D      ()); }
	template <class T> Gnuplot &sendBinary2d       (const T &arg) { return sendBinary(arg, Mode2D      ()); }
	template <class T> Gnuplot &sendBinary1d_unwrap(const T &arg) { return sendBinary(arg, Mode1DUnwrap()); }
	template <class T> Gnuplot &sendBinary2d_unwrap(const T &arg) { return sendBinary(arg, Mode2DUnwrap()); }

	template <class T> std::string binfmt         (const T &arg, const std::string &arr_or_rec="array") { return binfmt(arg, arr_or_rec,  ModeAuto    ()); }
	template <class T> std::string binfmt1d       (const T &arg, const std::string &arr_or_rec="array") { return binfmt(arg, arr_or_rec,  Mode1D      ()); }
	template <class T> std::string binfmt2d       (const T &arg, const std::string &arr_or_rec="array") { return binfmt(arg, arr_or_rec,  Mode2D      ()); }
	template <class T> std::string binfmt1d_unwrap(const T &arg, const std::string &arr_or_rec="array") { return binfmt(arg, arr_or_rec,  Mode1DUnwrap()); }
	template <class T> std::string binfmt2d_unwrap(const T &arg, const std::string &arr_or_rec="array") { return binfmt(arg, arr_or_rec,  Mode2DUnwrap()); }

	template <class T> std::string file         (const T &arg, const std::string &filename="") { return file(arg, filename, ModeAuto    ()); }
	template <class T> std::string file1d       (const T &arg, const std::string &filename="") { return file(arg, filename, Mode1D      ()); }
	template <class T> std::string file2d       (const T &arg, const std::string &filename="") { return file(arg, filename, Mode2D      ()); }
	template <class T> std::string file1d_unwrap(const T &arg, const std::string &filename="") { return file(arg, filename, Mode1DUnwrap()); }
	template <class T> std::string file2d_unwrap(const T &arg, const std::string &filename="") { return file(arg, filename, Mode2DUnwrap()); }

	template <class T> std::string binaryFile         (const T &arg, const std::string &filename="", const std::string &arr_or_rec="array") { return binaryFile(arg, filename, arr_or_rec,  ModeAuto    ()); }
	template <class T> std::string binaryFile1d       (const T &arg, const std::string &filename="", const std::string &arr_or_rec="array") { return binaryFile(arg, filename, arr_or_rec,  Mode1D      ()); }
	template <class T> std::string binaryFile2d       (const T &arg, const std::string &filename="", const std::string &arr_or_rec="array") { return binaryFile(arg, filename, arr_or_rec,  Mode2D      ()); }
	template <class T> std::string binaryFile1d_unwrap(const T &arg, const std::string &filename="", const std::string &arr_or_rec="array") { return binaryFile(arg, filename, arr_or_rec,  Mode1DUnwrap()); }
	template <class T> std::string binaryFile2d_unwrap(const T &arg, const std::string &filename="", const std::string &arr_or_rec="array") { return binaryFile(arg, filename, arr_or_rec,  Mode2DUnwrap()); }

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

} // namespace gnuplotio

// The first version of this library didn't use namespaces, and now this must be here forever
// for reverse compatibility:
using gnuplotio::Gnuplot;

#endif // GNUPLOT_IOSTREAM_H
