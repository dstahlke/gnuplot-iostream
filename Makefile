CXXFLAGS+=-Wall -Wextra -I/usr/lib64/blitz/include -O0 -g
LDFLAGS+=-lutil -lboost_iostreams -lboost_system -lboost_filesystem

ALL_EXAMPLES=example-misc example-tuples example-uv example-interactive
TEST_BINARIES=test-noncopyable test-outputs

all: $(ALL_EXAMPLES)

%.o: %.cc gnuplot-iostream.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

example-misc: example-misc.o
	$(CXX) -o $@ $^ $(LDFLAGS)

example-tuples: example-tuples.o
	$(CXX) -o $@ $^ $(LDFLAGS)

example-uv: example-uv.o
	$(CXX) -o $@ $^ $(LDFLAGS)

example-interactive: example-interactive.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test-asserts: test-assert-depth.error.txt test-assert-depth-colmajor.error.txt

%.error.txt: %.cc gnuplot-iostream.h
	# These are programs that are supposed to *not* compile.
	# The "!" causes "make" to throw an error if the compile succeeds.
	! $(CXX) $(CXXFLAGS) -c $< -o $<.o 2> $@

test: $(TEST_BINARIES) test-asserts
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
