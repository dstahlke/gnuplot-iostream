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

#ifndef GNUPLOT_IOSTREAM_H
#define GNUPLOT_IOSTREAM_H

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
#include "boost/iostreams/device/file_descriptor.hpp"
#include "boost/iostreams/stream.hpp"
#include "boost/version.hpp"
#include "boost/utility.hpp"
#ifdef GNUPLOT_ENABLE_BLITZ
#include "blitz/array.h"
#endif

// This is the version of boost which has v3 of the filesystem libraries by default.
#if BOOST_VERSION >= 104600
#define GNUPLOT_USE_TMPFILE
#include "boost/filesystem.hpp"
#endif // BOOST_VERSION

// Patch for Windows by Damien Loison
#ifdef WIN32
#define PCLOSE _pclose
#define POPEN  _popen
#define FILENO _fileno
#else
#define PCLOSE pclose
#define POPEN  popen
#define FILENO fileno
#endif

///////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////

template <class T>
class GnuplotEntry {
public:
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

///////////////////////////////////////////////////////////

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

template <class T>
class GnuplotArrayWriter : public GnuplotArrayWriterBase {
public:
	void send(const T &arr) {
		sendIter(arr.begin(), arr.end());
	}
};

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

///////////////////////////////////////////////////////////

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
		make_array_writer<T>(this).send(arg);
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
	std::string binfmt(const T &arg) {
		return make_array_writer<T>(this).binfmt(arg);
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
		make_array_writer<T>(&tmp_stream).send(arg);
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' ";
		return cmdline.str();
	}

	// NOTE: empty filename makes temporary file
	template <class T>
	std::string binaryFile(T arg, std::string filename="") {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out | std::fstream::binary);
		make_array_writer<T>(&tmp_stream).sendBinary(arg);
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

///////////////////////////////////////////////////////////

#endif // GNUPLOT_IOSTREAM_H
