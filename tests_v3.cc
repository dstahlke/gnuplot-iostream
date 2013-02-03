#define DO_ARMA 1
#define DO_BLITZ 1
#define HAVE_CXXABI 1

#include <fstream>
#include <vector>
#if __cplusplus >= 201103
#include <tuple>
#include <array>
#endif

#include <boost/array.hpp>

// Include this several times to test delayed loading of armadillo/blitz support.
#include "gnuplot-iostream.h"

#if DO_ARMA
#include <armadillo>
#endif

#include "gnuplot-iostream.h"

#if DO_BLITZ
#include <blitz/array.h>
#endif

#include "gnuplot-iostream.h"
#include "gnuplot-iostream.h"

#if HAVE_CXXABI
// for debugging
#include <typeinfo>
#include <cxxabi.h>

template <typename T>
std::string get_typename() {
	int status;
	char *name;
	name = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
	assert(!status);
	return std::string(name);
}
#endif

template <typename T>
void runtest(std::string header, const T &arg) {
	Gnuplot gp("cat");
	std::cout << "--- " << header << " -------------------------------------" << std::endl;
	std::cout << "depth=" << gnuplotio::ArrayTraits<T>::depth << std::endl;
#if HAVE_GCCABI
	std::cout << "val=" << get_typename<typename gnuplotio::ArrayTraits<T>::value_type>() << std::endl;
#endif
	std::cout << "binaryFile=[" << gp.binaryFile(arg, "unittest-output/"+header+".bin") << "]" << std::endl;
	//std::cout << "range_type=" << get_typename<typename gnuplotio::ArrayTraits<T>::range_type>() << std::endl;
	gp.send(arg);
	gp.file(arg, "unittest-output/"+header+".txt");
}

template <typename T, typename ArrayMode>
void runtest(std::string header, const T &arg, ArrayMode) {
	Gnuplot gp("cat");
	std::cout << "--- " << header << " -------------------------------------" << std::endl;
	std::cout << "depth=" << gnuplotio::ArrayTraits<T>::depth << std::endl;
	std::cout << "binaryFile=[" << gp.binaryFile(arg, "unittest-output/"+header+".bin", "record", ArrayMode()) << "]" << std::endl;
	gp.send(arg, ArrayMode());
	gp.file(arg, "unittest-output/"+header+".txt", ArrayMode());
}

