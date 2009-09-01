#include <blitz/array.h>
#include <stdio.h>
#include <boost/noncopyable.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fstream>
#include <iostream>

// FIXME
using namespace boost::iostreams ;

class Gnuplot : public stream<file_descriptor_sink>, private boost::noncopyable
{
public:
	Gnuplot();
	~Gnuplot();

	void getMouse(float &mx, float &my, int &mb);

	Gnuplot &operator <<(blitz::Array<double, 1> &a);

	Gnuplot &operator <<(blitz::Array<double, 2> &a);

	template <int N>
	Gnuplot &operator <<(blitz::Array<blitz::TinyVector<double,N>, 1> &a) {
		typename blitz::Array<blitz::TinyVector<double,N>, 1>::iterator 
			p = a.begin(), p_end = a.end();
		while(p != p_end) {
			for(int i=0; i<N; i++) {
				*this << boost::format("%.18g ") % (*p)[i];
			}
			*this << "\n";
			p++;
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <int N>
	Gnuplot &operator <<(blitz::Array<blitz::TinyVector<double,N>, 2> &a) {
		// FIXME - use upper/lower bound functions
		for(int i=0; i<a.shape()[0]; i++) {
			for(int j=0; j<a.shape()[1]; j++) {
				for(int k=0; k<N; k++) {
					*this << boost::format("%.18g ") % a(i,j)[k];
				}
				*this << "\n";
			}
			*this << "\n";
		}
		*this << "e" << std::endl;
		return *this;
	}

private:
	void allocReader();

	FILE *pout;
	const char *pty_fn;
	FILE *pty_fh;
	int master_fd, slave_fd;
};
