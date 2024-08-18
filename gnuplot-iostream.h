// vim:foldmethod=marker

/*
Copyright (c) 2020 Daniel Stahlke (dan@stahlke.org)

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

/* A C++ interface to gnuplot.
 * Web page: http://www.stahlke.org/dan/gnuplot-iostream
 * Documentation: https://github.com/dstahlke/gnuplot-iostream/wiki
 *
 * The whole library consists of this monolithic header file, for ease of installation (the
 * Makefile and *.cc files are only for examples and tests).
 *
 * TODO:
 *     Callbacks via gnuplot's 'bind' function.  This would allow triggering user functions when
 *     keys are pressed in the gnuplot window.  However, it would require a PTY reader thread.
 *     Maybe temporary files read in a thread can replace PTY stuff.
 */

#ifndef GNUPLOT_IOSTREAM_H
#define GNUPLOT_IOSTREAM_H

// {{{1 Includes and defines

#define GNUPLOT_IOSTREAM_VERSION 3

// C system includes
#include <cstdio>
#ifdef GNUPLOT_ENABLE_PTY
#    include <termios.h>
#    include <unistd.h>
#ifdef __APPLE__
#    include <util.h>
#else
#    include <pty.h>
#endif
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
#include <complex>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <tuple>
#include <type_traits>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/version.hpp>
#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>
// This is the version of boost which has v3 of the filesystem libraries by default.
#if BOOST_VERSION >= 104600
#    define GNUPLOT_USE_TMPFILE
#    include <boost/filesystem.hpp>
#endif // BOOST_VERSION

// Note: this is here for reverse compatibility.  The new way to enable blitz support is to
// just include the gnuplot-iostream.h header after you include the blitz header (likewise for
// armadillo).
#ifdef GNUPLOT_ENABLE_BLITZ
#    include <blitz/array.h>
#endif

// If this is defined, warn about use of deprecated functions.
#ifdef GNUPLOT_DEPRECATE_WARN
#    ifdef __GNUC__
#        define GNUPLOT_DEPRECATE(msg) __attribute__ ((deprecated(msg)))
#    elif defined(_MSC_VER)
#        define GNUPLOT_DEPRECATE(msg) __declspec(deprecated(msg))
#    else
#        define GNUPLOT_DEPRECATE(msg)
#    endif
#else
#    define GNUPLOT_DEPRECATE(msg)
#endif

// Patch for Windows by Damien Loison
#ifdef _WIN32
#    define GNUPLOT_PCLOSE _pclose
#    define GNUPLOT_POPEN  _popen
#    define GNUPLOT_FILENO _fileno
#else
#    define GNUPLOT_PCLOSE pclose
#    define GNUPLOT_POPEN  popen
#    define GNUPLOT_FILENO fileno
#endif

// MSVC gives a warning saying that fopen and getenv are not secure.  But they are secure.
// Unfortunately their replacement functions are not simple drop-in replacements.  The best
// solution is to just temporarily disable this warning whenever fopen or getenv is used.
// http://stackoverflow.com/a/4805353/1048959
#if defined(_MSC_VER) && _MSC_VER >= 1400
#    define GNUPLOT_MSVC_WARNING_4996_PUSH \
        __pragma(warning(push)) \
        __pragma(warning(disable:4996))
#    define GNUPLOT_MSVC_WARNING_4996_POP \
        __pragma(warning(pop))
#else
#    define GNUPLOT_MSVC_WARNING_4996_PUSH
#    define GNUPLOT_MSVC_WARNING_4996_POP
#endif

#ifndef GNUPLOT_DEFAULT_COMMAND
#ifdef _WIN32
// "pgnuplot" is considered deprecated according to the Internet.  It may be faster.  It
// doesn't seem to handle binary data though.
//#    define GNUPLOT_DEFAULT_COMMAND "pgnuplot -persist"
// The default install path for gnuplot is written here.  This way the user doesn't have to add
// anything to their %PATH% environment variable.
#    define GNUPLOT_DEFAULT_COMMAND "\"C:\\Program Files\\gnuplot\\bin\\gnuplot.exe\" -persist"
#else
#    define GNUPLOT_DEFAULT_COMMAND "gnuplot -persist"
#endif
#endif

// }}}1

namespace gnuplotio {

// {{{1 Basic traits helpers
//
// The mechanisms constructed in this section enable us to detect what sort of datatype has
// been passed to a function.

// This can be specialized as needed, in order to not use the STL interfaces for specific
// classes.
template <typename T, typename=void>
static constexpr bool dont_treat_as_stl_container = false;


template <typename T, typename=void>
static constexpr bool is_like_stl_container = false;

template <typename T>
static constexpr bool is_like_stl_container<T, std::void_t<
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end()),
        typename T::value_type
    >> = !dont_treat_as_stl_container<T>;

static_assert( is_like_stl_container<std::vector<int>>);
static_assert(!is_like_stl_container<int>);


template <typename T, typename=void>
static constexpr bool is_like_stl_container2 = false;

template <typename T>
static constexpr bool is_like_stl_container2<T, std::void_t<
        decltype(begin(std::declval<T>())),
        decltype(end  (std::declval<T>()))
    >> = !is_like_stl_container<T> && !dont_treat_as_stl_container<T>;


template <typename T>
static constexpr bool is_boost_tuple_nulltype =
    std::is_same_v<T, boost::tuples::null_type>;

static_assert(is_boost_tuple_nulltype<boost::tuples::null_type>);

template <typename T, typename=void>
static constexpr bool is_boost_tuple = false;

template <typename T>
static constexpr bool is_boost_tuple<T, std::void_t<
        typename T::head_type,
        typename T::tail_type
    >> = is_boost_tuple<typename T::tail_type> || is_boost_tuple_nulltype<typename T::tail_type>;

static_assert( is_boost_tuple<boost::tuple<int>>);
static_assert( is_boost_tuple<boost::tuple<int, int>>);
static_assert(!is_boost_tuple<std::tuple<int>>);
static_assert(!is_boost_tuple<std::tuple<int, int>>);

// }}}1

// {{{1 Tmpfile helper class
#ifdef GNUPLOT_USE_TMPFILE
// RAII temporary file.  File is removed when this object goes out of scope.
class GnuplotTmpfile {
public:
    explicit GnuplotTmpfile(bool _debug_messages) :
        file(boost::filesystem::unique_path(
            boost::filesystem::temp_directory_path() /
            "tmp-gnuplot-%%%%-%%%%-%%%%-%%%%")),
        debug_messages(_debug_messages)
    {
        if(debug_messages) {
            std::cerr << "create tmpfile " << file << std::endl;
        }
    }

private:
    // noncopyable
    GnuplotTmpfile(const GnuplotTmpfile &);
    const GnuplotTmpfile& operator=(const GnuplotTmpfile &);

public:
    ~GnuplotTmpfile() {
        if(debug_messages) {
            std::cerr << "delete tmpfile " << file << std::endl;
        }
        // it is never good to throw exceptions from a destructor
        try {
            remove(file);
        } catch(const std::exception &) {
            std::cerr << "Failed to remove temporary file " << file << std::endl;
        }
    }

public:
    boost::filesystem::path file;
    bool debug_messages;
};

class GnuplotTmpfileCollection {
public:
    std::string make_tmpfile() {
        const bool debug_messages = false;
        std::shared_ptr<GnuplotTmpfile> tmp_file(new GnuplotTmpfile(debug_messages));
        // The file will be removed once the pointer is removed from the
        // tmp_files container.
        tmp_files.push_back(tmp_file);
        return tmp_file->file.string();
    }

    void clear() {
        tmp_files.clear();
    }

private:
    std::vector<std::shared_ptr<GnuplotTmpfile>> tmp_files;
};
#else // GNUPLOT_USE_TMPFILE
class GnuplotTmpfileCollection {
public:
    std::string make_tmpfile() {
        throw std::logic_error("no filename given and temporary files not enabled");
    }

    void clear() { }
};
#endif // GNUPLOT_USE_TMPFILE
// }}}1

