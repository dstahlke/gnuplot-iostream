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

#include <stdexcept>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pty.h>

#include "gnuplot-iostream.h"

Gnuplot::Gnuplot(std::string cmd) : 
	boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
		fileno(pout = popen(cmd.c_str(), "w"))),
	pty_fh(NULL),
	master_fd(-1),
	slave_fd(-1),
	debug_messages(false)
{
	setf(std::ios::scientific, std::ios::floatfield);
	precision(18);
}

Gnuplot::~Gnuplot() {
	if(debug_messages) {
		std::cerr << "closing gnuplot" << std::endl;
	}

	// FIXME - boost's close method calls close() on the file descriptor, but
	// we need to use pclose instead.  For now, just skip calling boost's close
	// and use flush just in case.
	flush();
	//close();

	if(pclose(pout)) {
		std::cerr << "pclose returned error" << std::endl;
	}

	if(pty_fh) fclose(pty_fh);
	if(master_fd > 0) ::close(master_fd);
	if(slave_fd  > 0) ::close(slave_fd);
}

void Gnuplot::getMouse(double &mx, double &my, int &mb) {
	allocReader();
	*this << "pause mouse \"Click mouse!\\n\"" << std::endl;
	*this << "print MOUSE_X, MOUSE_Y, MOUSE_BUTTON" << std::endl;
	if(debug_messages) {
		std::cerr << "begin scanf" << std::endl;
	}
	if(3 != fscanf(pty_fh, "%lf %lf %d", &mx, &my, &mb)) {
		throw std::runtime_error("could not parse reply");
	}
	if(debug_messages) {
		std::cerr << "end scanf" << std::endl;
	}
}

// adapted from http://www.gnuplot.info/files/gpReadMouseTest.c
void Gnuplot::allocReader() {
	if(pty_fh) return;

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

	*this << "set mouse; set print \"" << pty_fn << "\"" << std::endl;
}
