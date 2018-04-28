#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#define MIDI_NOTEON 0x90
#define MIDI_NOTEOFF 0x80

unsigned long note_flags[2];

#define NOTE_SET(n) if (n < 64) { note_flags[0] |= (1 << n); } else { note_flags[1] |= (1 << (n-64)); }
#define NOTE_UNSET(n) if (n < 64) { note_flags[0] &= ~(1 << n); } else { note_flags[1] &= ~(1 << (n-64)); }
#define NOTE_GET(n) (n < 64)? note_flags[0] & (1 << n) : note_flags[1] & (n - 64)

typedef jack_default_audio_sample_t sample_t;

jack_port_t *input_port;
jack_port_t *output_port;
sample_t ramp=0.0;
sample_t note_on;

unsigned char notes[128];
sample_t ramps[128];

sample_t note_frqs[128];

// Set up note frequencies
void calc_note_frqs(sample_t srate) {
    for (int i = 0; i < 128; i++) {
        note_frqs[i] = (2.0 * 440.0 / 32.0) * pow(2, (((sample_t) i - 9.0) / 12.0)) / srate;
    }
}

// Main callback
int process(jack_nframes_t nframes, void *arg) {
    void* port_buf = jack_port_get_buffer(input_port, nframes);
    sample_t *out = (sample_t *) jack_port_get_buffer(output_port, nframes);
    jack_midi_event_t in_event;
    jack_nframes_t event_index = 0;
    jack_nframes_t event_count = jack_midi_get_event_count(port_buf);

    for (unsigned i = 0; i < event_count; i++) {
        jack_midi_event_get(&in_event, port_buf, i);
        unsigned char command = in_event.buffer[0];
        printf("MIDI Event: t=%d, command=0x%x\n", in_event.time, command);
        switch(command) {
        case MIDI_NOTEON:
            printf("\tON: note=%d\n", in_event.buffer[1]);
            break;
        case MIDI_NOTEOFF:
            printf("\tOFF: note=%d\n", in_event.buffer[1]);
            break;
        }
    }

    jack_midi_event_get(&in_event, port_buf, 0);
    for (unsigned i = 0; i < nframes; i++) {
        if ((in_event.time == i) && (event_index < event_count)) {
            if (in_event.buffer[0] == MIDI_NOTEON) {
                NOTE_SET(in_event.buffer[1]);
                notes[in_event.buffer[1]] = 1;
                /* notes[in_event.buffer[1]] = 1; */
            } else if (in_event.buffer[0] == MIDI_NOTEOFF) {
                NOTE_UNSET(in_event.buffer[1]);
                notes[in_event.buffer[1]] = 0;
                /* notes[in_event.buffer[1]] = 1; */
            }
            event_index++;
            if (event_index < event_count) {
                jack_midi_event_get(&in_event, port_buf, event_index);
            }
        }
        out[i] = 0.0;
        for (unsigned char n = 0; n < 128; n++) {
            /* if (NOTE_GET(n)) { */
            if (notes[n]) {
                /* out[i] += sin(2.0 * M_PI * ((sample_t) notes[n] * 2.0 / 128.0 - 1.0)); */
                /* notes[n] += (unsigned char) (note_frqs[n] * 256.0); */
                /* notes[n] %= 256; */
                /* printf("note=%d\n", n); */
                ramps[n] += note_frqs[n];
                ramps[n] = (ramps[n] > 1.0) ? ramps[n] - 2.0 : ramps[n];
                /* ramp += note_frqs[n]; */
                /* ramp = (ramp > 1.0) ? ramp - 2.0 : ramp; */
                out[i] += 0.2 * sin(2 * M_PI * ramps[n]);
                /* notes[n] = (notes[n] == 255)? 0 : notes[n] + 1; */
            }
        }
    }

    return 0;
}

// Sample rate change callback
int srate(jack_nframes_t nframes, void *arg) {
    printf("sample rate changed to %" PRIu32 "/sec\n", nframes);
    calc_note_frqs((sample_t) nframes);
    return 0;
}

// Jack Server shutdown callback
void jack_shutdown(void *arg) {
    exit(1);
}

int main(int argc, char **argv) {
    jack_client_t *client;

    if ((client = jack_client_open("pmidisine", JackNullOption, NULL)) == 0) {
        fprintf(stderr, "jack server not running?\n");
        return 1;
    }

    calc_note_frqs(jack_get_sample_rate(client));

    jack_set_process_callback(client, process, 0);

    jack_set_sample_rate_callback(client, srate, 0);

    jack_on_shutdown(client, jack_shutdown, 0);

    input_port = jack_port_register(client, "midi_in",JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    output_port = jack_port_register(client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (jack_activate(client)) {
        fprintf(stderr, "cannot activate client");
        return 1;
    }


    sleep(-1);
    jack_client_close(client);
    exit(0);
}
