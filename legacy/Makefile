CXXFLAGS+=-Wall -Wextra -I.. -O0 -g
# Don't warn about the fact that we use the deprecated send() function.
CXXFLAGS+=-Wno-deprecated-declarations
LDFLAGS+=-lutil -lboost_iostreams -lboost_system -lboost_filesystem

EVERYTHING=examples examples-blitz examples-interactive

all: examples
	@echo "Now type 'make blitz' if you have blitz installed, and 'make interactive' if you system has PTY support."

blitz: examples-blitz

interactive: examples-interactive

everything: $(EVERYTHING)

%.o: %.cc gnuplot-iostream.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

examples: examples.o
	$(CXX) -o $@ $^ $(LDFLAGS)

examples-blitz: examples-blitz.o
	$(CXX) -o $@ $^ $(LDFLAGS)

examples-interactive: examples-interactive.o
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o
	rm -f *.error.txt
	rm -f examples examples-blitz examples-interactive
	# Windows compilation
	rm -f *.exe *.obj
	# files created by demo scripts
	rm -f my_graph_*.png external_binary.dat external_binary.gnu external_text.dat external_text.gnu inline_binary.gnu inline_text.gnu
