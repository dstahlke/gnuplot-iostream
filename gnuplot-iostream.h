#ifndef GNUPLOT_IOSTREAM_H
#define GNUPLOT_IOSTREAM_H

#include <boost/noncopyable.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fstream>
#include <iostream>
#include <utility>
#include <string>

#include <stdio.h>

#ifdef GNUPLOT_ENABLE_BLITZ
#include <blitz/array.h>
#endif

class Gnuplot : public boost::iostreams::stream<
	boost::iostreams::file_descriptor_sink>, private boost::noncopyable
{
public:
	Gnuplot();
	~Gnuplot();

	void getMouse(double &mx, double &my, int &mb);

	template <class T>
	Gnuplot &send(T p, T last) {
		while(p != last) {
			sendEntry(*p);
			*this << "\n";
			++p;
		}
		*this << "e" << std::endl;
		return *this;
	}

#ifdef GNUPLOT_ENABLE_BLITZ
	template <class T>
	Gnuplot &send(const blitz::Array<T, 1> &a) {
		typename blitz::Array<T, 1>::iterator 
			p = a.begin(), p_end = a.end();
		while(p != p_end) {
			sendEntry(*p);
			*this << "\n";
			++p;
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <class T>
	Gnuplot &send(const blitz::Array<T, 2> &a) {
		for(int i=a.lbound(0); i<=a.ubound(0); i++) {
			for(int j=a.lbound(1); j<=a.ubound(1); j++) {
				sendEntry(a(i, j));
				*this << "\n";
			}
			*this << "\n";
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <class T, int N>
	Gnuplot &send(const blitz::Array<blitz::TinyVector<T, N>, 1> &a) {
		typename blitz::Array<blitz::TinyVector<T, N>, 1>::iterator 
			p = a.begin(), p_end = a.end();
		while(p != p_end) {
			for(int i=0; i<N; i++) {
				sendEntry((*p)[i]);
			}
			*this << "\n";
			++p;
		}
		*this << "e" << std::endl;
		return *this;
	}

	template <class T, int N>
	Gnuplot &send(const blitz::Array<blitz::TinyVector<T,N>, 2> &a) {
		for(int i=a.lbound(0); i<=a.ubound(0); i++) {
			for(int j=a.lbound(1); j<=a.ubound(1); j++) {
				for(int k=0; k<N; k++) {
					sendEntry(a(i,j)[k]);
				}
				*this << "\n";
			}
			*this << "\n";
		}
		*this << "e" << std::endl;
		return *this;
	}
#endif // GNUPLOT_ENABLE_BLITZ

private:
	template <class T>
	void sendEntry(T v) {
		*this << v << " ";
	}

	template <class T, class U>
	void sendEntry(std::pair<T, U> v) {
		sendEntry(v.first);
		sendEntry(v.second);
	}

	void allocReader();

private:
	FILE *pout;
	std::string pty_fn;
	FILE *pty_fh;
	int master_fd, slave_fd;

public:
	bool debug_messages;
};

#endif // GNUPLOT_IOSTREAM_H
