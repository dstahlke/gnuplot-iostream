CXXFLAGS+=-Wall -Wextra -I/usr/lib64/blitz/include -O0 -g
LDFLAGS+=-lutil -lboost_iostreams -lboost_system -lboost_filesystem

ALL_BINARIES=example-tuples example-uv example-interactive

# FIXME - some require various libraries
all: $(ALL_BINARIES)

%.o: %.cc gnuplot-iostream.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

example-tuples: example-tuples.o
	$(CXX) -o $@ $^ $(LDFLAGS)

example-uv: example-uv.o
	$(CXX) -o $@ $^ $(LDFLAGS)

example-interactive: example-interactive.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test-asserts: test-assert-depth.error.txt test-assert-depth-colmajor.error.txt

%.error.txt: %.cc gnuplot-iostream.h
	! $(CXX) $(CXXFLAGS) -c $< -o $<.o 2> $@

######################
# FIXME - remove all mentions of tests_v3 from makefile
tests_v3: tests_v3.o
	$(CXX) -o $@ $^ $(LDFLAGS)

tests_v4: tests_v4.o
	$(CXX) -o $@ $^ $(LDFLAGS)
# FIXME - end of experimental stuff
######################

clean:
	rm -f *.o
	rm -f *.error.txt
	rm -f $(ALL_BINARIES)
	# Windows compilation
	rm -f *.exe *.obj
	# files created by demo scripts
	rm -f my_graph_*.png external_binary.dat external_binary.gnu external_text.dat external_text.gnu inline_binary.gnu inline_text.gnu

lint:
	cpplint.py --filter=-whitespace,-readability/streams,-build/header_guard gnuplot-iostream.h

cppcheck:
	cppcheck *.cc *.h --template gcc --enable=all -q
