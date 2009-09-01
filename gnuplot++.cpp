#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pty.h>

#include "gnuplot++.h"

//// Gnuplot /////

Gnuplot::Gnuplot() : 
	stream<file_descriptor_sink>(fileno(pout = popen("gnuplot", "w"))),
	pty_fn(0)
{ }

Gnuplot::~Gnuplot() {
	std::cerr << "closing gnuplot" << std::endl;

	close();
	pclose(pout);
}

void Gnuplot::getMouse(float &mx, float &my, int &mb) {
	allocReader();
	*this << "pause mouse \"Click mouse!\\n\"" << std::endl;
	*this << "print MOUSE_X, MOUSE_Y, MOUSE_BUTTON" << std::endl;
	std::cerr << "begin scanf" << std::endl;
	if(3 != fscanf(pty_fh, "%f %f %d", &mx, &my, &mb)) {
		throw "could not parse reply";
	}
	std::cerr << "end scanf" << std::endl;
}

Gnuplot &Gnuplot::operator <<(blitz::Array<double, 1> &a) {
	blitz::Array<double, 1>::iterator 
		p = a.begin(), p_end = a.end();
	while(p != p_end) {
		*this << boost::format("%.18g\n") % (*p);
		p++;
	}
	*this << "e" << std::endl;
	return *this;
}

Gnuplot &Gnuplot::operator <<(blitz::Array<double, 2> &a) {
	// FIXME - use upper/lower bound functions
	for(int i=0; i<a.shape()[0]; i++) {
		for(int j=0; j<a.shape()[1]; j++) {
			*this << boost::format("%.18g\n") % a(i,j);
		}
		*this << "\n";
	}
	*this << "e" << std::endl;
	return *this;
}

// based on http://www.gnuplot.info/files/gpReadMouseTest.c
void Gnuplot::allocReader() {
//	if(fh_read) return;
//	fifo_fn = "./gp_pipe"; // FIXME - need unique name
//
//	std::cerr << "make pipe" << std::endl;
//	unlink(fifo_fn);
//	if(mkfifo(fifo_fn, 0600)) {
//		if(errno != EEXIST) {
//			std::cerr << "fail" << std::endl;
//			perror(fifo_fn);
//			unlink(fifo_fn);
//			throw "cannot create fifo";
//		}
//	}
//
//	*this << "set mouse; set print \"%s\"" << std::endl;
//
//	std::cerr << "open pipe" << std::endl;
//	fh_read = fopen(fifo_fn,"r");
//	if(!fh_read) {
//		std::cerr << "fail" << std::endl;
//		perror(fifo_fn);
//		unlink(fifo_fn);
//		throw "cannot open fifo";
//	}
//
//	std::cerr << "pipe opened" << std::endl;

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
