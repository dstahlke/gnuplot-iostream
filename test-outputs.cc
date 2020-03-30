/*
Copyright (c) 2020 Daniel Stahlke

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

// Include this several times to test delayed loading of armadillo/blitz support.
#include "gnuplot-iostream.h"

#include <fstream>
#include <vector>
#include <tuple>
#include <array>
#include <cstdint>

#include <boost/array.hpp>

#if USE_ARMA
#include <armadillo>
#endif

#ifdef USE_EIGEN
#include <Eigen/Dense>
#endif

#include "gnuplot-iostream.h"

#if USE_BLITZ
#include <blitz/array.h>
#endif

#include "gnuplot-iostream.h"
#include "gnuplot-iostream.h"

using namespace gnuplotio;

Gnuplot gp;
const std::string basedir = "unittest-output";

template <typename T, bool DoBinary, typename ArrayMode>
void test_given_mode(
    std::ostream &log_fh, std::string header, const T &arg, ArrayMode
) {
    if constexpr (DoBinary) {
        std::string modename = ArrayMode::class_name();
        std::string fn_prefix = basedir+"/"+header+"-"+modename;
        log_fh << "* " << modename << " -> "
            << gp.binaryFile(arg, fn_prefix+".bin", "record", ArrayMode()) << std::endl;
        gp.file(arg, fn_prefix+".txt", ArrayMode());
    } else {
        std::string modename = ArrayMode::class_name();
        std::string fn_prefix = basedir+"/"+header+"-"+modename;
        log_fh << "* " << modename << " (skipped binary) " << std::endl;
        gp.file(arg, fn_prefix+".txt", ArrayMode());
    }
}

template <typename T, bool DoBinary>
typename std::enable_if_t<(ArrayTraits<T>::depth == 1)>
runtest_inner(std::ostream &log_fh, std::string header, const T &arg) {
    test_given_mode<T, DoBinary>(log_fh, header, arg, Mode1D());
}

template <typename T, bool DoBinary>
typename std::enable_if_t<(ArrayTraits<T>::depth == 2)>
runtest_inner(std::ostream &log_fh, std::string header, const T &arg) {
    test_given_mode<T, DoBinary>(log_fh, header, arg, Mode2D());
    // FIXME need to make baselines for these
    //test_given_mode<T, DoBinary>(log_fh, header, arg, Mode1D());
    test_given_mode<T, DoBinary>(log_fh, header, arg, Mode1DUnwrap());
}

template <typename T, bool DoBinary>
typename std::enable_if_t<(ArrayTraits<T>::depth >= 3)>
runtest_inner(std::ostream &log_fh, std::string header, const T &arg) {
    test_given_mode<T, DoBinary>(log_fh, header, arg, Mode2D());
    test_given_mode<T, DoBinary>(log_fh, header, arg, Mode2DUnwrap());
}

template <typename T, bool DoBinary>
void runtest_maybe_dobin(std::string header, const T &arg) {
    std::ofstream log_fh((basedir+"/"+header+"-log.txt").c_str());
    log_fh << "--- " << header << " -------------------------------------" << std::endl;
    log_fh << "depth=" << ArrayTraits<T>::depth << std::endl;
    log_fh << "ModeAutoDecoder=" << ModeAutoDecoder<T>::mode::class_name() << std::endl;
    runtest_inner<T, DoBinary>(log_fh, header, arg);
}

template <typename T>
void runtest(std::string header, const T &arg) {
    runtest_maybe_dobin<T, true>(header, arg);
}

template <typename T>
void runtest_nobin(std::string header, const T &arg) {
    runtest_maybe_dobin<T, false>(header, arg);
}

template <typename T, bool DoBinary>
void basic_datatype_test_integral(std::string name) {
    std::vector<T> v;
    for(int i=0; i<4; i++) {
        v.push_back(i);
    }
    runtest_maybe_dobin<std::vector<T>, DoBinary>(name, v);
}

template <typename T, bool DoBinary>
void basic_datatype_test_float(std::string name) {
    std::vector<T> v;
    for(int i=0; i<4; i++) {
        v.push_back(i + T(0.1234));
    }
    v.push_back(std::numeric_limits<T>::quiet_NaN());
    runtest_maybe_dobin<std::vector<T>, DoBinary>(name, v);
}

int main() {
    gp << std::setprecision(6);

    const int NX=3, NY=4, NZ=2;
    std::vector<double> vd;
    std::vector<int> vi;
    std::vector<float> vf;
    std::vector<std::vector<double>> vvd(NX);
    std::vector<std::vector<int>> vvi(NX);
    std::vector<std::vector<std::vector<int>>> vvvi(NX);
    std::vector<std::vector<std::vector<std::pair<double, int>>>> vvvp(NX);
    int ai[NX];
    boost::array<int, NX> bi;
    std::vector<boost::tuple<double, int, int>> v_bt;
    std::array<int, NX> si;
    std::vector<  std::tuple<double, int, int>> v_st;

    for(int x=0; x<NX; x++) {
        vd.push_back(x+7.5);
        vi.push_back(x+7);
        vf.push_back(x+7.2F);
        v_bt.push_back(boost::make_tuple(x+0.123, 100+x, 200+x));
        v_st.push_back(std::make_tuple(x+0.123, 100+x, 200+x));
        si[x] = x+90;
        ai[x] = x+7;
        bi[x] = x+70;
        for(int y=0; y<NY; y++) {
            vvd[x].push_back(100+x*10+y);
            vvi[x].push_back(200+x*10+y);
            std::vector<int> tup;
            tup.push_back(300+x*10+y);
            tup.push_back(400+x*10+y);
            vvvi[x].push_back(tup);

            std::vector<std::pair<double, int>> stuff;
            for(int z=0; z<NZ; z++) {
                stuff.push_back(std::make_pair(
                        x*1000+y*100+z+0.5,
                        x*1000+y*100+z));
            }
            vvvp[x].push_back(stuff);
        }
    }

    basic_datatype_test_integral<  int8_t, true>("vi8");
    basic_datatype_test_integral< uint8_t, true>("vu8");
    basic_datatype_test_integral< int16_t, true>("vi16");
    basic_datatype_test_integral<uint16_t, true>("vu16");
    basic_datatype_test_integral< int32_t, true>("vi32");
    basic_datatype_test_integral<uint32_t, true>("vu32");

    // these should all print as integers
    basic_datatype_test_integral<char, false>("vpc");
    basic_datatype_test_integral<signed char, false>("vsc");
    basic_datatype_test_integral<unsigned char, false>("vuc");

    basic_datatype_test_float<float, true>("vf");
    basic_datatype_test_float<double, true>("vd");
    basic_datatype_test_float<long double, false>("vld");

    runtest("vd,vi,bi", std::make_pair(vd, std::make_pair(vi, bi)));
    runtest("vvd", vvd);
    runtest("vvd,vvi", std::make_pair(vvd, vvi));
    runtest("ai", ai);
    runtest("bi", bi);
    runtest("si", si);
    runtest("tie{si,bi}", boost::tie(si, bi));
    runtest("pair{&si,&bi}", std::pair<std::array<int, NX>&, boost::array<int, NX>&>(si, bi));
    // Doesn't work because array gets cast to pointer
    //runtest("pair{ai,bi}", std::make_pair(ai, bi));
    // However, these work:
    runtest("boost_tie{ai,bi}", boost::tie(ai, bi));
    runtest("std_tie{ai,bi}", std::tie(ai, bi));
    runtest("std_fwd{ai,bi}", std::forward_as_tuple(ai, bi));

    runtest("pair{ai,bi}", std::pair<int(&)[NX], boost::array<int, NX>>(ai, bi));
    runtest("vvd,vvi,vvvi", std::make_pair(vvd, std::make_pair(vvi, vvvi)));
    runtest("vvvp", vvvp);

#if USE_ARMA
    arma::vec armacol(NX);
    arma::rowvec armarow(NX);
    arma::mat armamat(NX, NY);

    for(int x=0; x<NX; x++) {
        armacol(x) = x+0.123;
        armarow(x) = x+0.123;
        for(int y=0; y<NY; y++) {
            armamat(x, y) = x*10+y+0.123;
        }
    }

    runtest("armacol", armacol);
    runtest("armarow", armarow);
    runtest("armamat", armamat);
#endif

#if USE_EIGEN
    Eigen::VectorXd eigencol(NX);
    Eigen::RowVectorXd eigenrow(NX);
    Eigen::MatrixXd eigenmat(NX, NY);

    for(int x=0; x<NX; x++) {
        eigencol(x) = x+0.123;
        eigenrow(x) = x+0.123;
        for(int y=0; y<NY; y++) {
            eigenmat(x, y) = x*10+y+0.123;
        }
    }

    runtest("eigencol", eigencol);
    runtest("eigenrow", eigenrow);
    runtest("eigenmat", eigenmat);
#endif

#if USE_BLITZ
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
    runtest("pair{vf,stup{vd,pair{vi,vi},vf}}", std::make_pair(vf, std::make_tuple(vd, std::make_pair(vi, vi), vf)));
    runtest("btup{vd,stup{vi,btup{vf},vi},vd}", boost::make_tuple(vd, std::make_tuple(vi, boost::make_tuple(vf), vi), vd));

    runtest("v_bt", v_bt);
    runtest("v_st", v_st);

#if USE_BLITZ
    runtest("blitz2d cols", blitz2d);
#endif
}