// {{{1 Feedback helper classes
//
// Used for reading stuff sent from gnuplot via gnuplot's "print" function.
//
// For example, this is used for capturing mouse clicks in the gnuplot window.  There are two
// implementations, only the first of which is complete.  The first implementation allocates a
// PTY (pseudo terminal) which is written to by gnuplot and read by us.  This only works in
// Linux.  The second implementation creates a temporary file which is written to by gnuplot
// and read by us.  However, this doesn't currently work since fscanf doesn't block.  It would
// be possible to get this working using a more complicated mechanism (select or threads) but I
// haven't had the need for it.

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
        pty_fh(nullptr),
        master_fd(-1),
        slave_fd(-1)
    {
    // adapted from http://www.gnuplot.info/files/gpReadMouseTest.c
        if(0 > openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr)) {
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
//    explicit GnuplotFeedbackTmpfile(bool debug_messages) :
//        tmp_file(),
//        fh(nullptr)
//    {
//        if(debug_messages) {
//            std::cerr << "feedback_fn=" << filename() << std::endl;
//        }
//        GNUPLOT_MSVC_WARNING_4996_PUSH
//        fh = std::fopen(filename().c_str(), "a");
//        GNUPLOT_MSVC_WARNING_4996_POP
//    }
//
//    ~GnuplotFeedbackTmpfile() {
//        fclose(fh);
//    }
//
//private:
//    // noncopyable
//    GnuplotFeedbackTmpfile(const GnuplotFeedbackTmpfile &);
//    const GnuplotFeedbackTmpfile& operator=(const GnuplotFeedbackTmpfile &);
//
//public:
//    std::string filename() const {
//        return tmp_file.file.string();
//    }
//
//    FILE *handle() const {
//        return fh;
//    }
//
//private:
//    GnuplotTmpfile tmp_file;
//    FILE *fh;
//};
#endif // GNUPLOT_ENABLE_PTY, GNUPLOT_USE_TMPFILE
// }}}1

// {{{1 Traits and printers for entry datatypes
//
// This section contains the mechanisms for sending scalar and tuple data to gnuplot.  Pairs
// and tuples are sent by appealing to the senders defined for their component scalar types.
// Senders for arrays are defined in a later section.
//
// There are three classes which need to be specialized for each supported datatype:
// 1. TextSender to send data as text.  The default is to just send using the ostream's `<<`
// operator.
// 2. BinarySender to send data as binary, in a format which gnuplot can understand.  There is
// no default implementation (unimplemented types raise a compile time error), however
// inheriting from FlatBinarySender will send the data literally as it is stored in memory.
// This suffices for most of the standard built-in types (e.g. uint32_t or double).
// 3. BinfmtSender sends a description of the data format to gnuplot (e.g. `%uint32`).  Type
// `show datafile binary datasizes` in gnuplot to see a list of supported formats.

// {{{2 Basic entry datatypes

// Default TextSender, sends data using `<<` operator.
template <typename T, typename Enable=void>
struct TextSender {
    static void send(std::ostream &stream, const T &v) {
        stream << v;
    }
};

class BinarySenderNotImplemented : public std::logic_error {
public:
    explicit BinarySenderNotImplemented(const std::string &w) : std::logic_error(w) { }
};

// Default BinarySender, raises a compile time error.
template <typename T, typename Enable=void>
struct BinarySender {
    static void send(std::ostream &, const T &) {
        throw BinarySenderNotImplemented("BinarySender not implemented for this type");
    }
};

// This is a BinarySender implementation that just sends directly from memory.  Data types
// which can be sent this way can have their BinarySender specialization inherit from this.
template <typename T>
struct FlatBinarySender {
    static void send(std::ostream &stream, const T &v) {
        stream.write(reinterpret_cast<const char *>(&v), sizeof(T));
    }
};

// Default BinfmtSender, raises a compile time error.
template <typename T, typename Enable=void>
struct BinfmtSender {
    static void send(std::ostream &) {
        throw BinarySenderNotImplemented("BinfmtSender not implemented for this type");
    }
};

// BinfmtSender implementations for basic data types supported by gnuplot.
template<> struct BinfmtSender< float> { static void send(std::ostream &stream) { stream << "%float";  } };
template<> struct BinfmtSender<double> { static void send(std::ostream &stream) { stream << "%double"; } };
template<> struct BinfmtSender<  int8_t> { static void send(std::ostream &stream) { stream << "%int8";   } };
template<> struct BinfmtSender< uint8_t> { static void send(std::ostream &stream) { stream << "%uint8";  } };
template<> struct BinfmtSender< int16_t> { static void send(std::ostream &stream) { stream << "%int16";  } };
template<> struct BinfmtSender<uint16_t> { static void send(std::ostream &stream) { stream << "%uint16"; } };
template<> struct BinfmtSender< int32_t> { static void send(std::ostream &stream) { stream << "%int32";  } };
template<> struct BinfmtSender<uint32_t> { static void send(std::ostream &stream) { stream << "%uint32"; } };
template<> struct BinfmtSender< int64_t> { static void send(std::ostream &stream) { stream << "%int64";  } };
template<> struct BinfmtSender<uint64_t> { static void send(std::ostream &stream) { stream << "%uint64"; } };

// BinarySender implementations for basic data types supported by gnuplot.  These types can
// just be sent as stored in memory, so all these senders inherit from FlatBinarySender.
template<> struct BinarySender< float> : public FlatBinarySender< float> { };
template<> struct BinarySender<double> : public FlatBinarySender<double> { };
template<> struct BinarySender<  int8_t> : public FlatBinarySender<  int8_t> { };
template<> struct BinarySender< uint8_t> : public FlatBinarySender< uint8_t> { };
template<> struct BinarySender< int16_t> : public FlatBinarySender< int16_t> { };
template<> struct BinarySender<uint16_t> : public FlatBinarySender<uint16_t> { };
template<> struct BinarySender< int32_t> : public FlatBinarySender< int32_t> { };
template<> struct BinarySender<uint32_t> : public FlatBinarySender<uint32_t> { };
template<> struct BinarySender< int64_t> : public FlatBinarySender< int64_t> { };
template<> struct BinarySender<uint64_t> : public FlatBinarySender<uint64_t> { };

// Make char types print as integers, not as characters.
template <typename T>
struct CastIntTextSender {
    static void send(std::ostream &stream, const T &v) {
        stream << static_cast<int>(v);
    }
};
template<> struct TextSender<          char> : public CastIntTextSender<          char> { };
template<> struct TextSender<   signed char> : public CastIntTextSender<   signed char> { };
template<> struct TextSender< unsigned char> : public CastIntTextSender< unsigned char> { };

// Make sure that the same not-a-number string is printed on all platforms.
template <typename T>
struct FloatTextSender {
    static void send(std::ostream &stream, const T &v) {
        if(std::isnan(v)) { stream << "nan"; } else { stream << v; }
    }
};
template<> struct TextSender<      float> : FloatTextSender<      float> { };
template<> struct TextSender<     double> : FloatTextSender<     double> { };
template<> struct TextSender<long double> : FloatTextSender<long double> { };

// }}}2

// {{{2 std::pair support

template <typename T, typename U>
struct TextSender<std::pair<T, U>> {
    static void send(std::ostream &stream, const std::pair<T, U> &v) {
        TextSender<T>::send(stream, v.first);
        stream << " ";
        TextSender<U>::send(stream, v.second);
    }
};

template <typename T, typename U>
struct BinfmtSender<std::pair<T, U>> {
    static void send(std::ostream &stream) {
        BinfmtSender<T>::send(stream);
        BinfmtSender<U>::send(stream);
    }
};

template <typename T, typename U>
struct BinarySender<std::pair<T, U>> {
    static void send(std::ostream &stream, const std::pair<T, U> &v) {
        BinarySender<T>::send(stream, v.first);
        BinarySender<U>::send(stream, v.second);
    }
};

// }}}2

// {{{2 std::complex support

template <typename T>
struct TextSender<std::complex<T>> {
    static void send(std::ostream &stream, const std::complex<T> &v) {
        TextSender<T>::send(stream, v.real());
        stream << " ";
        TextSender<T>::send(stream, v.imag());
    }
};

template <typename T>
struct BinfmtSender<std::complex<T>> {
    static void send(std::ostream &stream) {
        BinfmtSender<T>::send(stream);
        BinfmtSender<T>::send(stream);
    }
};

template <typename T>
struct BinarySender<std::complex<T>> {
    static void send(std::ostream &stream, const std::complex<T> &v) {
        BinarySender<T>::send(stream, v.real());
        BinarySender<T>::send(stream, v.imag());
    }
};

// }}}2

// {{{2 boost::tuple support

template <typename T>
struct TextSender<T,
    typename std::enable_if_t<is_boost_tuple<T>>
> {
    static void send(std::ostream &stream, const T &v) {
        TextSender<typename T::head_type>::send(stream, v.get_head());
        if constexpr (!is_boost_tuple_nulltype<typename T::tail_type>) {
            stream << " ";
            TextSender<typename T::tail_type>::send(stream, v.get_tail());
        }
    }
};

template <typename T>
struct BinfmtSender<T,
    typename std::enable_if_t<is_boost_tuple<T>>
> {
    static void send(std::ostream &stream) {
        BinfmtSender<typename T::head_type>::send(stream);
        if constexpr (!is_boost_tuple_nulltype<typename T::tail_type>) {
            stream << " ";
            BinfmtSender<typename T::tail_type>::send(stream);
        }
    }
};

template <typename T>
struct BinarySender<T,
    typename std::enable_if_t<is_boost_tuple<T>>
> {
    static void send(std::ostream &stream, const T &v) {
        BinarySender<typename T::head_type>::send(stream, v.get_head());
        if constexpr (!is_boost_tuple_nulltype<typename T::tail_type>) {
            BinarySender<typename T::tail_type>::send(stream, v.get_tail());
        }
    }
};

// }}}2

// {{{2 std::tuple support

// http://stackoverflow.com/questions/6245735/pretty-print-stdtuple

template<std::size_t> struct int_{}; // compile-time counter

template <typename Tuple, std::size_t I>
void std_tuple_formatcode_helper(std::ostream &stream, const Tuple *, int_<I>) {
    std_tuple_formatcode_helper(stream, (const Tuple *)(0), int_<I-1>());
    stream << " ";
    BinfmtSender<typename std::tuple_element<I, Tuple>::type>::send(stream);
}

template <typename Tuple>
void std_tuple_formatcode_helper(std::ostream &stream, const Tuple *, int_<0>) {
    BinfmtSender<typename std::tuple_element<0, Tuple>::type>::send(stream);
}

template <typename... Args>
struct BinfmtSender<std::tuple<Args...>> {
    typedef typename std::tuple<Args...> Tuple;

    static void send(std::ostream &stream) {
        std_tuple_formatcode_helper(stream, (const Tuple *)(0), int_<sizeof...(Args)-1>());
    }
};

template <typename Tuple, std::size_t I>
void std_tuple_textsend_helper(std::ostream &stream, const Tuple &v, int_<I>) {
    std_tuple_textsend_helper(stream, v, int_<I-1>());
    stream << " ";
    TextSender<typename std::tuple_element<I, Tuple>::type>::send(stream, std::get<I>(v));
}

template <typename Tuple>
void std_tuple_textsend_helper(std::ostream &stream, const Tuple &v, int_<0>) {
    TextSender<typename std::tuple_element<0, Tuple>::type>::send(stream, std::get<0>(v));
}

template <typename... Args>
struct TextSender<std::tuple<Args...>> {
    typedef typename std::tuple<Args...> Tuple;

    static void send(std::ostream &stream, const Tuple &v) {
        std_tuple_textsend_helper(stream, v, int_<sizeof...(Args)-1>());
    }
};

template <typename Tuple, std::size_t I>
void std_tuple_binsend_helper(std::ostream &stream, const Tuple &v, int_<I>) {
    std_tuple_binsend_helper(stream, v, int_<I-1>());
    BinarySender<typename std::tuple_element<I, Tuple>::type>::send(stream, std::get<I>(v));
}

template <typename Tuple>
void std_tuple_binsend_helper(std::ostream &stream, const Tuple &v, int_<0>) {
    BinarySender<typename std::tuple_element<0, Tuple>::type>::send(stream, std::get<0>(v));
}

template <typename... Args>
struct BinarySender<std::tuple<Args...>> {
    typedef typename std::tuple<Args...> Tuple;

    static void send(std::ostream &stream, const Tuple &v) {
        std_tuple_binsend_helper(stream, v, int_<sizeof...(Args)-1>());
    }
};

// }}}2

// }}}1

// {{{1 ArrayTraits and Range classes
//
// This section handles sending of array data to gnuplot.  It is rather complicated because of
// the diversity of storage schemes supported.  For example, it treats a
// `std::pair<std::vector<T>, std::vector<U>>` in the same way as a
// `std::vector<std::pair<T, U>>`, iterating through the two arrays in lockstep, and sending
// pairs <T,U> to gnuplot as columns.  In fact, any nested combination of pairs, tuples, STL
// containers, Blitz arrays, and Armadillo arrays is supported (with the caveat that, for
// instance, Blitz arrays should never be put into an STL container or you will suffer
// unpredictable results due the way Blitz handles assignment).  Nested containers are
// considered to be multidimensional arrays.  Although gnuplot only supports 1D and 2D arrays,
// our module is in principle not limited.
//
// The ArrayTraits class is specialized for every supported array or container type (the
// default, unspecialized, version of ArrayTraits exists only to tell you that something is
// *not* a container, via the is_container flag).  ArrayTraits tells you the depth of a nested
// (or multidimensional) container, as well as the value type, and provides a specialized
// sort of iterator (a.k.a. "range").  Ranges are sort of like STL iterators, except that they
// have built-in knowledge of the end condition so you don't have to carry around both a
// begin() and an end() iterator like in STL.
//
// As an example of how this works, consider a std::pair of std::vectors.  Ultimately this gets
// sent to gnuplot as two columns, so the two vectors need to be iterated in lockstep.
// The `value_type` of `std::pair<std::vector<T>, std::vector<U>>` is then `std::pair<T, U>`
// and this is what deferencing the range (iterator) gives.  Internally, this range is built
// out of a pair of ranges (PairOfRange class), the `inc()` (advance to next element)
// method calls `inc()` on each of the children, and `deref()` calls `deref()` on each child
// and combines the results to return a `std::pair`.  Tuples are handled as nested pairs.
//
// In addition to PairOfRange, there is also a VecOfRange class that can be used to treat the
// outermost part of a nested container as if it were a tuple.  Since tuples are printed as
// columns, this is like treating a multidimensional array as if it were column-major.  A
// VecOfRange is obtained by calling `get_columns_range`.  This is used by, for instance,
// `send1d_colmajor`.  The implementation is similar to that of PairOfRange.
//
// The range, accessed via `ArrayTraits<T>::get_range`, will be of a different class depending
// on T, and this is defined by the ArrayTraits specialization for T.  It will always have
// methods `inc()` to advance to the next element and `is_end()` for checking whether one has
// advanced past the final element.  For nested containers, `deref_subiter()` returns a range
// iterator for the next nesting level.  When at the innermost level of nesting, `deref()`
// returns the value of the entry the iterator points to (a scalar, pair, or tuple).
// Only one of `deref()` or `deref_subiter()` will be available, depending on whether there are
// deeper levels of nesting.  The typedefs `value_type` and `subiter_type` tell the return
// types of these two methods.
//
// Support for standard C++ and boost containers and tuples of containers is provided in this
// section.  Support for third party packages like Blitz and Armadillo is in a later section.

// {{{2 ArrayTraits generic class and defaults

// Error messages involving this stem from treating something that was not a container as if it
// was.  This is only here to allow compiliation without errors in normal cases.
struct Error_WasNotContainer {
    // This is just here to make VC++ happy.
    // https://connect.microsoft.com/VisualStudio/feedback/details/777612/class-template-specialization-that-compiles-in-g-but-not-visual-c
    typedef void subiter_type;
};

// The unspecialized version of this class gives traits for things that are *not* arrays.
template <typename T, typename Enable=void>
class ArrayTraitsImpl {
public:
    // The value type of elements after all levels of nested containers have been dereferenced.
    typedef Error_WasNotContainer value_type;
    // The type of the range (a.k.a. iterator) that `get_range()` returns.
    typedef Error_WasNotContainer range_type;
    // Tells whether T is in fact a container type.
    static constexpr bool is_container = false;
    // This flag supports the legacy behavior of automatically guessing whether the data should
    // be treated as column major.  This guessing happens when `send()` is called rather than
    // `send1d()` or `send2d()`.  This is deprecated, but is still supported for reverse
    // compatibility.
    static constexpr bool allow_auto_unwrap = false;
    // The number of levels of nesting, or the dimension of multidimensional arrays.
    static constexpr size_t depth = 0;

    // Returns the range (iterator) for an array.
    static range_type get_range(const T &) {
        static_assert((sizeof(T)==0), "argument was not a container");
        throw std::logic_error("static assert should have been triggered by this point");
    }
};

template <typename T>
using ArrayTraits = ArrayTraitsImpl<typename std::remove_reference<typename std::remove_cv<T>::type>::type>;

// Most specializations of ArrayTraits should inherit from this (with V set to the value type).
// It sets some default values.
template <typename V>
class ArrayTraitsDefaults {
public:
    typedef V value_type;

    static constexpr bool is_container = true;
    static constexpr bool allow_auto_unwrap = true;
    static constexpr size_t depth = ArrayTraits<V>::depth + 1;
};

// }}}2

// {{{2 STL container support

template <typename TI, typename TV>
class IteratorRange {
public:
    IteratorRange() { }
    IteratorRange(const TI &_it, const TI &_end) : it(_it), end(_end) { }

    static constexpr bool is_container = ArrayTraits<TV>::is_container;

    // Error messages involving this stem from calling deref instead of deref_subiter for a nested
    // container.
    struct Error_InappropriateDeref { };
    using value_type = typename std::conditional_t<is_container, Error_InappropriateDeref, TV>;

    using subiter_type = typename ArrayTraits<TV>::range_type;

    bool is_end() const { return it == end; }

    void inc() { ++it; }

    value_type deref() const {
        static_assert(sizeof(TV) && !is_container,
            "deref called on nested container");
        if(is_end()) {
            throw std::runtime_error("attepted to dereference past end of iterator");
        }
        return *it;
    }

    subiter_type deref_subiter() const {
        static_assert(sizeof(TV) && is_container,
            "deref_subiter called on non-nested container");
        if(is_end()) {
            throw std::runtime_error("attepted to dereference past end of iterator");
        }
        return ArrayTraits<TV>::get_range(*it);
    }

private:
    TI it, end;
};

template <typename T>
class ArrayTraitsImpl<T,
    typename std::enable_if_t<is_like_stl_container<T>>
> : public ArrayTraitsDefaults<typename T::value_type> {
public:
    typedef IteratorRange<typename T::const_iterator, typename T::value_type> range_type;

    static range_type get_range(const T &arg) {
        return range_type(arg.begin(), arg.end());
    }
};

template <typename T>
class ArrayTraitsImpl<T,
    typename std::enable_if_t<is_like_stl_container2<T>>
> : public ArrayTraitsDefaults<typename std::iterator_traits<decltype(begin(std::declval<T const>()))>::value_type> {
    using IterType = decltype(begin(std::declval<T const>()));
    using ValType = typename std::iterator_traits<IterType>::value_type;
public:
    typedef IteratorRange<IterType, ValType> range_type;

    static range_type get_range(const T &arg) {
        return range_type(begin(arg), end(arg));
    }
};

// }}}2

// {{{2 C style array support

template <typename T, size_t N>
class ArrayTraitsImpl<T[N]> : public ArrayTraitsDefaults<T> {
public:
    typedef IteratorRange<const T*, T> range_type;

    static range_type get_range(const T (&arg)[N]) {
        return range_type(arg, arg+N);
    }
};

// }}}2

// {{{2 std::pair support

template <typename RT, typename RU>
class PairOfRange {
    template <typename T, typename U, typename PrintMode>
    friend void deref_and_print(std::ostream &, const PairOfRange<T, U> &, PrintMode);

public:
    PairOfRange() { }
    PairOfRange(const RT &_l, const RU &_r) : l(_l), r(_r) { }

    static constexpr bool is_container = RT::is_container && RU::is_container;

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
class ArrayTraitsImpl<std::pair<T, U>> {
public:
    typedef PairOfRange<typename ArrayTraits<T>::range_type, typename ArrayTraits<U>::range_type> range_type;
    typedef std::pair<typename ArrayTraits<T>::value_type, typename ArrayTraits<U>::value_type> value_type;
    static constexpr bool is_container = ArrayTraits<T>::is_container && ArrayTraits<U>::is_container;
    // Don't allow colwrap since it's already wrapped.
    static constexpr bool allow_auto_unwrap = false;
    // It is allowed for l_depth != r_depth, for example one column could be 'double' and the
    // other column could be 'vector<double>'.
    static constexpr size_t l_depth = ArrayTraits<T>::depth;
    static constexpr size_t r_depth = ArrayTraits<U>::depth;
    static constexpr size_t depth = (l_depth < r_depth) ? l_depth : r_depth;

    static range_type get_range(const std::pair<T, U> &arg) {
        return range_type(
            ArrayTraits<T>::get_range(arg.first),
            ArrayTraits<U>::get_range(arg.second)
        );
    }
};

// }}}2

// {{{2 boost::tuple support

template <typename T>
class ArrayTraitsImpl<T,
    typename std::enable_if_t<
        is_boost_tuple<T> && !is_boost_tuple_nulltype<typename T::tail_type>
    >
> : public ArrayTraits<
    typename std::pair<
        typename T::head_type,
        typename T::tail_type
    >
> {
public:
    typedef typename T::head_type HT;
    typedef typename T::tail_type TT;

    typedef ArrayTraits<typename std::pair<HT, TT>> parent;

    static typename parent::range_type get_range(const T &arg) {
        return typename parent::range_type(
            ArrayTraits<HT>::get_range(arg.get_head()),
            ArrayTraits<TT>::get_range(arg.get_tail())
        );
    }
};

template <typename T>
class ArrayTraitsImpl<T,
    typename std::enable_if_t<
        is_boost_tuple<T> && is_boost_tuple_nulltype<typename T::tail_type>
    >
> : public ArrayTraits<
    typename T::head_type
> {
    typedef typename T::head_type HT;

    typedef ArrayTraits<HT> parent;

public:
    static typename parent::range_type get_range(const T &arg) {
        return parent::get_range(arg.get_head());
    }
};

// }}}2

// {{{2 std::tuple support

template <typename Tuple, size_t idx>
struct StdTupUnwinder {
    typedef std::pair<
        typename StdTupUnwinder<Tuple, idx-1>::type,
        typename std::tuple_element<idx, Tuple>::type
    > type;

    static typename ArrayTraits<type>::range_type get_range(const Tuple &arg) {
        return typename ArrayTraits<type>::range_type(
            StdTupUnwinder<Tuple, idx-1>::get_range(arg),
            ArrayTraits<typename std::tuple_element<idx, Tuple>::type>::get_range(std::get<idx>(arg))
        );
    }
};

template <typename Tuple>
struct StdTupUnwinder<Tuple, 0> {
    typedef typename std::tuple_element<0, Tuple>::type type;

    static typename ArrayTraits<type>::range_type get_range(const Tuple &arg) {
        return ArrayTraits<type>::get_range(std::get<0>(arg));
    }
};

template <typename... Args>
class ArrayTraitsImpl<std::tuple<Args...>> :
    public ArrayTraits<typename StdTupUnwinder<std::tuple<Args...>, sizeof...(Args)-1>::type>
{
    typedef std::tuple<Args...> Tuple;
    typedef ArrayTraits<typename StdTupUnwinder<Tuple, sizeof...(Args)-1>::type> parent;

public:
    static typename parent::range_type get_range(const Tuple &arg) {
        return StdTupUnwinder<std::tuple<Args...>, sizeof...(Args)-1>::get_range(arg);
    }
};

// }}}2

// {{{2 Support column unwrap of container (VecOfRange)
//
// VecOfRange (created via `get_columns_range()`) treats the outermost level of a nested
// container as if it were a tuple.  Since tuples are sent to gnuplot as columns, this has the
// effect of addressing a multidimensional array in column major order.

template <typename RT>
class VecOfRange {
    template <typename T, typename PrintMode>
    friend void deref_and_print(std::ostream &, const VecOfRange<T> &, PrintMode);

public:
    VecOfRange() { }
    explicit VecOfRange(const std::vector<RT> &_rvec) : rvec(_rvec) { }

    static constexpr bool is_container = RT::is_container;
    // Don't allow colwrap since it's already wrapped.
    static constexpr bool allow_auto_unwrap = false;

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

// }}}2

// }}}1

// {{{1 Array printing functions
//
// This section coordinates the sending of data to gnuplot.  The ArrayTraits mechanism tells us
// about nested containers and provides iterators over them.  Here we make use of this,
// deciding what dimensions should be treated as rows, columns, or blocks, telling gnuplot the
// size of the array if needed, and so on.

// If this is set, then text-mode data will be sent in a format that is not compatible with
// gnuplot, but which helps the programmer tell what the library is thinking.  Basically it
// puts brackets around groups of items and puts a message delineating blocks of data.
static bool debug_array_print = 0;

// This is thrown when an empty container is being plotted.  This exception should always
// be caught and should not propagate to the user.
class plotting_empty_container : public std::length_error {
public:
    plotting_empty_container() : std::length_error("plotting empty container") { }
};

// {{{2 Tags (like enums for metaprogramming)

// These tags define what our goal is, what sort of thing should ultimately be sent to the
// ostream.  These tags are passed to the PrintMode template argument of the functions in this
// section.
//
// ModeText   - Sends the data in an array in text format
// ModeBinary - Sends the data in an array in binary format
// ModeBinfmt - Sends the gnuplot format code for binary data (e.g. "%double%double")
// ModeSize   - Sends the size of an array.  Needed when sending binary data.
struct ModeText   { static constexpr bool is_text = 1; static constexpr bool is_binfmt = 0; static constexpr bool is_size = 0; };
struct ModeBinary { static constexpr bool is_text = 0; static constexpr bool is_binfmt = 0; static constexpr bool is_size = 0; };
struct ModeBinfmt { static constexpr bool is_text = 0; static constexpr bool is_binfmt = 1; static constexpr bool is_size = 0; };
struct ModeSize   { static constexpr bool is_text = 0; static constexpr bool is_binfmt = 0; static constexpr bool is_size = 1; };

// Whether to treat the outermost level of a nested container as columns (column major mode).
struct ColUnwrapNo  { };
struct ColUnwrapYes { };

// The user must give a hint to describe how nested containers are to be interpreted.  This is
// done by calling e.g. `send1d_colmajor()` or `send2d()`.  This hint is then described by the
// following tags.  This is passed to the OrganizationMode template argument.
struct Mode1D       { static std::string class_name() { return "Mode1D"      ; } };
struct Mode2D       { static std::string class_name() { return "Mode2D"      ; } };
struct Mode1DUnwrap { static std::string class_name() { return "Mode1DUnwrap"; } };
struct Mode2DUnwrap { static std::string class_name() { return "Mode2DUnwrap"; } };
// Support for the legacy behavior that guesses which of the above four modes should be used.
struct ModeAuto     { static std::string class_name() { return "ModeAuto"    ; } };

// }}}2

// {{{2 ModeAutoDecoder
//
// ModeAuto guesses which of Mode1D, Mode2D, Mode1DUnwrap, or Mode2DUnwrap should be used.
// This is provided for reverse compatibility; it is better to specify explicitly which mode to
// use.  Since this is only for reverse compatibility, and shouldn't be used, I'm not going to
// spell out what the rules are.  See below for details.

template <typename T, typename Enable=void>
struct ModeAutoDecoder { };

template <typename T>
struct ModeAutoDecoder<T,
    typename std::enable_if_t<
        (ArrayTraits<T>::depth == 1)
    >>
{
    typedef Mode1D mode;
};

template <typename T>
struct ModeAutoDecoder<T,
    typename std::enable_if_t<
        (ArrayTraits<T>::depth == 2) &&
        !ArrayTraits<T>::allow_auto_unwrap
    >>
{
    typedef Mode2D mode;
};

template <typename T>
struct ModeAutoDecoder<T,
    typename std::enable_if_t<
        (ArrayTraits<T>::depth == 2) &&
        ArrayTraits<T>::allow_auto_unwrap
    >>
{
    typedef Mode1DUnwrap mode;
};

template <typename T>
struct ModeAutoDecoder<T,
    typename std::enable_if_t<
        (ArrayTraits<T>::depth > 2) &&
        ArrayTraits<T>::allow_auto_unwrap
    >>
{
    typedef Mode2DUnwrap mode;
};

template <typename T>
struct ModeAutoDecoder<T,
    typename std::enable_if_t<
        (ArrayTraits<T>::depth > 2) &&
        !ArrayTraits<T>::allow_auto_unwrap
    >>
{
    typedef Mode2D mode;
};

// }}}2

// The data is processed using several levels of functions that call each other in sequence,
// each defined in a subsection of code below.  Because C++ wants you to declare a function
// before using it, we begin with the innermost function.  So in order to see the sequence in
// which these are called, you should read the following subsections in reverse order.  Nested
// arrays are formated into blocks (for 2D data) and lines (for 1D or 2D data), then further
// nesting levels are formatted into columns.  Also tag dispatching is used in order to define
// various sorts of behavior.  Each of these tasks is handled by one of the following
// subsections.

// {{{2 send_scalar()
//
// Send a scalar in one of three possible ways: via TextSender, BinarySender, or BinfmtSender,
// depending on which PrintMode tag is passed to the function.

template <typename T>
void send_scalar(std::ostream &stream, const T &arg, ModeText) {
    TextSender<T>::send(stream, arg);
}

template <typename T>
void send_scalar(std::ostream &stream, const T &arg, ModeBinary) {
    BinarySender<T>::send(stream, arg);
}

template <typename T>
void send_scalar(std::ostream &stream, const T &, ModeBinfmt) {
    BinfmtSender<T>::send(stream);
}

// }}}2

// {{{2 deref_and_print()
//
// Dereferences and prints the given range (iterator).  At this point we are done with treating
// containers as blocks (for 2D data) and lines (for 1D or 2D data).  Any further levels of
// nested containers will at this point be treated as columns.

// If arg is not a container, then print it via send_scalar().
template <typename T, typename PrintMode>
typename std::enable_if_t<!T::is_container>
deref_and_print(std::ostream &stream, const T &arg, PrintMode) {
    const typename T::value_type &v = arg.deref();
    send_scalar(stream, v, PrintMode());
}

// If arg is a container (but not a PairOfRange or VecOfRange, which are handled below) then
// treat the contents as columns, iterating over the contents recursively.  If outputting in
// text mode, put a space between columns.
template <typename T, typename PrintMode>
typename std::enable_if_t<T::is_container>
deref_and_print(std::ostream &stream, const T &arg, PrintMode) {
    if(arg.is_end()) throw plotting_empty_container();
    typename T::subiter_type subrange = arg.deref_subiter();
    if(PrintMode::is_binfmt && subrange.is_end()) throw plotting_empty_container();
    if(debug_array_print && PrintMode::is_text) stream << "{";
    bool first = true;
    while(!subrange.is_end()) {
        if(!first && PrintMode::is_text) stream << " ";
        first = false;
        deref_and_print(stream, subrange, PrintMode());
        subrange.inc();
    }
    if(debug_array_print && PrintMode::is_text) stream << "}";
}

// PairOfRange is treated as columns.  In text mode, put a space between columns.
template <typename T, typename U, typename PrintMode>
void deref_and_print(std::ostream &stream, const PairOfRange<T, U> &arg, PrintMode) {
    deref_and_print(stream, arg.l, PrintMode());
    if(PrintMode::is_text) stream << " ";
    deref_and_print(stream, arg.r, PrintMode());
}

// VecOfRange is treated as columns.  In text mode, put a space between columns.
template <typename T, typename PrintMode>
void deref_and_print(std::ostream &stream, const VecOfRange<T> &arg, PrintMode) {
    if(PrintMode::is_binfmt && arg.rvec.empty()) throw plotting_empty_container();
    for(size_t i=0; i<arg.rvec.size(); i++) {
        if(i && PrintMode::is_text) stream << " ";
        deref_and_print(stream, arg.rvec[i], PrintMode());
    }
}

// }}}2

// {{{2 print_block()
//
// Here we format nested containers into blocks (for 2D data) and lines.  Actually, block and
// line formatting is only truely needed for text mode output, but for uniformity this function
// is also invoked in binary mode (the PrintMode tag determines the output mode).  If the goal
// is to just print the array size or the binary format string, then the loops exit after the
// first iteration.
//
// The Depth argument tells how deep to recurse.  It will be either `2` for 2D data, formatted
// into blocks and lines, with empty lines between blocks, or `1` for 1D data formatted into
// lines but not blocks.  Gnuplot only supports 1D and 2D data, but if it were to support 3D in
// the future (e.g. volume rendering), all that would be needed would be some trivial changes
// in this section.  After Depth number of nested containers have been recursed into, control
// is passed to deref_and_print(), which treats any further nested containers as columns.

// Depth==1 and we are not asked to print the size of the array.  Send each element of the
// range to deref_and_print() for further processing into columns.
template <size_t Depth, typename T, typename PrintMode>
typename std::enable_if_t<(Depth==1) && !PrintMode::is_size>
print_block(std::ostream &stream, T &arg, PrintMode) {
    if(PrintMode::is_binfmt && arg.is_end()) throw plotting_empty_container();
    for(; !arg.is_end(); arg.inc()) {
        //print_entry(arg.deref());
        deref_and_print(stream, arg, PrintMode());
        // If asked to print the binary format string, only the first element needs to be
        // looked at.
        if(PrintMode::is_binfmt) break;
        if(PrintMode::is_text) stream << std::endl;
    }
}

// Depth>1 and we are not asked to print the size of the array.  Loop over the range and
// recurse into print_block() with Depth -> Depth-1.
template <size_t Depth, typename T, typename PrintMode>
typename std::enable_if_t<(Depth>1) && !PrintMode::is_size>
print_block(std::ostream &stream, T &arg, PrintMode) {
    if(PrintMode::is_binfmt && arg.is_end()) throw plotting_empty_container();
    bool first = true;
    for(; !arg.is_end(); arg.inc()) {
        if(first) {
            first = false;
        } else {
            if(PrintMode::is_text) stream << std::endl;
        }
        if(debug_array_print && PrintMode::is_text) stream << "<block>" << std::endl;
        if(arg.is_end()) throw plotting_empty_container();
        typename T::subiter_type sub = arg.deref_subiter();
        print_block<Depth-1>(stream, sub, PrintMode());
        // If asked to print the binary format string, only the first element needs to be
        // looked at.
        if(PrintMode::is_binfmt) break;
    }
}

// Determine how many elements are in the given range.  Used in the functions below.
template <typename T>
size_t get_range_size(const T &arg) {
    // FIXME - not the fastest way.  Implement a size() method for range.
    size_t ret = 0;
    for(T i=arg; !i.is_end(); i.inc()) ++ret;
    return ret;
}

// Depth==1 and we are asked to print the size of the array.
template <size_t Depth, typename T, typename PrintMode>
typename std::enable_if_t<(Depth==1) && PrintMode::is_size>
print_block(std::ostream &stream, T &arg, PrintMode) {
    stream << get_range_size(arg);
}

// Depth>1 and we are asked to print the size of the array.
template <size_t Depth, typename T, typename PrintMode>
typename std::enable_if_t<(Depth>1) && PrintMode::is_size>
print_block(std::ostream &stream, T &arg, PrintMode) {
    if(arg.is_end()) throw plotting_empty_container();
    // It seems that size for two dimensional arrays needs the fastest varying index first,
    // contrary to intuition.  The gnuplot documentation is not too clear on this point.
    typename T::subiter_type sub = arg.deref_subiter();
    print_block<Depth-1>(stream, sub, PrintMode());
    stream << "," << get_range_size(arg);
}

// }}}2

// {{{2 handle_colunwrap_tag()
//
// If passed the ColUnwrapYes then treat the outermost nested container as columns by calling
// get_columns_range().  Otherwise just call get_range().  The range iterator is then passed to
// print_block() for further processing.

template <size_t Depth, typename T, typename PrintMode>
void handle_colunwrap_tag(std::ostream &stream, const T &arg, ColUnwrapNo, PrintMode) {
    static_assert(ArrayTraits<T>::depth >= Depth, "container not deep enough");
    typename ArrayTraits<T>::range_type range = ArrayTraits<T>::get_range(arg);
    print_block<Depth>(stream, range, PrintMode());
}

template <size_t Depth, typename T, typename PrintMode>
void handle_colunwrap_tag(std::ostream &stream, const T &arg, ColUnwrapYes, PrintMode) {
    static_assert(ArrayTraits<T>::depth >= Depth+1, "container not deep enough");
    VecOfRange<typename ArrayTraits<T>::range_type::subiter_type> cols = get_columns_range(arg);
    print_block<Depth>(stream, cols, PrintMode());
}

// }}}2

// {{{2 handle_organization_tag()
//
// Parse the OrganizationMode tag then forward to handle_colunwrap_tag() for further
// processing.  If passed the Mode1D or Mode2D tags, then set Depth=1 or Depth=2.  If passed
// Mode{1,2}DUnwrap then use the ColUnwrapYes tag.  If passed ModeAuto (which is for legacy
// support) then use ModeAutoDecoder to guess which of Mode1D, Mode2D, etc. should be used.

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, Mode1D, PrintMode) {
    handle_colunwrap_tag<1>(stream, arg, ColUnwrapNo(), PrintMode());
}

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, Mode2D, PrintMode) {
    handle_colunwrap_tag<2>(stream, arg, ColUnwrapNo(), PrintMode());
}

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, Mode1DUnwrap, PrintMode) {
    handle_colunwrap_tag<1>(stream, arg, ColUnwrapYes(), PrintMode());
}

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, Mode2DUnwrap, PrintMode) {
    handle_colunwrap_tag<2>(stream, arg, ColUnwrapYes(), PrintMode());
}

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, ModeAuto, PrintMode) {
    handle_organization_tag(stream, arg, typename ModeAutoDecoder<T>::mode(), PrintMode());
}

