CXXFLAGS+=-Wall -Wextra -I/usr/lib64/blitz/include -O0 -g
# FIXME - bring this back
#CXXFLAGS+=-Weffc++
LDFLAGS+=-lutil -lboost_iostreams -lboost_system -lboost_filesystem

EVERYTHING=examples examples-blitz examples-interactive

all: examples
	@echo "Now type 'make blitz' if you have blitz installed, and 'make interactive' if you system has PTY support."

blitz: examples-blitz

interactive: examples-interactive

everything: $(EVERYTHING)

clean:
	rm -f *.o
	rm -f examples examples-blitz examples-interactive
	# files created by demo scripts
	rm -f my_graph_*.png external_binary.dat external_binary.gnu external_text.dat external_text.gnu inline_binary.gnu inline_text.gnu

lint:
	cpplint.py --filter=-whitespace,-readability/streams,-build/header_guard gnuplot-iostream.h

cppcheck:
	cppcheck *.cc *.h --template gcc --enable=all -q

$(EVERYTHING) *.o: gnuplot-iostream.h
