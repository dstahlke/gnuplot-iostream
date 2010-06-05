CXXFLAGS=-Wall -Wextra -I/usr/lib64/blitz/include -O0 -g
LDFLAGS=-lutil -lboost_iostreams

PROGS=example-make-png example-popup example-interactive

all: $(PROGS)
	@echo "Now type 'make blitz-demo' if you have blitz installed."

blitz-demo: example-blitz

clean:
	rm -f *.o
	rm -f $(PROGS) example-blitz
	rm -f my_graph_*.png

*.o: gnuplot-iostream.h

example-interactive: example-interactive.o
	g++ -o $@ $^ $(LDFLAGS)

example-make-png: example-make-png.o
	g++ -o $@ $^ $(LDFLAGS)

example-make-popup: example-make-popup.o
	g++ -o $@ $^ $(LDFLAGS)