// }}}2

// The entry point for the processing defined in this section.  It just forwards immediately to
// handle_organization_tag().  This function is only here to give a sane name to the entry
// point.
//
// The allowed values for the OrganizationMode and PrintMode tags are defined in the beginning
// of this section.
template <typename T, typename OrganizationMode, typename PrintMode>
void top_level_array_sender(std::ostream &stream, const T &arg, OrganizationMode, PrintMode) {
    handle_organization_tag(stream, arg, OrganizationMode(), PrintMode());
}

// }}}1

// {{{1 PlotGroup

class PlotData {
public:
    PlotData() { }

    template <typename T, typename OrganizationMode, typename PrintMode>
    PlotData(
        const T &arg,
        const std::string &_plotspec,
        const std::string &_arr_or_rec,
        OrganizationMode, PrintMode
    ) :
        plotspec(_plotspec),
        is_text(PrintMode::is_text),
        is_inline(true),
        has_data(true),
        arr_or_rec(_arr_or_rec)
    {
        {
            std::ostringstream tmp;
            top_level_array_sender(tmp, arg, OrganizationMode(), PrintMode());
            data = tmp.str();
        }

        if(!is_text) {
            try {
                {
                    std::ostringstream tmp;
                    top_level_array_sender(tmp, arg, OrganizationMode(), ModeBinfmt());
                    bin_fmt = tmp.str();
                }
                {
                    std::ostringstream tmp;
                    top_level_array_sender(tmp, arg, OrganizationMode(), ModeSize());
                    bin_size = tmp.str();
                }
            } catch(const plotting_empty_container &) {
                bin_fmt = "";
                bin_size = "0";
            }
        }
    }

