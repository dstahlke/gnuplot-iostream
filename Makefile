CPPFLAGS=-Wall -Wextra -I/usr/lib64/blitz/include -O0 -g
LDFLAGS=-lutil -lboost_iostreams

all: example-make-png
	@echo "Now type 'make blitz-demo' if you have blitz installed."

blitz-demo: example-interactive

clean:
	rm -f *.o
	rm -f example-interactive example-make-png
	rm -f my_graph_*.png

example-interactive: example-interactive.o gnuplot-iostream.o
	g++ -o $@ $^ $(LDFLAGS)

example-make-png: example-make-png.o gnuplot-iostream.o
	g++ -o $@ $^ $(LDFLAGS)
