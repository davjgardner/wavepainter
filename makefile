CC=gcc
LDLIBS=-lm -ljack -lpthread
all: synth
synth:
clean:
	rm -f *.o synth
.PHONY: all clean