    explicit PlotData(const std::string &_plotspec) :
        plotspec(_plotspec),
        is_text(true),
        is_inline(false),
        has_data(false)
    { }

    PlotData &file(const std::string &fn) {
        filename = fn;
        is_inline = false;

        std::ios_base::openmode mode = std::fstream::out;
        if(!is_text) mode |= std::fstream::binary;
        std::fstream fh(filename.c_str(), mode);
        fh << data;
        fh.close();

        return *this;
    }

    std::string plotCmd() const {
        std::string cmd;
        if(has_data) {
            if(filename.empty()) {
                cmd += "'-' ";
            } else {
                // FIXME - hopefully filename doesn't contain quotes or such...
                cmd += "'" + filename + "' ";
            }
            if(!is_text) {
                cmd += binConfig() + " ";
            }
        }
        cmd += plotspec;
        return cmd;
    }

    bool isInline() const {
        return is_inline;
    }

    const std::string &getData() const {
        return data;
    }

    bool isText() const { return is_text; }

    bool isBinary() const { return !is_text; }

private:
    std::string binConfig() const {
        return "binary format='" + bin_fmt + "' " + arr_or_rec + "=(" + bin_size + ")";
    }

private:
    std::string plotspec;
    bool is_text;
    bool is_inline;
    bool has_data;
    std::string data;
    std::string filename;
    std::string arr_or_rec;
    std::string bin_fmt;
    std::string bin_size;
};

