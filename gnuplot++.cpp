#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pty.h>

#include "gnuplot++.h"

//// Gnuplot /////

Gnuplot::Gnuplot() : 
	boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
		fileno(pout = popen("gnuplot", "w"))),
	pty_fn(0)
{ }

Gnuplot::~Gnuplot() {
	std::cerr << "closing gnuplot" << std::endl;

	close();
	pclose(pout);
}

void Gnuplot::getMouse(double &mx, double &my, int &mb) {
	allocReader();
	*this << "pause mouse \"Click mouse!\\n\"" << std::endl;
	*this << "print MOUSE_X, MOUSE_Y, MOUSE_BUTTON" << std::endl;
	std::cerr << "begin scanf" << std::endl;
	if(3 != fscanf(pty_fh, "%lf %lf %d", &mx, &my, &mb)) {
		throw "could not parse reply";
	}
	std::cerr << "end scanf" << std::endl;
}

// based on http://www.gnuplot.info/files/gpReadMouseTest.c
void Gnuplot::allocReader() {
	if(pty_fn) return;

	openpty(&master_fd, &slave_fd, NULL, NULL, NULL);
	pty_fn = strdup(ttyname(slave_fd));
	printf("fn=%s\n", pty_fn);

	// disable echo
	struct termios tios;
	if(tcgetattr(slave_fd, &tios) < 0) {
		perror("tcgetattr");
		exit(1);
	}
	tios.c_lflag &= ~(ECHO | ECHONL);
	if(tcsetattr(slave_fd, TCSAFLUSH, &tios) < 0) {
		perror("tcsetattr");
		exit(1);
	}

	// FIXME - close everything on destruction
	pty_fh = fdopen(master_fd, "r");
	assert(pty_fh);

	*this << "set mouse; set print \"" << pty_fn << "\"" << std::endl;
}
