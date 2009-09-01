#include <blitz/array.h>
#include <stdio.h>
#include <boost/noncopyable.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fstream>
#include <iostream>

class Gnuplot : public boost::iostreams::stream<
	boost::iostreams::file_descriptor_sink>, private boost::noncopyable
{
public:
	Gnuplot();
	~Gnuplot();

	void getMouse(double &mx, double &my, int &mb);

	// FIXME - avoid cast to double for print

	template <class T>
	Gnuplot &send(blitz::Array<T, 1> &a) {
		typename blitz::Array<T, 1>::iterator 
			p = a.begin(), p_end = a.end();
		while(p != p_end) {
			*this << boost::format("%.18g\n") % double(*p);
			p++;
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <class T>
	Gnuplot &send(blitz::Array<T, 2> &a) {
		// FIXME - use upper/lower bound functions
		for(int i=0; i<a.shape()[0]; i++) {
			for(int j=0; j<a.shape()[1]; j++) {
				*this << boost::format("%.18g\n") % double(a(i,j));
			}
			*this << "\n";
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <class T, int N>
	Gnuplot &send(blitz::Array<blitz::TinyVector<T, N>, 1> &a) {
		typename blitz::Array<blitz::TinyVector<T, N>, 1>::iterator 
			p = a.begin(), p_end = a.end();
		while(p != p_end) {
			for(int i=0; i<N; i++) {
				*this << boost::format("%.18g ") % double((*p)[i]);
			}
			*this << "\n";
			p++;
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <class T, int N>
	Gnuplot &send(blitz::Array<blitz::TinyVector<T,N>, 2> &a) {
		// FIXME - use upper/lower bound functions
		for(int i=0; i<a.shape()[0]; i++) {
			for(int j=0; j<a.shape()[1]; j++) {
				for(int k=0; k<N; k++) {
					*this << boost::format("%.18g ") % double(a(i,j)[k]);
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