class PlotGroup {
public:
    friend class Gnuplot;

    explicit PlotGroup(const std::string &plot_type_) : plot_type(plot_type_) { }

    PlotGroup &add_preamble(const std::string &s) {
        preamble_lines.push_back(s);
        return *this;
    }

    PlotGroup &add_plot(const std::string &plotspec) { plots.emplace_back(plotspec); return *this; }

    template <typename T> PlotGroup &add_plot1d         (const T &arg, const std::string &plotspec="", const std::string &text_array_record="text") { add(arg, plotspec, text_array_record, Mode1D      ()); return *this; }
    template <typename T> PlotGroup &add_plot2d         (const T &arg, const std::string &plotspec="", const std::string &text_array_record="text") { add(arg, plotspec, text_array_record, Mode2D      ()); return *this; }
    template <typename T> PlotGroup &add_plot1d_colmajor(const T &arg, const std::string &plotspec="", const std::string &text_array_record="text") { add(arg, plotspec, text_array_record, Mode1DUnwrap()); return *this; }
    template <typename T> PlotGroup &add_plot2d_colmajor(const T &arg, const std::string &plotspec="", const std::string &text_array_record="text") { add(arg, plotspec, text_array_record, Mode2DUnwrap()); return *this; }

    PlotGroup &file(const std::string &fn) {
        assert(!plots.empty());
        plots.back().file(fn);
        return *this;
    }

    size_t num_plots() const { return plots.size(); }

private:
    template <typename T, typename OrganizationMode>
    void add(const T &arg, const std::string &plotspec, const std::string &text_array_record, OrganizationMode) {
        if(!(
            text_array_record == "text" ||
            text_array_record == "array" ||
            text_array_record == "record"
        )) throw std::logic_error("text_array_record must be one of text, array, or record (was "+
            text_array_record+")");

        if(text_array_record == "text") {
            plots.emplace_back(arg, plotspec,
                "array", // arbitrary value
                OrganizationMode(), ModeText());
        } else {
            plots.emplace_back(arg, plotspec, text_array_record,
                OrganizationMode(), ModeBinary());
        }
    }

