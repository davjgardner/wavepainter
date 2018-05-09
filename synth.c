/* synth.c
 *
 * TODO
 *
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#define MIDI_NOTEON 0x90
#define MIDI_NOTEOFF 0x80

#define NSAMPLES 256

typedef jack_default_audio_sample_t sample_t;

jack_port_t *input_port;
jack_port_t *output_port;

pthread_mutex_t input_mutex;

unsigned char notes[128];
sample_t ramps[128];

sample_t note_frqs[128];

sample_t wtable[NSAMPLES];

// Populate the wavetable with samples
void calc_table() {
    for (unsigned i = 0; i < NSAMPLES; i++) {
        // sine wave
        /* wtable[i] = sin(2.0 * M_PI * ((sample_t) i / NSAMPLES)); */

        // triangle wave
        wtable[i] = 2.0 * ((sample_t) i / NSAMPLES) - 1.0;

        // square wave
        /* if (i < NSAMPLES / 2) wtable[i] = 0; else wtable[i] = 1; */
    }
}

// Set up note frequencies
void calc_note_frqs(sample_t srate) {
    for (int i = 0; i < 128; i++) {
        note_frqs[i] = (2.0 * 440.0 / 32.0) * pow(2, (((sample_t) i - 9.0) / 12.0)) / srate;
    }
}

// Main callback
int process(jack_nframes_t nframes, void *arg) {
    sample_t *out = (sample_t *) jack_port_get_buffer(output_port, nframes);

    /* try to lock the input lock - if this fails, the table is being
       updated and we should output silence */
    int l = pthread_mutex_trylock(&input_mutex);
    if (l == 0) {
        // got the lock
        void* port_buf = jack_port_get_buffer(input_port, nframes);
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
                    notes[in_event.buffer[1]] = 1;
                } else if (in_event.buffer[0] == MIDI_NOTEOFF) {
                    notes[in_event.buffer[1]] = 0;
                }
                event_index++;
                if (event_index < event_count) {
                    jack_midi_event_get(&in_event, port_buf, event_index);
                }
            }
            out[i] = 0.0;
            for (unsigned char n = 0; n < 128; n++) {
                if (notes[n]) {
                    ramps[n] += note_frqs[n];
                    ramps[n] = (ramps[n] > 1.0) ? ramps[n] - 1.0 : ramps[n];
                    out[i] += 0.2 * wtable[(unsigned) (ramps[n] * NSAMPLES)];
                }
            }
        }
    } else {
        // didn't get the lock, output silence
        for (unsigned i = 0; i < nframes; i++) {
            out[i] = 0;
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

    pthread_mutex_init(&input_mutex, 0);

    jack_client_t *client;

    if ((client = jack_client_open("wavetable", JackNullOption, NULL)) == 0) {
        fprintf(stderr, "jack server not running?\n");
        return 1;
    }

    calc_note_frqs(jack_get_sample_rate(client));
    calc_table();

    jack_set_process_callback(client, process, 0);

    jack_set_sample_rate_callback(client, srate, 0);

    jack_on_shutdown(client, jack_shutdown, 0);

    input_port = jack_port_register(client, "midi_in",JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    output_port = jack_port_register(client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (jack_activate(client)) {
        fprintf(stderr, "cannot activate client");
        return 1;
    }

    unsigned char buf[NSAMPLES];

    while (1) {
        // wait for input
        int r = read(0, buf, 1);
        if (r == 1) {
            // ready to recieve
            int size = 0;
            unsigned char* into = buf;
            while (size < NSAMPLES) {
                r = read(0, into, NSAMPLES - size);
                if (r > 0) {
                    size += r;
                    into += r;
                }
            }
            // set wavetable data
            /* acquire input lock - prevent sound from
               playing while we set the wavetable data */
            pthread_mutex_lock(&input_mutex);
            for (int i = 0; i < NSAMPLES; i++) {
                wtable[i] = ((sample_t) buf[i]) / 256.0;
            }
            pthread_mutex_unlock(&input_mutex);
        }
    }

    sleep(-1);
    jack_client_close(client);
    exit(0);
}
