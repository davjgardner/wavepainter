CC=gcc
LDLIBS=-lm -ljack -lpthread
all: synth sendwaves
synth:
sendwaves:
clean:
	rm -f *.o synth sendwaves
run: all
	./sendwaves | ./synth
.PHONY: all clean