    std::string plot_type;
    std::vector<std::string> preamble_lines;
    std::vector<PlotData> plots;
};

// }}}1

// {{{1 FileHandleWrapper

// This holds the file handle that gnuplot commands will be sent to.  The purpose of this
// wrapper is twofold:
// 1. It allows storing the FILE* before it gets passed to the boost::iostreams::stream
//    constructor (which is a base class of the main Gnuplot class).  This is accomplished
//    via multiple inheritance as described at http://stackoverflow.com/a/3821756/1048959
// 2. It remembers whether the handle needs to be closed via fclose or pclose.
struct FileHandleWrapper {
    FileHandleWrapper(std::FILE *_fh, bool _should_use_pclose) :
        wrapped_fh(_fh), should_use_pclose(_should_use_pclose) { }

    void fh_close() {
        if(should_use_pclose) {
            if(GNUPLOT_PCLOSE(wrapped_fh)) {
                perror("pclose");
                //char msg[1000];
                //strerror_s(msg, sizeof(msg), errno);
                //std::cerr << "pclose returned error: " << msg << std::endl;
            }
        } else {
            if(fclose(wrapped_fh)) {
                std::cerr << "fclose returned error" << std::endl;
            }
        }
    }

    int fh_fileno() {
        return GNUPLOT_FILENO(wrapped_fh);
    }

    std::FILE *wrapped_fh;
    bool should_use_pclose;
};

// }}}1

// {{{1 Main class

class Gnuplot :
    // Some setup needs to be done before obtaining the file descriptor that gets passed to
    // boost::iostreams::stream.  This is accomplished by using a multiple inheritance trick,
    // as described at http://stackoverflow.com/a/3821756/1048959
    private FileHandleWrapper,
    public boost::iostreams::stream<boost::iostreams::file_descriptor_sink>
{
private:
    static std::string get_default_cmd() {
        GNUPLOT_MSVC_WARNING_4996_PUSH
        char *from_env = std::getenv("GNUPLOT_IOSTREAM_CMD");
        GNUPLOT_MSVC_WARNING_4996_POP
        if(from_env && from_env[0]) {
            return from_env;
        } else {
            return GNUPLOT_DEFAULT_COMMAND;
        }
    }

    static FileHandleWrapper open_cmdline(const std::string &in) {
        std::string cmd = in.empty() ? get_default_cmd() : in;
        assert(!cmd.empty());
        if(cmd[0] == '>') {
            std::string fn = cmd.substr(1);
            GNUPLOT_MSVC_WARNING_4996_PUSH
            FILE *fh = std::fopen(fn.c_str(), "w");
            GNUPLOT_MSVC_WARNING_4996_POP
            if(!fh) throw std::ios_base::failure("cannot open file "+fn);
            return FileHandleWrapper(fh, false);
        } else {
            FILE *fh = GNUPLOT_POPEN(cmd.c_str(), "w");
            if(!fh) throw std::ios_base::failure("cannot open pipe "+cmd);
            return FileHandleWrapper(fh, true);
        }
    }

public:
    explicit Gnuplot(const std::string &_cmd="") :
        FileHandleWrapper(open_cmdline(_cmd)),
        boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
            fh_fileno(),
#if BOOST_VERSION >= 104400
            boost::iostreams::never_close_handle
#else
            false
#endif
        ),
        feedback(nullptr),
        tmp_files(new GnuplotTmpfileCollection()),
        debug_messages(false),
        transport_tmpfile(false)
    {
        set_stream_options(*this);
    }

    explicit Gnuplot(FILE *_fh) :
        FileHandleWrapper(_fh, 0),
        boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
            fh_fileno(),
#if BOOST_VERSION >= 104400
            boost::iostreams::never_close_handle
#else
            false
#endif
        ),
        feedback(nullptr),
        tmp_files(new GnuplotTmpfileCollection()),
        debug_messages(false),
        transport_tmpfile(false)
    {
        set_stream_options(*this);
    }

private:
    // noncopyable
    Gnuplot(const Gnuplot &) = delete;
    const Gnuplot& operator=(const Gnuplot &) = delete;

public:
    ~Gnuplot() {
        if(debug_messages) {
            std::cerr << "ending gnuplot session" << std::endl;
        }

        // FIXME - boost's close method calls close() on the file descriptor, but we need to
        // use sometimes use pclose instead.  For now, just skip calling boost's close and use
        // flush just in case.
        do_flush();
        // Wish boost had a pclose method...
        //close();

        fh_close();

        delete feedback;
    }

    void useTmpFile(bool state) {
        transport_tmpfile = state;
    }

    void clearTmpfiles() {
        // destructors will cause deletion
        tmp_files->clear();
    }

public:
    void do_flush() {
        *this << std::flush;
        fflush(wrapped_fh);
    }

private:
    std::string make_tmpfile() {
        return tmp_files->make_tmpfile();
    }

    void set_stream_options(std::ostream &os) const
    {
        os << std::defaultfloat << std::setprecision(17);  // refer <iomanip>
    }

public:
// {{{2 Generic sender routines.
//
// These are declared public, but are undocumented.  It is recommended to use the functions in
// the next section, which serve as adapters that pass specific values for the OrganizationMode
// tag.

    template <typename T, typename OrganizationMode>
    Gnuplot &send(const T &arg, OrganizationMode) {
        top_level_array_sender(*this, arg, OrganizationMode(), ModeText());
        *this << "e" << std::endl; // gnuplot's "end of array" token
        do_flush(); // probably not really needed, but doesn't hurt
        return *this;
    }

    template <typename T, typename OrganizationMode>
    Gnuplot &sendBinary(const T &arg, OrganizationMode) {
        top_level_array_sender(*this, arg, OrganizationMode(), ModeBinary());
        do_flush(); // probably not really needed, but doesn't hurt
        return *this;
    }

    template <typename T, typename OrganizationMode>
    std::string binfmt(const T &arg, const std::string &arr_or_rec, OrganizationMode) {
        assert((arr_or_rec == "array") || (arr_or_rec == "record"));
        std::string ret;
        try {
            std::ostringstream tmp;
            tmp << " format='";
            top_level_array_sender(tmp, arg, OrganizationMode(), ModeBinfmt());
            tmp << "' " << arr_or_rec << "=(";
            top_level_array_sender(tmp, arg, OrganizationMode(), ModeSize());
            tmp << ")";
            tmp << " ";
            ret = tmp.str();
        } catch(const plotting_empty_container &) {
            ret = std::string(" format='' ") + arr_or_rec + "=(0) ";
        }
        return ret;
    }

    // NOTE: empty filename makes temporary file
    template <typename T, typename OrganizationMode>
    std::string file(const T &arg, std::string filename, OrganizationMode) {
        if(filename.empty()) filename = make_tmpfile();
        std::fstream tmp_stream(filename.c_str(), std::fstream::out);
        tmp_stream.copyfmt(*this);
        top_level_array_sender(tmp_stream, arg, OrganizationMode(), ModeText());
        tmp_stream.close();

        std::ostringstream cmdline;
        // FIXME - hopefully filename doesn't contain quotes or such...
        cmdline << " '" << filename << "' ";
        return cmdline.str();
    }

    // NOTE: empty filename makes temporary file
    template <typename T, typename OrganizationMode>
    std::string binaryFile(const T &arg, std::string filename, const std::string &arr_or_rec, OrganizationMode) {
        if(filename.empty()) filename = make_tmpfile();
        std::fstream tmp_stream(filename.c_str(), std::fstream::out | std::fstream::binary);
        top_level_array_sender(tmp_stream, arg, OrganizationMode(), ModeBinary());
        tmp_stream.close();

        std::ostringstream cmdline;
        // FIXME - hopefully filename doesn't contain quotes or such...
        cmdline << " '" << filename << "' binary" << binfmt(arg, arr_or_rec, OrganizationMode());
        return cmdline.str();
    }

// }}}2

// {{{2 Deprecated data sending interface that guesses an appropriate OrganizationMode.  This is here
// for reverse compatibility.  Don't use it.  A warning will be printed if
// GNUPLOT_DEPRECATE_WARN is defined.

    template <typename T> Gnuplot GNUPLOT_DEPRECATE("use send1d or send2d")
        &send(const T &arg) { return send(arg, ModeAuto()); }

    template <typename T> std::string GNUPLOT_DEPRECATE("use binfmt1d or binfmt2d")
        binfmt(const T &arg, const std::string &arr_or_rec="array")
        { return binfmt(arg, arr_or_rec,  ModeAuto()); }

    template <typename T> Gnuplot GNUPLOT_DEPRECATE("use sendBinary1d or sendBinary2d")
        &sendBinary(const T &arg) { return sendBinary(arg, ModeAuto()); }

    template <typename T> std::string GNUPLOT_DEPRECATE("use file1d or file2d")
        file(const T &arg, const std::string &filename="")
        { return file(arg, filename, ModeAuto()); }

    template <typename T> std::string GNUPLOT_DEPRECATE("use binArr1d or binArr2d")
        binaryFile(const T &arg, const std::string &filename="", const std::string &arr_or_rec="array")
        { return binaryFile(arg, filename, arr_or_rec,  ModeAuto()); }

// }}}2

