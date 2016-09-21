CFLAGS = -g -Wall -I/opt/X11/include/cairo -I/opt/X11/include/pixman-1
LIB = -L/opt/X11/lib -lcairo -lpixman-1 -lX11 -lm

PROG = test_cairo2
OBJECT = test_cairo2.o

default: $(PROG)

$(PROG): $(OBJECT)
	cc -o $(PROG) $(OBJECT) $(LIB)

clean:
	rm $(OBJECT) $(PROG)

.o: $*.c
	cc -c $*.c
