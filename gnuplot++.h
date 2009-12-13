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

	template <class T>
	Gnuplot &send(blitz::Array<T, 1> &a) {
		typename blitz::Array<T, 1>::iterator 
			p = a.begin(), p_end = a.end();
		while(p != p_end) {
			*this << boost::format("%.18g\n") % (*p);
			p++;
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <class T>
	Gnuplot &send(blitz::Array<T, 2> &a) {
		for(int i=a.lbound(0); i<=a.ubound(0); i++) {
			for(int j=a.lbound(1); j<=a.ubound(1); j++) {
				*this << boost::format("%.18g\n") % a(i,j);
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
				*this << boost::format("%.18g ") % (*p)[i];
			}
			*this << "\n";
			p++;
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <class T, int N>
	Gnuplot &send(blitz::Array<blitz::TinyVector<T,N>, 2> &a) {
		for(int i=a.lbound(0); i<=a.ubound(0); i++) {
			for(int j=a.lbound(1); j<=a.ubound(1); j++) {
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

private:
	FILE *pout;
	std::string pty_fn;
	FILE *pty_fh;
	int master_fd, slave_fd;

public:
	bool debug_messages;
};