int main() {
	gnuplotio::set_debug_array_print(true);

	const int NX=3, NY=4, NZ=2;
	std::vector<double> vd;
	std::vector<int> vi;
	std::vector<float> vf;
	std::vector<std::vector<double> > vvd(NX);
	std::vector<std::vector<int> > vvi(NX);
	std::vector<std::vector<std::vector<int> > > vvvi(NX);
	std::vector<std::vector<std::vector<std::pair<double, int> > > > vvvp(NX);
	int ai[NX];
	boost::array<int, NX> bi;
	std::vector<boost::tuple<double, int, int> > v_bt;
#if __cplusplus >= 201103
	std::array<int, NX> si;
	std::vector<  std::tuple<double, int, int> > v_st;
#endif

	for(int x=0; x<NX; x++) {
		vd.push_back(x+7.5);
		vi.push_back(x+7);
		vf.push_back(x+7.2);
		v_bt.push_back(boost::make_tuple(x+0.123, 100+x, 200+x));
#if __cplusplus >= 201103
		v_st.push_back(std::make_tuple(x+0.123, 100+x, 200+x));
		si[x] = x+90;
#endif
		ai[x] = x+7;
		bi[x] = x+70;
		for(int y=0; y<NY; y++) {
			vvd[x].push_back(100+x*10+y);
			vvi[x].push_back(200+x*10+y);
			std::vector<int> tup;
			tup.push_back(300+x*10+y);
			tup.push_back(400+x*10+y);
			vvvi[x].push_back(tup);

			std::vector<std::pair<double, int> > stuff;
			for(int z=0; z<NZ; z++) {
				stuff.push_back(std::make_pair(
						x*1000+y*100+z+0.5,
						x*1000+y*100+z));
			}
			vvvp[x].push_back(stuff);
		}
	}

	runtest("vd,vi,bi", std::make_pair(vd, std::make_pair(vi, bi)));
	runtest("vvd", vvd);
	runtest("vvd,vvi", std::make_pair(vvd, vvi));
	runtest("ai", ai);
	runtest("bi", bi);
#if __cplusplus >= 201103
	runtest("si", si);
	runtest("tie{si,bi}", boost::tie(si, bi));
	runtest("pair{&si,&bi}", std::pair<std::array<int, NX>&, boost::array<int, NX>&>(si, bi));
#endif
	// Doesn't work because array gets cast to pointer
	//runtest("pair{ai,bi}", std::make_pair(ai, bi));
	// However, this does work:
	runtest("tie{ai,bi}", boost::tie(ai, bi));
	runtest("pair{ai,bi}", std::pair<int(&)[NX], boost::array<int, NX> >(ai, bi));
	runtest("vvd,vvi,vvvi", std::make_pair(vvd, std::make_pair(vvi, vvvi)));
	runtest("vvvp", vvvp);

#if DO_ARMA
	arma::vec armacol(NX);
	arma::mat armamat(NX, NY);

	for(int x=0; x<NX; x++) {
		armacol(x) = x+0.123;
		for(int y=0; y<NY; y++) {
			armamat(x, y) = x*10+y+0.123;
		}
	}

	runtest("armacol", armacol);
	runtest("armamat", armamat);
#endif

#if DO_BLITZ
	blitz::Array<double, 1> blitz1d(NX);
	blitz::Array<double, 2> blitz2d(NX, NY);
	{
		blitz::firstIndex i;
		blitz::secondIndex j;
		blitz1d = i + 0.777;
		blitz2d = 100 + i*10 + j;
	}
	blitz::Array<blitz::TinyVector<double, 2>, 2> blitz2d_tup(NX, NY);
	for(int x=0; x<NX; x++) {
		for(int y=0; y<NY; y++) {
			blitz2d_tup(x, y)[0] = 100+x*10+y;
			blitz2d_tup(x, y)[1] = 200+x*10+y;
		}
	}

	runtest("blitz1d", blitz1d);
	runtest("blitz1d,vd", std::make_pair(blitz1d, vd));
	runtest("blitz2d", blitz2d);
	runtest("blitz2d_tup", blitz2d_tup);
	runtest("blitz2d,vvi", std::make_pair(blitz2d, vvi));
	runtest("blitz2d,vd", std::make_pair(blitz2d, vd));
#endif

	runtest("vvvi cols", vvvi, gnuplotio::Mode2DUnwrap());

	runtest("pair{vf,btup{vd,pair{vi,vi},vf}}", std::make_pair(vf, boost::make_tuple(vd, std::make_pair(vi, vi), vf)));
#if __cplusplus >= 201103
	runtest("pair{vf,stup{vd,pair{vi,vi},vf}}", std::make_pair(vf, std::make_tuple(vd, std::make_pair(vi, vi), vf)));
	runtest("btup{vd,stup{vi,btup{vf},vi},vd}", boost::make_tuple(vd, std::make_tuple(vi, boost::make_tuple(vf), vi), vd));
#endif

	runtest("v_bt", v_bt);
#if __cplusplus >= 201103
	runtest("v_st", v_st);
#endif

#if DO_BLITZ
	runtest("blitz2d cols", blitz2d, gnuplotio::Mode1DUnwrap());

	Gnuplot gp("cat");
	gp << "### foobar ###" << std::endl;
	gp.send(blitz2d, gnuplotio::ModeAuto());
	gp << "### foobar ###" << std::endl;
	gp.foobar(blitz2d);
	gp << "### foobar ###" << std::endl;
	gp.foobar<gnuplotio::Mode1D>(blitz2d);
#endif
}
