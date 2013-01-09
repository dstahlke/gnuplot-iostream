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

// This is for sending array data to gnuplot directly or via a file.
class GnuplotWriter {
public:
	explicit GnuplotWriter(std::ostream *_stream, bool _send_e=true) :
		stream(_stream),
		send_e(_send_e)
	{ }

private:
	template <class T>
	void sendEntry(T v) {
		*stream << v << " ";
	}

	template <class T, class U>
	void sendEntry(std::pair<T, U> v) {
		sendEntry(v.first, v.second);
	}

	template <class T, class U>
	void sendEntry(T t, U u) {
		sendEntry(t);
		sendEntry(u);
	}

	std::string formatCode(   float *) { return "%float"; }
	std::string formatCode(  double *) { return "%double"; }
	std::string formatCode(  int8_t *) { return "%int8"; }
	std::string formatCode( uint8_t *) { return "%uint8"; }
	std::string formatCode( int16_t *) { return "%int16"; }
	std::string formatCode(uint16_t *) { return "%uint16"; }
	std::string formatCode( int32_t *) { return "%int32"; }
	std::string formatCode(uint32_t *) { return "%uint32"; }
	std::string formatCode( int64_t *) { return "%int64"; }
	std::string formatCode(uint64_t *) { return "%uint64"; }

public:
	// used for one STL container
	template <class T>
	void sendIter(T p, T last) {
		while(p != last) {
			sendEntry(*p);
			*stream << "\n";
			++p;
		}
		if(send_e) {
			*stream << "e" << std::endl; // gnuplot's "end of array" token
		}
	}

	// used for two STL containers
	template <class T, class U>
	void sendIterPair(T x, T x_last, U y, U y_last) {
		while(x != x_last && y != y_last) {
			sendEntry(*x, *y);
			*stream << "\n";
			++x;
			++y;
		}
		// assert inputs same size
		assert(x==x_last && y==y_last);
		if(send_e) {
			*stream << "e" << std::endl; // gnuplot's "end of array" token
		}
	}

	// this handles STL containers as well as blitz::Array<T, 1> and
	// blitz::Array<blitz::TinyVector<T, N>, 1>
	template <class T>
	void send(T arr) {
		sendIter(arr.begin(), arr.end());
	}

	template <class T>
	void sendBinary(const std::vector<T> &arr) {
		stream->write(reinterpret_cast<const char *>(&arr[0]), arr.size() * sizeof(T));
	}

	template <class T>
	std::string binfmt(const std::vector<T> &arr) {
		std::ostringstream tmp;
		tmp << " format='" << formatCode((T*)NULL) << "'";
		tmp << " array=(" << arr.size() << ")";
		tmp << " ";
		return tmp.str();
	}

	// send vector of vectors containing data points
	template <class T>
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
		if(send_e) {
			*stream << "e" << std::endl; // gnuplot's "end of array" token
		}
	}

	template <class T>
	std::string binfmt(const std::vector<std::vector<T> > &arr) {
		assert(arr.size() > 0);
		std::ostringstream tmp;
		tmp << " format='";
		for(size_t i=0; i<arr.size(); i++) {
			tmp << formatCode((T*)NULL);
		}
		tmp << "' array=(" << arr[0].size() << ")";
		tmp << " ";
		return tmp.str();
	}

	// send vector of vectors containing data points
	template <class T>
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

#ifdef GNUPLOT_ENABLE_BLITZ
	// Note: T could be either a scalar or a blitz::TinyVector.
	template <class T>
	void send(const blitz::Array<T, 2> &a) {
		for(int i=a.lbound(0); i<=a.ubound(0); i++) {
			for(int j=a.lbound(1); j<=a.ubound(1); j++) {
				sendEntry(a(i, j));
				*stream << "\n";
			}
			*stream << "\n"; // blank line between rows
		}
		if(send_e) {
			*stream << "e" << std::endl; // gnuplot's "end of array" token
		}
	}

	template <class T, int d>
	void sendBinary(const blitz::Array<T, d> &arr) {
		stream->write(reinterpret_cast<const char *>(arr.data()), arr.size() * sizeof(T));
	}

	template <class T>
	std::string binfmt(const blitz::Array<T, 2> &arr) {
		std::ostringstream tmp;
		tmp << " format='" << formatCode((T*)NULL) << "'";
		tmp << " array=(" << arr.extent(0) << "," << arr.extent(1) << ")";
		if(arr.isMajorRank(0)) tmp << "scan=yx"; // i.e. C-style ordering
		tmp << " ";
		return tmp.str();
	}

private:
	template <class T, int N>
	void sendEntry(blitz::TinyVector<T, N> v) {
		for(int i=0; i<N; i++) {
			sendEntry(v[i]);
		}
	}

	template <class T, int N>
	std::string formatCode(blitz::TinyVector<T, N> *) {
		std::ostringstream tmp;
		for(int i=0; i<N; i++) {
			tmp << formatCode((T*)NULL);
		}
		return tmp.str();
	}
#endif // GNUPLOT_ENABLE_BLITZ

private:
	std::ostream *stream;
	bool send_e;
};

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
		writer(this),
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
		writer(this),
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

public:
	// These next few methods just pass through to the underlying GnuplotWriter object, which
	// in turn writes to the iostream.

	template <class T1>
	Gnuplot &send(T1 arg1) {
		writer.send(arg1);
		return *this;
	}

	// Handle fixed length C style arrays.
	// Ideally, this specialization would be done in GnuplotWriter and called from the previous
	// generic send(T1) function.  However, that way doesn't seem to compile.
	template <typename T, std::size_t N>
	Gnuplot &send(T (&arr)[N]) {
		writer.sendIter(arr, arr+N);
		return *this;
	}

	template <class T1, class T2>
	Gnuplot &send(T1 arg1, T2 arg2) {
		writer.sendIter(arg1, arg2);
		return *this;
	}

	template <class T1, class T2, class T3, class T4>
	Gnuplot &send(T1 arg1, T2 arg2, T3 arg3, T4 arg4) {
		writer.sendIterPair(arg1, arg2, arg3, arg4);
		return *this;
	}

	template <class T1>
	Gnuplot &sendBinary(T1 arg1) {
		writer.sendBinary(arg1);
		return *this;
	}

	template <class T>
	std::string binfmt(T arg) {
		return writer.binfmt(arg);
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
	template <class T1>
	std::string file(T1 arg1, std::string filename="") {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out);
		GnuplotWriter tmp_writer(&tmp_stream, false);
		tmp_writer.send(arg1);
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' ";
		return cmdline.str();
	}

	// NOTE: empty filename makes temporary file
	template <class T1>
	std::string binaryFile(T1 arg1, std::string filename="") {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out | std::fstream::binary);
		GnuplotWriter tmp_writer(&tmp_stream);
		tmp_writer.sendBinary(arg1);
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' binary" << binfmt(arg1);
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
	GnuplotWriter writer;
#ifdef GNUPLOT_USE_TMPFILE
	std::vector<boost::shared_ptr<GnuplotTmpfile> > tmp_files;
#else
	// just a placeholder
	std::vector<int> tmp_files;
#endif // GNUPLOT_USE_TMPFILE

public:
	bool debug_messages;
};

#endif // GNUPLOT_IOSTREAM_H