// {{{2 Public (documented) data sending interface.
//
// It seems odd to define 16 different functions, but I think this ends up being the most
// convenient in terms of usage by the end user.

    template <typename T> Gnuplot &send1d         (const T &arg) { return send(arg, Mode1D      ()); }
    template <typename T> Gnuplot &send2d         (const T &arg) { return send(arg, Mode2D      ()); }
    template <typename T> Gnuplot &send1d_colmajor(const T &arg) { return send(arg, Mode1DUnwrap()); }
    template <typename T> Gnuplot &send2d_colmajor(const T &arg) { return send(arg, Mode2DUnwrap()); }

    template <typename T> Gnuplot &sendBinary1d         (const T &arg) { return sendBinary(arg, Mode1D      ()); }
    template <typename T> Gnuplot &sendBinary2d         (const T &arg) { return sendBinary(arg, Mode2D      ()); }
    template <typename T> Gnuplot &sendBinary1d_colmajor(const T &arg) { return sendBinary(arg, Mode1DUnwrap()); }
    template <typename T> Gnuplot &sendBinary2d_colmajor(const T &arg) { return sendBinary(arg, Mode2DUnwrap()); }

    template <typename T> std::string file1d         (const T &arg, const std::string &filename="") { return file(arg, filename, Mode1D      ()); }
    template <typename T> std::string file2d         (const T &arg, const std::string &filename="") { return file(arg, filename, Mode2D      ()); }
    template <typename T> std::string file1d_colmajor(const T &arg, const std::string &filename="") { return file(arg, filename, Mode1DUnwrap()); }
    template <typename T> std::string file2d_colmajor(const T &arg, const std::string &filename="") { return file(arg, filename, Mode2DUnwrap()); }

    template <typename T> std::string binFmt1d         (const T &arg, const std::string &arr_or_rec) { return binfmt(arg, arr_or_rec,  Mode1D      ()); }
    template <typename T> std::string binFmt2d         (const T &arg, const std::string &arr_or_rec) { return binfmt(arg, arr_or_rec,  Mode2D      ()); }
    template <typename T> std::string binFmt1d_colmajor(const T &arg, const std::string &arr_or_rec) { return binfmt(arg, arr_or_rec,  Mode1DUnwrap()); }
    template <typename T> std::string binFmt2d_colmajor(const T &arg, const std::string &arr_or_rec) { return binfmt(arg, arr_or_rec,  Mode2DUnwrap()); }

    template <typename T> std::string binFile1d         (const T &arg, const std::string &arr_or_rec, const std::string &filename="") { return binaryFile(arg, filename, arr_or_rec,  Mode1D      ()); }
    template <typename T> std::string binFile2d         (const T &arg, const std::string &arr_or_rec, const std::string &filename="") { return binaryFile(arg, filename, arr_or_rec,  Mode2D      ()); }
    template <typename T> std::string binFile1d_colmajor(const T &arg, const std::string &arr_or_rec, const std::string &filename="") { return binaryFile(arg, filename, arr_or_rec,  Mode1DUnwrap()); }
    template <typename T> std::string binFile2d_colmajor(const T &arg, const std::string &arr_or_rec, const std::string &filename="") { return binaryFile(arg, filename, arr_or_rec,  Mode2DUnwrap()); }

// }}}2

#ifdef GNUPLOT_ENABLE_FEEDBACK
public:
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

private:
    void allocFeedback() {
        if(!feedback) {
#ifdef GNUPLOT_ENABLE_PTY
            feedback = new GnuplotFeedbackPty(debug_messages);
//#elif defined GNUPLOT_USE_TMPFILE
//// Currently this doesn't work since fscanf doesn't block (need something like "tail -f")
//            feedback = new GnuplotFeedbackTmpfile(debug_messages);
#else
            // This shouldn't happen because we are in an `#ifdef GNUPLOT_ENABLE_FEEDBACK`
            // block which should only be activated if GNUPLOT_ENABLE_PTY is defined.
            static_assert((sizeof(T) == 0), "No feedback mechanism defined.");
#endif
            *this << "set print \"" << feedback->filename() << "\"" << std::endl;
        }
    }
#endif // GNUPLOT_ENABLE_FEEDBACK

// {{{2 PlotGroup

public:
    static PlotGroup plotGroup() {
        return PlotGroup("plot");
    }

    static PlotGroup splotGroup() {
        return PlotGroup("splot");
    }

    Gnuplot &send(const PlotGroup &plot_group) {
        return send(PlotGroup(plot_group));
    }

    Gnuplot &send(const PlotGroup &&plot_group) {
        for(const std::string &s : plot_group.preamble_lines) {
            *this << s << "\n";
        }

        std::vector<PlotData> spl = std::move(plot_group.plots);

        if(transport_tmpfile) {
            for(size_t i=0; i<spl.size(); i++) {
                if(spl[i].isInline()) {
                    spl[i].file(make_tmpfile());
                }
            }
        }

        int need_sort = 0;
        for(const PlotData &sp : spl) {
            if(need_sort==0 && sp.isInline() && sp.isBinary()) need_sort = 1;
            if(need_sort==1 && sp.isInline() && sp.isText  ()) need_sort = 2;
        }
        if(need_sort == 2) { // inline text occurs after inline binary
            std::stable_sort(spl.begin(), spl.end(), [](const PlotData &a, const PlotData &b) {
                bool x = a.isInline() && a.isBinary();
                bool y = b.isInline() && b.isBinary();
                return x < y;
            });
        }

        *this << plot_group.plot_type << " ";
        for(size_t i=0; i<spl.size(); i++) {
            if(i) *this << ", ";
            *this << spl[i].plotCmd();
        }
        *this << std::endl;

        for(const PlotData &sp : spl) {
            if(sp.isInline()) {
                *this << sp.getData();
                if(sp.isText()) {
                    *this << "e" << std::endl; // gnuplot's "end of array" token
                }
            }
        }

        do_flush();

        return *this;
    }
// }}}2

private:
    GnuplotFeedback *feedback;
    std::shared_ptr<GnuplotTmpfileCollection> tmp_files;
public:
    bool debug_messages;
    bool transport_tmpfile;
};

inline Gnuplot &operator<<(Gnuplot &gp, PlotGroup &sp) {
    return gp.send(sp);
}

inline Gnuplot &operator<<(Gnuplot &gp, PlotGroup &&sp) {
    return gp.send(sp);
}

// }}}1

} // namespace gnuplotio

// The first version of this library didn't use namespaces, and now this must be here forever
// for reverse compatibility.
using gnuplotio::Gnuplot;

#endif // GNUPLOT_IOSTREAM_H

// {{{1 Support for 3rd party array libraries

// {{{2 Blitz support

// This is outside of the main header guard so that it will be compiled when people do
// something like this:
//    #include "gnuplot-iostream.h"
//    #include <blitz/array.h>
//    #include "gnuplot-iostream.h"
// Note that it has its own header guard to avoid double inclusion.

#ifdef BZ_BLITZ_H
#ifndef GNUPLOT_BLITZ_SUPPORT_LOADED
#define GNUPLOT_BLITZ_SUPPORT_LOADED
namespace gnuplotio {

template <typename T, int N>
struct BinfmtSender<blitz::TinyVector<T, N>> {
    static void send(std::ostream &stream) {
        for(int i=0; i<N; i++) {
            BinfmtSender<T>::send(stream);
        }
    }
};

template <typename T, int N>
struct TextSender<blitz::TinyVector<T, N>> {
    static void send(std::ostream &stream, const blitz::TinyVector<T, N> &v) {
        for(int i=0; i<N; i++) {
            if(i) stream << " ";
            TextSender<T>::send(stream, v[i]);
        }
    }
};

template <typename T, int N>
struct BinarySender<blitz::TinyVector<T, N>> {
    static void send(std::ostream &stream, const blitz::TinyVector<T, N> &v) {
        for(int i=0; i<N; i++) {
            BinarySender<T>::send(stream, v[i]);
        }
    }
};

class Error_WasBlitzPartialSlice { };

template <typename T, int ArrayDim, int SliceDim>
class BlitzIterator {
public:
    BlitzIterator() : p(nullptr) { }
    BlitzIterator(
        const blitz::Array<T, ArrayDim> *_p,
        const blitz::TinyVector<int, ArrayDim> _idx
    ) : p(_p), idx(_idx) { }

    typedef Error_WasBlitzPartialSlice value_type;
    typedef BlitzIterator<T, ArrayDim, SliceDim-1> subiter_type;
    static constexpr bool is_container = true;

    // FIXME - it would be nice to also handle one-based arrays
    bool is_end() const {
        return idx[ArrayDim-SliceDim] == p->shape()[ArrayDim-SliceDim];
    }

    void inc() {
        ++idx[ArrayDim-SliceDim];
    }

    value_type deref() const {
        static_assert((sizeof(T) == 0), "cannot deref a blitz slice");
        throw std::logic_error("static assert should have been triggered by this point");
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
    BlitzIterator() : p(nullptr) { }
    BlitzIterator(
        const blitz::Array<T, ArrayDim> *_p,
        const blitz::TinyVector<int, ArrayDim> _idx
    ) : p(_p), idx(_idx) { }

    typedef T value_type;
    typedef Error_WasNotContainer subiter_type;
    static constexpr bool is_container = false;

    // FIXME - it would be nice to also handle one-based arrays
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
        static_assert((sizeof(T) == 0), "argument was not a container");
        throw std::logic_error("static assert should have been triggered by this point");
    }

private:
    const blitz::Array<T, ArrayDim> *p;
    blitz::TinyVector<int, ArrayDim> idx;
};

template <typename T, int ArrayDim>
class ArrayTraitsImpl<blitz::Array<T, ArrayDim>> : public ArrayTraitsDefaults<T> {
public:
    static constexpr bool allow_auto_unwrap = false;
    static constexpr size_t depth = ArrayTraits<T>::depth + ArrayDim;

    typedef BlitzIterator<T, ArrayDim, ArrayDim> range_type;

    static range_type get_range(const blitz::Array<T, ArrayDim> &arg) {
        blitz::TinyVector<int, ArrayDim> start_idx;
        start_idx = 0;
        return range_type(&arg, start_idx);
    }
};

} // namespace gnuplotio
#endif // GNUPLOT_BLITZ_SUPPORT_LOADED
#endif // BZ_BLITZ_H

// }}}2

// {{{2 Armadillo support

// This is outside of the main header guard so that it will be compiled when people do
// something like this:
//    #include "gnuplot-iostream.h"
//    #include <armadillo>
//    #include "gnuplot-iostream.h"
// Note that it has its own header guard to avoid double inclusion.

#ifdef ARMA_INCLUDES
#ifndef GNUPLOT_ARMADILLO_SUPPORT_LOADED
#define GNUPLOT_ARMADILLO_SUPPORT_LOADED
namespace gnuplotio {

template <typename T> static constexpr bool dont_treat_as_stl_container<arma::Row  <T>> = true;
template <typename T> static constexpr bool dont_treat_as_stl_container<arma::Col  <T>> = true;
template <typename T> static constexpr bool dont_treat_as_stl_container<arma::Mat  <T>> = true;
template <typename T> static constexpr bool dont_treat_as_stl_container<arma::Cube <T>> = true;
template <typename T> static constexpr bool dont_treat_as_stl_container<arma::field<T>> = true;

// {{{3 Cube

template <typename T>
class ArrayTraitsImpl<arma::Cube<T>> : public ArrayTraitsDefaults<T> {
    class SliceRange {
    public:
        SliceRange() : p(nullptr), col(0), slice(0) { }
        explicit SliceRange(const arma::Cube<T> *_p, size_t _row, size_t _col) :
            p(_p), row(_row), col(_col), slice(0) { }

