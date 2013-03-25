#define DO_ARMA 1
#define DO_BLITZ 1
//#define GNUPLOT_ENABLE_CXX11 1 //(__cplusplus >= 201103)

// Include this several times to test delayed loading of armadillo/blitz support.
#include "gnuplot-iostream.h"

#include <fstream>
#include <vector>
#if GNUPLOT_ENABLE_CXX11
#include <tuple>
#include <array>
#endif

#include <boost/array.hpp>

#if DO_ARMA
#include <armadillo>
#endif

#include "gnuplot-iostream.h"

#if DO_BLITZ
#include <blitz/array.h>
#endif

#include "gnuplot-iostream.h"
#include "gnuplot-iostream.h"

using namespace gnuplotio;
Gnuplot gp("cat");

template <typename T, typename ArrayMode>
void foo(std::string header, const T &arg, ArrayMode) {
	std::string modename = ArrayMode::class_name();
	std::string fn_prefix = "unittest-output/"+header+"-"+modename;
	std::cout << "ArrayMode=[" << modename << "]" << std::endl;
	std::cout << "binaryFile=[" << gp.binaryFile(arg, fn_prefix+".bin", "record", ArrayMode()) << "]" << std::endl;
	gp.file(arg, fn_prefix+".txt", ArrayMode());
}

template <typename T>
typename boost::enable_if_c<(ArrayTraits<T>::depth == 1)>::type
bar(std::string header, const T &arg) {
	foo(header, arg, ModeAuto());
	foo(header, arg, Mode1D());
}

template <typename T>
typename boost::enable_if_c<(ArrayTraits<T>::depth == 2)>::type
bar(std::string header, const T &arg) {
	foo(header, arg, ModeAuto());
	foo(header, arg, Mode2D());
	foo(header, arg, Mode1DUnwrap());
}

template <typename T>
typename boost::enable_if_c<(ArrayTraits<T>::depth >= 3)>::type
bar(std::string header, const T &arg) {
	foo(header, arg, ModeAuto());
	foo(header, arg, Mode2D());
	foo(header, arg, Mode2DUnwrap());
}

template <typename T>
void runtest(std::string header, const T &arg) {
	std::cout << "--- " << header << " -------------------------------------" << std::endl;
	std::cout << "depth=" << ArrayTraits<T>::depth << std::endl;
	bar(header, arg);
}

int main() {
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
#if GNUPLOT_ENABLE_CXX11
	std::array<int, NX> si;
	std::vector<  std::tuple<double, int, int> > v_st;
#endif

	for(int x=0; x<NX; x++) {
		vd.push_back(x+7.5);
		vi.push_back(x+7);
		vf.push_back(x+7.2);
		v_bt.push_back(boost::make_tuple(x+0.123, 100+x, 200+x));
#if GNUPLOT_ENABLE_CXX11
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
#if GNUPLOT_ENABLE_CXX11
	runtest("si", si);
	runtest("tie{si,bi}", boost::tie(si, bi));
	runtest("pair{&si,&bi}", std::pair<std::array<int, NX>&, boost::array<int, NX>&>(si, bi));
#endif
	// Doesn't work because array gets cast to pointer
	//runtest("pair{ai,bi}", std::make_pair(ai, bi));
	// However, these work:
	runtest("boost_tie{ai,bi}", boost::tie(ai, bi));
#if GNUPLOT_ENABLE_CXX11
	runtest("std_tie{ai,bi}", std::tie(ai, bi));
	runtest("std_fwd{ai,bi}", std::forward_as_tuple(ai, bi));
#endif

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

	runtest("vvvi cols", vvvi);

	runtest("pair{vf,btup{vd,pair{vi,vi},vf}}", std::make_pair(vf, boost::make_tuple(vd, std::make_pair(vi, vi), vf)));
#if GNUPLOT_ENABLE_CXX11
	runtest("pair{vf,stup{vd,pair{vi,vi},vf}}", std::make_pair(vf, std::make_tuple(vd, std::make_pair(vi, vi), vf)));
	runtest("btup{vd,stup{vi,btup{vf},vi},vd}", boost::make_tuple(vd, std::make_tuple(vi, boost::make_tuple(vf), vi), vd));
#endif

	runtest("v_bt", v_bt);
#if GNUPLOT_ENABLE_CXX11
	runtest("v_st", v_st);
#endif

#if DO_BLITZ
	runtest("blitz2d cols", blitz2d);
#endif
}
