CXXFLAGS+=-Wall -Wextra -Weffc++ -I/usr/lib64/blitz/include -Os -g
LDFLAGS+=-lutil -lboost_iostreams

PROGS=example-make-png example-popup example-interactive
BLITZ_PROGS=example-blitz example-blitz-binary

all: $(PROGS)
	@echo "Now type 'make blitz-demo' if you have blitz installed."

blitz-demo: $(BLITZ_PROGS)

clean:
	rm -f *.o
	rm -f $(PROGS) example-blitz
	rm -f my_graph_*.png

lint:
	cpplint.py --filter=-whitespace,-readability/streams,-build/header_guard gnuplot-iostream.h

cppcheck:
	cppcheck *.cc *.h --template gcc --enable=all -q

*.o: gnuplot-iostream.h
