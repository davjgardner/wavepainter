# WavePainter

WavePainter is a custom wavetable synthesizer which allows the user to draw a waveform and then play it at any pitch using MIDI input.

It is intended to run on a Raspberry Pi with the PiTFT Capacitive Touchscreen.

WavePainter uses the JACK Audio Connection Kit for synthesis and Java for the GUI.

For more information, see https://blogs.harvard.edu/wavepainter/

# To Use

Compile with `make`.

Run the main application with `make run-java`. Alternatively, use `java WavePainter | ./synth`.

The GUI program writes wave data to standard out, and the synth reads it from standard in. In theory, any program could be created to feed wave data to the synth. `sendwaves` is another such program, which allows the selection of 5 different waves to send to the synth. Run that with `make run-sendwaves` or `./sendwaves | ./synth`.

The JACK Application does not automatically connect itself to MIDI sources or output sinks, so it will need to be patched manually using an application such as `qjackctl` or `Catia`.
