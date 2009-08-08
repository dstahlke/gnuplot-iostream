#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "gnuplot++.h"

//// Gnuplot /////

Gnuplot::Gnuplot() : gh(0) { reassign(new Gnuplot::GnuplotHandle()); }

Gnuplot::Gnuplot(const Gnuplot &right) : gh(0) { reassign(right.gh); }

Gnuplot::~Gnuplot() { reassign(0); }

void Gnuplot::getMouse(float &mx, float &my, int &mb) {
	gh->allocReader();
	operator <<("pause mouse \"Click mouse!\\n\"");
	operator <<("print MOUSE_X, MOUSE_Y, MOUSE_BUTTON");
	printf("begin scanf\n");
	if(3 != fscanf(gh->fh_read, "%f %f %d", &mx, &my, &mb)) {
		throw "could not parse reply";
	}
}

Gnuplot &Gnuplot::operator <<(const char *cmd) {
	fputs(cmd, gh->fh);
	fputs("\n", gh->fh);
	fflush(gh->fh);
	return *this;
}

Gnuplot &Gnuplot::operator <<(blitz::Array<double, 1> &a) {
	blitz::Array<double, 1>::iterator 
		p = a.begin(), p_end = a.end();
	while(p != p_end) {
		fprintf(gh->fh, "%.18g\n", *p);
		p++;
	}
	fputs("e\n", gh->fh);
	fflush(gh->fh);
	return *this;
}

Gnuplot &Gnuplot::operator <<(blitz::Array<double, 2> &a) {
	for(int i=0; i<a.shape()[0]; i++) {
		for(int j=0; j<a.shape()[1]; j++) {
			fprintf(gh->fh, "%.18g\n", a(i,j));
		}
		fputs("\n", gh->fh);
	}
	fputs("e\n", gh->fh);
	fflush(gh->fh);
	return *this;
}

void Gnuplot::reassign(Gnuplot::GnuplotHandle *np) {
	if(np) np->refcnt++;
	if(gh) {
		gh->refcnt--;
		if(gh->refcnt == 0) delete gh;
	}
	gh = np;
}

//// Gnuplot::GnuplotHandle /////

Gnuplot::GnuplotHandle::GnuplotHandle() : fh_read(0), fifo_fn(0) {
	printf("opening gnuplot\n");
	fh = popen("gnuplot", "w");
	assert(fh);

	printf("constructed\n");
	refcnt = 0;
}

Gnuplot::GnuplotHandle::~GnuplotHandle() {
	printf("closing gnuplot\n");
	pclose(fh);
	if(fifo_fn) {
		printf("removing pipe\n");
		unlink(fifo_fn);
	}
}

// based on http://www.gnuplot.info/files/gpReadMouseTest.c
void Gnuplot::GnuplotHandle::allocReader() {
	if(fh_read) return;
	fifo_fn = "./gp_pipe"; // FIXME - need unique name

	printf("make pipe\n");
	unlink(fifo_fn);
	if(mkfifo(fifo_fn, 0600)) {
		if(errno != EEXIST) {
			printf("fail\n");
			perror(fifo_fn);
			unlink(fifo_fn);
			throw "cannot create fifo";
		}
	}

	fprintf(fh, "set mouse; set print \"%s\"\n", fifo_fn);
	fflush(fh);

	printf("open pipe\n");
	fh_read = fopen(fifo_fn,"r");
	if(!fh_read) {
		printf("fail\n");
		perror(fifo_fn);
		unlink(fifo_fn);
		throw "cannot open fifo";
	}

	printf("pipe opened\n");
}
