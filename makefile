CC=gcc
JC=javac
LDLIBS=-lm -ljack -lpthread
all: synth sendwaves gui
synth:
sendwaves:
%.class: %.java
	$(JC) $<
gui: WavePainter.class
clean:
	rm -f *.o synth sendwaves *.class
run-java: all
	java WavePainter | ./synth
run-sendwaves: all
	./sendwaves | ./synth
.PHONY: all clean