        typedef T value_type;
        typedef Error_WasNotContainer subiter_type;
        static constexpr bool is_container = false;

        bool is_end() const { return slice == p->n_slices; }

        void inc() { ++slice; }

        value_type deref() const {
            return (*p)(row, col, slice);
        }

        subiter_type deref_subiter() const {
            static_assert((sizeof(T) == 0), "argument was not a container");
            throw std::logic_error("static assert should have been triggered by this point");
        }

    private:
        const arma::Cube<T> *p;
        size_t row, col, slice;
    };

    class ColRange {
    public:
        ColRange() : p(nullptr), row(0), col(0) { }
        explicit ColRange(const arma::Cube<T> *_p, size_t _row) :
            p(_p), row(_row), col(0) { }

        typedef T value_type;
        typedef SliceRange subiter_type;
        static constexpr bool is_container = true;

        bool is_end() const { return col == p->n_cols; }

        void inc() { ++col; }

        value_type deref() const {
            static_assert((sizeof(T) == 0), "can't call deref on an armadillo cube col");
            throw std::logic_error("static assert should have been triggered by this point");
        }

        subiter_type deref_subiter() const {
            return subiter_type(p, row, col);
        }

    private:
        const arma::Cube<T> *p;
        size_t row, col;
    };

    class RowRange {
    public:
        RowRange() : p(nullptr), row(0) { }
        explicit RowRange(const arma::Cube<T> *_p) : p(_p), row(0) { }

        typedef T value_type;
        typedef ColRange subiter_type;
        static constexpr bool is_container = true;

        bool is_end() const { return row == p->n_rows; }

        void inc() { ++row; }

        value_type deref() const {
            static_assert((sizeof(T) == 0), "can't call deref on an armadillo cube row");
            throw std::logic_error("static assert should have been triggered by this point");
        }

        subiter_type deref_subiter() const {
            return subiter_type(p, row);
        }

    private:
        const arma::Cube<T> *p;
        size_t row;
    };

public:
    static constexpr bool allow_auto_unwrap = false;
    static constexpr size_t depth = ArrayTraits<T>::depth + 3;

    typedef RowRange range_type;

    static range_type get_range(const arma::Cube<T> &arg) {
        //std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
        return range_type(&arg);
    }
};

// }}}3

// {{{3 Mat and Field

template <typename RF, typename T>
class ArrayTraits_ArmaMatOrField : public ArrayTraitsDefaults<T> {
    class ColRange {
    public:
        ColRange() : p(nullptr), row(0), col(0) { }
        explicit ColRange(const RF *_p, size_t _row) :
            p(_p), row(_row), col(0) { }

        typedef T value_type;
        typedef Error_WasNotContainer subiter_type;
        static constexpr bool is_container = false;

        bool is_end() const { return col == p->n_cols; }

        void inc() { ++col; }

        value_type deref() const {
            return (*p)(row, col);
        }

        subiter_type deref_subiter() const {
            static_assert((sizeof(T) == 0), "argument was not a container");
            throw std::logic_error("static assert should have been triggered by this point");
        }

    private:
        const RF *p;
        size_t row, col;
    };

    class RowRange {
    public:
        RowRange() : p(nullptr), row(0) { }
        explicit RowRange(const RF *_p) : p(_p), row(0) { }

        typedef T value_type;
        typedef ColRange subiter_type;
        static constexpr bool is_container = true;

        bool is_end() const { return row == p->n_rows; }

        void inc() { ++row; }

        value_type deref() const {
            static_assert((sizeof(T) == 0), "can't call deref on an armadillo matrix row");
            throw std::logic_error("static assert should have been triggered by this point");
        }

        subiter_type deref_subiter() const {
            return subiter_type(p, row);
        }

    private:
        const RF *p;
        size_t row;
    };

public:
    static constexpr bool allow_auto_unwrap = false;
    static constexpr size_t depth = ArrayTraits<T>::depth + 2;

    typedef RowRange range_type;

    static range_type get_range(const RF &arg) {
        //std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
        return range_type(&arg);
    }
};

template <typename T>
class ArrayTraitsImpl<arma::field<T>> : public ArrayTraits_ArmaMatOrField<arma::field<T>, T> { };

template <typename T>
class ArrayTraitsImpl<arma::Mat<T>> : public ArrayTraits_ArmaMatOrField<arma::Mat<T>, T> { };

// }}}3

// {{{3 Row

template <typename T>
class ArrayTraitsImpl<arma::Row<T>> : public ArrayTraitsDefaults<T> {
public:
    static constexpr bool allow_auto_unwrap = false;

    typedef IteratorRange<typename arma::Row<T>::const_iterator, T> range_type;

    static range_type get_range(const arma::Row<T> &arg) {
        //std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
        return range_type(arg.begin(), arg.end());
    }
};

// }}}3

// {{{3 Col

template <typename T>
class ArrayTraitsImpl<arma::Col<T>> : public ArrayTraitsDefaults<T> {
public:
    static constexpr bool allow_auto_unwrap = false;

    typedef IteratorRange<typename arma::Col<T>::const_iterator, T> range_type;

    static range_type get_range(const arma::Col<T> &arg) {
        //std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
        return range_type(arg.begin(), arg.end());
    }
};

// }}}3

} // namespace gnuplotio
#endif // GNUPLOT_ARMADILLO_SUPPORT_LOADED
#endif // ARMA_INCLUDES

// }}}2

// {{{2 Eigen support

// This is outside of the main header guard so that it will be compiled when people do
// something like this:
//    #include "gnuplot-iostream.h"
//    #include <Eigen/Dense>
//    #include "gnuplot-iostream.h"
// Note that it has its own header guard to avoid double inclusion.

#ifdef EIGEN_CORE_H
#ifndef GNUPLOT_EIGEN_SUPPORT_LOADED
#define GNUPLOT_EIGEN_SUPPORT_LOADED
namespace gnuplotio {

template <typename T, typename=void>
static constexpr bool is_eigen_matrix = false;

template <typename T>
static constexpr bool is_eigen_matrix<T,
    std::enable_if_t<std::is_base_of_v<Eigen::EigenBase<T>, T>>> = true;

static_assert( is_eigen_matrix<Eigen::MatrixXf>);
static_assert(!is_eigen_matrix<int>);

template <typename T>
static constexpr bool dont_treat_as_stl_container<T, typename std::enable_if_t<is_eigen_matrix<T>>> = true;

static_assert(dont_treat_as_stl_container<Eigen::MatrixXf>);

// {{{3 Matrix

template <typename RF>
class ArrayTraits_Eigen1D : public ArrayTraitsDefaults<typename RF::value_type> {
    class IdxRange {
    public:
        IdxRange() : p(nullptr), idx(0) { }
        explicit IdxRange(const RF *_p) :
            p(_p), idx(0) { }

        using value_type = typename RF::value_type;
        typedef Error_WasNotContainer subiter_type;
        static constexpr bool is_container = false;

        bool is_end() const { return idx == p->size(); }

        void inc() { ++idx; }

        value_type deref() const {
            return (*p)(idx);
        }

        subiter_type deref_subiter() const {
            static_assert((sizeof(value_type) == 0), "argument was not a container");
            throw std::logic_error("static assert should have been triggered by this point");
        }

    private:
        const RF *p;
        Eigen::Index idx;
    };

public:
    static constexpr bool allow_auto_unwrap = false;
    static constexpr size_t depth = ArrayTraits<typename RF::value_type>::depth + 1;

    typedef IdxRange range_type;

    static range_type get_range(const RF &arg) {
        //std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
        return range_type(&arg);
    }
};

template <typename RF>
class ArrayTraits_Eigen2D : public ArrayTraitsDefaults<typename RF::value_type> {
    class ColRange {
    public:
        ColRange() : p(nullptr), row(0), col(0) { }
        explicit ColRange(const RF *_p, Eigen::Index _row) :
            p(_p), row(_row), col(0) { }

        using value_type = typename RF::value_type;
        typedef Error_WasNotContainer subiter_type;
        static constexpr bool is_container = false;

        bool is_end() const { return col == p->cols(); }

        void inc() { ++col; }

        value_type deref() const {
            return (*p)(row, col);
        }

        subiter_type deref_subiter() const {
            static_assert((sizeof(value_type) == 0), "argument was not a container");
            throw std::logic_error("static assert should have been triggered by this point");
        }

    private:
        const RF *p;
        Eigen::Index row, col;
    };

    class RowRange {
    public:
        RowRange() : p(nullptr), row(0) { }
        explicit RowRange(const RF *_p) : p(_p), row(0) { }

        using value_type = typename RF::value_type;
        typedef ColRange subiter_type;
        static constexpr bool is_container = true;

        bool is_end() const { return row == p->rows(); }

        void inc() { ++row; }

        value_type deref() const {
            static_assert((sizeof(value_type) == 0), "can't call deref on an eigen matrix row");
            throw std::logic_error("static assert should have been triggered by this point");
        }

        subiter_type deref_subiter() const {
            return subiter_type(p, row);
        }

    private:
        const RF *p;
        Eigen::Index row;
    };

public:
    static constexpr bool allow_auto_unwrap = false;
    static constexpr size_t depth = ArrayTraits<typename RF::value_type>::depth + 2;

    typedef RowRange range_type;

    static range_type get_range(const RF &arg) {
        //std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
        return range_type(&arg);
    }
};

template <typename T>
class ArrayTraitsImpl<T, typename std::enable_if_t<is_eigen_matrix<T>>> :
    public std::conditional_t<
        T::RowsAtCompileTime == 1 || T::ColsAtCompileTime == 1,
        ArrayTraits_Eigen1D<T>,
        ArrayTraits_Eigen2D<T>
    > { };

// }}}3

} // namespace gnuplotio
#endif // GNUPLOT_EIGEN_SUPPORT_LOADED
#endif // EIGEN_CORE_H

// }}}2

// }}}1
