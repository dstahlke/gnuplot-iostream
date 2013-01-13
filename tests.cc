/*
Copyright (c) 2009 Daniel Stahlke

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <fstream>
#include <vector>
#include <math.h>

#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/and.hpp>
#include "meta_util.h"

#include "gnuplot-iostream.h"

// from FireBreath
//    typedef boost::mpl::vector
//    <
//        std::string, 
//        std::wstring
//    > pseudo_container_types;
//
//    template<typename T>
//    struct plain_type {
//        typedef typename boost::remove_const<typename boost::remove_reference<T>::type>::type type;
//    };
//
//    template<class T>
//    struct is_pseudo_container 
//      : boost::mpl::contains<pseudo_container_types, typename plain_type<T>::type>::type
//    {};
//
//    template<bool isClass, class T>
//    struct is_container_helper
//      : boost::mpl::and_< 
//            boost::mpl::not_< is_pseudo_container<T> >,
//            typename is_container_impl<T>::type >::type
//    {};
//
//    template<class T>
//    struct is_container_helper<false, T> 
//      : boost::mpl::false_
//    {};
//
//    template<class T>
//    struct is_container
//      : is_container_helper<boost::is_class<T>::value, T>
//    {};

// FIXME - experimental stuff
template <class T>
typename boost::enable_if_c<GnuplotEntry<T>::is_tuple, void>::type
foo(const T &) {
	std::cout << "tuple" << std::endl;
}

//template <class T>
//typename boost::enable_if_c<!GnuplotEntry<T>::is_tuple, void>::type
//foo(const T &) {
//	std::cout << "tuple" << std::endl;
//}

template <class T>
typename boost::enable_if_c<GnuplotEntry<typename T::value_type::value_type>::is_tuple, void>::type
foo(const T &) {
	std::cout << "vec vec tuple" << std::endl;
}

template <class T>
typename boost::enable_if_c<!GnuplotEntry<typename T::value_type::value_type>::is_tuple, void>::type
foo(const T &) {
	std::cout << "vec vec scalar" << std::endl;
}

template<class T>
void f(T&, ...) {
    std::cout << "flat" << std::endl;
}

template<class Cont>
void f(Cont& c, typename Cont::iterator begin = Cont().begin(),
                typename Cont::iterator end   = Cont().end()) {
    std::cout << "container" << std::endl;
}

template <class T>
typename boost::enable_if<boost::mpl::not_<FB::meta::is_container<T> >, void>
g(const T &) {
	std::cout << "g flat" << std::endl;
}

template <class T>
typename boost::enable_if<FB::meta::is_container<T>, void>
g(const T &) {
	std::cout << "g cont" << std::endl;
}

template <class T>
void h(const T &) {
	std::cout << "is_container: " << FB::meta::is_container<T>::value << std::endl;
}

template <class T, bool C>
class B { };

template <class T>
class B<T, false> {
public: void go(const T &) { std::cout << "B flat" << std::endl; }
};

template <class T>
class B<T, true> {
public: void go(const T &) { std::cout << "B cont" << std::endl; }
};

template <class T>
void a(const T &x) {
	B<T, FB::meta::is_container<T>::value>().go(x);
}

template <typename T>
class has_helloworld {
    typedef char one;
    typedef long two;

    template <typename C> static one test(typename C::value_type *, typename C::const_iterator *);
    template <typename C> static two test(...);

public:
    enum { value = sizeof(test<T>(NULL, NULL)) == sizeof(char) };
};

int main() {
	//Gnuplot gp("cat");

	const int NX=3, NY=4;
	std::vector<std::vector<double> > scalar_array(NX);
	std::vector<std::vector<std::pair<double, double> > > tuple_array(NX);

	for(int y=0; y<NY; y++) {
		for(int x=0; x<NX; x++) {
			scalar_array[x].push_back(x*10+y);
			tuple_array[x].push_back(std::make_pair(100+x*10+y, 200+x*10+y));
		}
	}

	//gp.send(scalar_array);
	//gp.send(tuple_array);

	double s = 0.0;
	std::pair<double, double> t;
	std::vector<double> vs;
	std::vector<std::pair<double, double> > vt;

	foo(scalar_array);
	foo(tuple_array);
	//foo(0.0);
	foo(std::make_pair(0.0, 1.0));
	//foo(std::vector<double>());
	foo(std::vector<std::vector<bool> >());

	a(s);
	a(vs);

	std::cout << "hb=" << has_helloworld<std::pair<double, double> >::value << std::endl;
	std::cout << "hb=" << has_helloworld<std::vector<double> >::value << std::endl;
}
