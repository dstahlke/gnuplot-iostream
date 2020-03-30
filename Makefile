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
CXXFLAGS+=--std=c++17 -Wall -Wextra -O0 -g -D_GLIBCXX_DEBUG
CXXFLAGS+=-fdiagnostics-color=auto
LDFLAGS+=-lutil -lboost_iostreams -lboost_system -lboost_filesystem

# This makes the examples and tests more complete, but only works if you have the corresponding
# libraries installed.
#CXXFLAGS+=-DUSE_ARMA=1
#CXXFLAGS+=-DUSE_BLITZ=1
#CXXFLAGS+=-DUSE_EIGEN=1 -isystem /usr/include/eigen3

ALL_EXAMPLES=example-misc example-data-1d example-data-2d example-interactive
TEST_BINARIES=test-noncopyable test-outputs test-empty

.DELETE_ON_ERROR:

all: $(ALL_EXAMPLES)

%.o: %.cc gnuplot-iostream.h
	@echo Compiling $@
	$(CXX) $(CXXFLAGS) -c $< -o $@

example-misc: example-misc.o
	@echo Linking $@
	$(CXX) -o $@ $^ $(LDFLAGS)

example-data-1d: example-data-1d.o
	@echo Linking $@
	$(CXX) -o $@ $^ $(LDFLAGS)

example-data-2d: example-data-2d.o
	@echo Linking $@
	$(CXX) -o $@ $^ $(LDFLAGS)

example-interactive: example-interactive.o
	@echo Linking $@
	$(CXX) -o $@ $^ $(LDFLAGS)

test-noncopyable: test-noncopyable.o
	@echo Linking $@
	$(CXX) -o $@ $^ $(LDFLAGS)

test-outputs: test-outputs.o
	@echo Linking $@
	$(CXX) -o $@ $^ $(LDFLAGS)

test-empty: test-empty.o
	@echo Linking $@
	$(CXX) -o $@ $^ $(LDFLAGS)

test-asserts: unittest-errors/test-assert-depth.error.txt unittest-errors/test-assert-depth-colmajor.error.txt
	@echo Running $@
	diff -r unittest-errors-good unittest-errors

unittest-errors/%.error.txt: %.cc gnuplot-iostream.h
	@echo Generating $@
	mkdir -p unittest-errors
	# These are programs that are supposed to *not* compile.
	# The "!" causes "make" to throw an error if the compile succeeds.
	! $(CXX) $(CXXFLAGS) -c $< -o $<.o 2> $@.orig
	grep 'error:' $@.orig |sed 's/.*error:/error:/' > $@
	rm -f $@.orig

test: $(TEST_BINARIES) test-asserts
	@echo Running $@
	mkdir -p unittest-output
	rm -f unittest-output/*
	./test-outputs
	diff -r unittest-output-good unittest-output

clean:
	rm -f *.o
	rm -rf unittest-errors unittest-output
	rm -f $(ALL_EXAMPLES) $(TEST_BINARIES)
	# Windows compilation
	rm -f *.exe *.obj
	# files created by demo scripts
	rm -f my_graph_*.png external_binary.dat external_binary.gnu external_text.dat external_text.gnu inline_binary.gnu inline_text.gnu

lint:
	cpplint.py --filter=-whitespace,-readability/streams,-build/header_guard,-build/include_order,-runtime/references *.h *.cc \
		2>&1 |grep -v 'Include the directory when naming'

cppcheck:
	cppcheck *.cc *.h --template gcc --enable=all -q --force --std=c++17
