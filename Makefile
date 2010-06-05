CXXFLAGS=-Wall -Wextra -I/usr/lib64/blitz/include -O0 -g
LDFLAGS=-lutil -lboost_iostreams

all: example-make-png example-popup
	@echo "Now type 'make blitz-demo' if you have blitz installed."

blitz-demo: example-interactive

clean:
	rm -f *.o
	rm -f example-interactive example-make-png example-popup
	rm -f my_graph_*.png

*.o: gnuplot-iostream.h

example-interactive: example-interactive.o
	g++ -o $@ $^ $(LDFLAGS)

example-make-png: example-make-png.o
	g++ -o $@ $^ $(LDFLAGS)

example-make-popup: example-make-popup.o
	g++ -o $@ $^ $(LDFLAGS)
