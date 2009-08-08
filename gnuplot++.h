#include <blitz/array.h>
#include <stdio.h>

class Gnuplot {
public:
	Gnuplot():
	Gnuplot(const Gnuplot &right):
	~Gnuplot();
	void getMouse(float &mx, float &my, int &mb);
	Gnuplot &operator <<(const char *cmd);

	template <int N>
	Gnuplot &operator <<(blitz::Array<blitz::TinyVector<double,N>, 1> &a);

	Gnuplot &operator <<(blitz::Array<double, 1> &a);

private:
	class GnuplotHandle {
	public:
		FILE *fh;
		FILE *fh_read;
		const char *fifo_fn;
		int refcnt;

		GnuplotHandle():
		~GnuplotHandle();
		void allocReader();
	};

	void reassign(GnuplotHandle *np);
	GnuplotHandle *gh;
};
