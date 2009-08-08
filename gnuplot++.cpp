// g++ -o zz gnuplot++.cpp -Wall -I/usr/lib64/blitz/include -O0 -g

#include <blitz/array.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

class Gnuplot {
public:
	Gnuplot() : gh(0) { reassign(new GnuplotHandle()); }

	Gnuplot(const Gnuplot &right) : gh(0) { reassign(right.gh); }

	~Gnuplot() { reassign(0); }

	void getMouse(float &mx, float &my, int &mb) {
		gh->allocReader();
    	operator <<("pause mouse \"Click mouse!\\n\"");
		operator <<("print MOUSE_X, MOUSE_Y, MOUSE_BUTTON");
		printf("begin scanf\n");
		if(3 != fscanf(gh->fh_read, "%f %f %d", &mx, &my, &mb)) {
			throw "could not parse reply";
		}
	}

	Gnuplot &operator <<(const char *cmd) {
		fputs(cmd, gh->fh);
		fputs("\n", gh->fh);
		fflush(gh->fh);
		return *this;
	}

	template <int N>
	Gnuplot &operator <<(blitz::Array<blitz::TinyVector<double,N>, 1> &a) {
		typename blitz::Array<blitz::TinyVector<double,N>, 1>::iterator 
			p = a.begin(), p_end = a.end();
		while(p != p_end) {
			for(int i=0; i<N; i++) {
				fprintf(gh->fh, "%.18g ", (*p)[i]);
			}
			fputs("\n", gh->fh);
			p++;
		}
		fputs("e\n", gh->fh);
		fflush(gh->fh);
		return *this;
	}

	Gnuplot &operator <<(blitz::Array<double, 1> &a) {
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

private:
	class GnuplotHandle {
	public:
		// apparently there is no equivalent of
		// popen that uses iostreams
		FILE *fh;
		FILE *fh_read;
		const char *fifo_fn;
		int refcnt;

		GnuplotHandle() : fh_read(0), fifo_fn(0) {
			printf("opening gnuplot\n");
			fh = popen("gnuplot", "w");
			assert(fh);

			printf("constructed\n");
			refcnt = 0;
		}

		~GnuplotHandle() {
			printf("closing gnuplot\n");
			pclose(fh);
			if(fifo_fn) {
				printf("removing pipe\n");
				unlink(fifo_fn);
			}
		}

		// based on http://www.gnuplot.info/files/gpReadMouseTest.c
		void allocReader() {
			if(fh_read) return;
			fifo_fn = "./gp_pipe"; // FIXME - need unique name

			printf("make pipe\n");
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
	};

	void reassign(GnuplotHandle *np) {
		if(np) np->refcnt++;
		if(gh) {
			gh->refcnt--;
			if(gh->refcnt == 0) delete gh;
		}
		gh = np;
	}

	GnuplotHandle *gh;
};

int main() {
	Gnuplot gp;
	blitz::Array<blitz::TinyVector<double, 2>, 1> arr(10);
	blitz::firstIndex i;
	arr[0] = i*i;
	arr[1] = i*i*i;
	blitz::Array<double, 1> a0(arr[0]);
	blitz::Array<double, 1> a1(arr[1]);
	//gp << "p '-' w lp, '-' w lp" << arr[0] << arr[1];
	gp << "p '-' w lp, '-' w lp" << a0 << a1;

	float mx, my;
	int mb;
	for(int i=0; i<3; i++) {
		gp.getMouse(mx, my, mb);
		printf("You pressed mouse button %d at x=%f y=%f\n", mb, mx, my);
	}
}
