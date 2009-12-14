CPPFLAGS=-Wall -Wextra -I/usr/lib64/blitz/include -O0 -g
LDFLAGS=-lutil -lboost_iostreams

all: example-interactive example-make-png

clean:
	rm -f *.o
	rm -f example-interactive example-make-png
	rm -f my_graph_1.png my_graph_2.png

example-interactive: example-interactive.o gnuplot-iostream.o
	g++ -o $@ $^ $(LDFLAGS)

example-make-png: example-make-png.o gnuplot-iostream.o
	g++ -o $@ $^ $(LDFLAGS)
