# Copyright (c) 2013 Daniel Stahlke
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

###########################################################################
# This Makefile is just for the demos and unit tests.  You don't need to compile
# anything to install this package.  Just copy the gnuplot-iostream.h header
# somewhere and you are good to go.
###########################################################################

# The -O0 option speeds up the compile, which is good for testing.  This should
# never be used for production since the generated code is extremely slow!
CXXFLAGS+=-Wall -Wextra -O0 -g -D_GLIBCXX_DEBUG
LDFLAGS+=-lutil -lboost_iostreams -lboost_system -lboost_filesystem

# This makes the examples and tests more complete, but only works if you have the corresponding
# libraries installed.
#CXXFLAGS+=--std=c++11 -DUSE_ARMA=1 -DUSE_BLITZ=1

ALL_EXAMPLES=example-misc example-data-1d example-data-2d example-interactive
TEST_BINARIES=test-noncopyable test-outputs test-empty

all: $(ALL_EXAMPLES)

%.o: %.cc gnuplot-iostream.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

example-misc: example-misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

example-data-1d: example-data-1d.o
	$(CXX) -o $@ $^ $(LDFLAGS)

example-data-2d: example-data-2d.o
	$(CXX) -o $@ $^ $(LDFLAGS)

example-interactive: example-interactive.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test-noncopyable: test-noncopyable.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test-outputs: test-outputs.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test-empty: test-empty.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test-asserts: test-assert-depth.error.txt test-assert-depth-colmajor.error.txt

%.error.txt: %.cc gnuplot-iostream.h
	# These are programs that are supposed to *not* compile.
	# The "!" causes "make" to throw an error if the compile succeeds.
	! $(CXX) $(CXXFLAGS) -c $< -o $<.o 2> $@
	grep -q 'container not deep enough\|boost::STATIC_ASSERTION_FAILURE' $@

test: $(TEST_BINARIES) test-asserts
	mkdir -p unittest-output
	rm -f unittest-output/*
	./test-outputs
	diff -qr unittest-output unittest-output-good

clean:
	rm -f *.o
	rm -f *.error.txt
	rm -f $(ALL_EXAMPLES) $(TEST_BINARIES)
	# Windows compilation
	rm -f *.exe *.obj
	# files created by demo scripts
	rm -f my_graph_*.png external_binary.dat external_binary.gnu external_text.dat external_text.gnu inline_binary.gnu inline_text.gnu

lint:
	cpplint.py --filter=-whitespace,-readability/streams,-build/header_guard gnuplot-iostream.h

cppcheck:
	cppcheck *.cc *.h --template gcc --enable=all -q
