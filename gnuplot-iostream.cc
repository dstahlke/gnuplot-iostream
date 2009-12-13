#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pty.h>

#include "gnuplot-iostream.h"

Gnuplot::Gnuplot() : 
	boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
		fileno(pout = popen("gnuplot", "w"))),
	pty_fh(NULL),
	master_fd(-1),
	slave_fd(-1),
	debug_messages(false)
{ }

Gnuplot::~Gnuplot() {
	if(debug_messages) {
		std::cerr << "closing gnuplot" << std::endl;
	}

	close();
	pclose(pout);
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

// based on http://www.gnuplot.info/files/gpReadMouseTest.c
void Gnuplot::allocReader() {
	if(pty_fh) return;

	if(0 > openpty(&master_fd, &slave_fd, NULL, NULL, NULL)) {
		perror("openpty");
		throw std::runtime_error("openpty failed");
	}
	pty_fn = ttyname(slave_fd);
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
