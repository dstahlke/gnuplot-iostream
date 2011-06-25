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
#include <stdexcept>
#include <string>
#include <utility>
#include <iomanip>

// library includes: double quotes make cpplint not complain
#include "boost/iostreams/device/file_descriptor.hpp"
#include "boost/iostreams/stream.hpp"
#include "boost/noncopyable.hpp"
#ifdef GNUPLOT_ENABLE_BLITZ
#include "blitz/array.h"
#endif

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

#ifdef GNUPLOT_ENABLE_PTY
// this is a private class
class GnuplotPty {
public:
	explicit GnuplotPty(bool debug_messages);
	~GnuplotPty();

	std::string pty_fn;
	FILE *pty_fh;
	int master_fd, slave_fd;
};

GnuplotPty::GnuplotPty(bool debug_messages) :
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
		std::cerr << "fn=" << pty_fn << std::endl;
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

GnuplotPty::~GnuplotPty() {
	if(pty_fh) fclose(pty_fh);
	if(master_fd > 0) ::close(master_fd);
	if(slave_fd  > 0) ::close(slave_fd);
}
#else // GNUPLOT_ENABLE_PTY
class GnuplotPty { };
#endif // GNUPLOT_ENABLE_PTY

///////////////////////////////////////////////////////////

class Gnuplot : public boost::iostreams::stream<
	boost::iostreams::file_descriptor_sink>, private boost::noncopyable
{
public:
	explicit Gnuplot(const std::string cmd = "gnuplot");
	~Gnuplot();

#ifdef GNUPLOT_ENABLE_PTY
	void getMouse(
		double &mx, double &my, int &mb,
		std::string msg="Click Mouse!"
	);
#endif // GNUPLOT_ENABLE_PTY

	// used for one STL container
	template <class T>
	Gnuplot &send(T p, T last) {
		while(p != last) {
			sendEntry(*p);
			*this << "\n";
			++p;
		}
		*this << "e" << std::endl;
		return *this;
	}

	// used for two STL containers
	template <class T, class U>
	Gnuplot &send(T x, T x_last, U y, U y_last) {
		while(x != x_last && y != y_last) {
			sendEntry(*x, *y);
			*this << "\n";
			++x;
			++y;
		}
		// assert inputs same size
		assert(x==x_last && y==y_last);
		*this << "e" << std::endl;
		return *this;
	}

	// this handles STL containers as well as blitz::Array<T, 1> and
	// blitz::Array<blitz::TinyVector<T, N>, 1>
	template <class Iter>
	Gnuplot &send(Iter arr) {
		send(arr.begin(), arr.end());
		return *this;
	}

#ifdef GNUPLOT_ENABLE_BLITZ
	// Note: T could be either a scalar or a blitz::TinyVector.
	template <class T>
	Gnuplot &send(const blitz::Array<T, 2> &a) {
		for(int i=a.lbound(0); i<=a.ubound(0); i++) {
			for(int j=a.lbound(1); j<=a.ubound(1); j++) {
				sendEntry(a(i, j));
				*this << "\n";
			}
			*this << "\n";
		}
		*this << "e" << std::endl;
		return *this;
	}

private:
	template <class T, int N>
	void sendEntry(blitz::TinyVector<T, N> v) {
		for(int i=0; i<N; i++) {
			sendEntry(v[i]);
		}
	}
#endif // GNUPLOT_ENABLE_BLITZ

private:
	template <class T>
	void sendEntry(T v) {
		*this << v << " ";
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

#ifdef GNUPLOT_ENABLE_PTY
	void allocPty() {
		if(!gp_pty) {
			gp_pty = new GnuplotPty(debug_messages);
		}
	}
#endif // GNUPLOT_ENABLE_PTY

private:
	FILE *pout;
	// this is included even in the absense of GNUPLOT_ENABLE_PTY, to
	// protect binary compatibility
	GnuplotPty *gp_pty;

public:
	bool debug_messages;
};

Gnuplot::Gnuplot(const std::string cmd) : 
	boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
		FILENO(pout = POPEN(cmd.c_str(), "w")),
		boost::iostreams::never_close_handle),
	pout(pout), // keeps '-Weff++' quiet
	gp_pty(NULL),
	debug_messages(false)
{
	*this << std::scientific << std::setprecision(18);  // refer <iomanip>
}

Gnuplot::~Gnuplot() {
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

	if(PCLOSE(pout)) {
		std::cerr << "pclose returned error" << std::endl;
	}

	if(gp_pty) delete(gp_pty);
}

#ifdef GNUPLOT_ENABLE_PTY
void Gnuplot::getMouse(
	double &mx, double &my, int &mb,
	std::string msg
) {
	allocPty();

	*this << "set mouse; set print \"" << gp_pty->pty_fn << "\"" << std::endl;
	*this << "pause mouse \"" << msg << "\\n\"" << std::endl;
	*this << "print MOUSE_X, MOUSE_Y, MOUSE_BUTTON" << std::endl;
	if(debug_messages) {
		std::cerr << "begin scanf" << std::endl;
	}
	if(3 != fscanf(gp_pty->pty_fh, "%lf %lf %d", &mx, &my, &mb)) {
		throw std::runtime_error("could not parse reply");
	}
	if(debug_messages) {
		std::cerr << "end scanf" << std::endl;
	}
}
#endif // GNUPLOT_ENABLE_PTY

#endif // GNUPLOT_IOSTREAM_H
