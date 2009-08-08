#include <blitz/array.h>
#include <stdio.h>

class Gnuplot {
public:
	Gnuplot();
	Gnuplot(const Gnuplot &right);
	~Gnuplot();
	void getMouse(float &mx, float &my, int &mb);
	Gnuplot &operator <<(const char *cmd);

	Gnuplot &operator <<(blitz::Array<double, 1> &a);

	Gnuplot &operator <<(blitz::Array<double, 2> &a);

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

	template <int N>
	Gnuplot &operator <<(blitz::Array<blitz::TinyVector<double,N>, 2> &a) {
		for(int i=0; i<a.shape()[0]; i++) {
			for(int j=0; j<a.shape()[1]; j++) {
				for(int k=0; k<N; k++) {
					fprintf(gh->fh, "%.18g ", a(i,j)[k]);
				}
				fputs("\n", gh->fh);
			}
			fputs("\n", gh->fh);
		}
		fputs("e\n", gh->fh);
		fflush(gh->fh);
		return *this;
	}

private:
	class GnuplotHandle {
	public:
		FILE *fh;
		FILE *fh_read;
		const char *fifo_fn;
		int refcnt;

		GnuplotHandle();
		~GnuplotHandle();
		void allocReader();
	};

	void reassign(GnuplotHandle *np);
	GnuplotHandle *gh;
};
