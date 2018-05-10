#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define NSAMPLES 256

char wtable[NSAMPLES];

typedef enum {wsin = 0, wtri, wsqr, wrmp, wsaw} wave_t;

// Populate the wavetable with samples
void calc_table(wave_t type) {
    for (unsigned i = 0; i < NSAMPLES; i++) {
        double val;
        switch(type) {
        case wsin:
            val = sin(2.0 * M_PI * ((double) i / NSAMPLES));
            break;
        case wtri:
            val = (i < NSAMPLES / 2)? (2.0 * (double) i / (NSAMPLES / 2)) - 1.0
                : (2.0 * (double) (NSAMPLES - (i - NSAMPLES / 2)) / (NSAMPLES / 2));
            break;
        case wsqr:
            if (i < NSAMPLES / 2) val = 0; else val = 1;
            break;
        case wrmp:
            val = (2.0 * (double) i / NSAMPLES) - 1.0;
            break;
        case wsaw:
            val = (2.0 * (double) (NSAMPLES - i) / NSAMPLES) - 1.0;
            break;
        }

        wtable[i] = (char) (val * 127.0);
    }
}

void send_data(wave_t type) {
    calc_table(type);

    // say we're sending
    write(1, "s", 1);
    // send
    write(1, wtable, NSAMPLES);
}

int main(void) {

    dprintf(2, "Send a wave: [sin=0, tri=1, sqr=2, rmp=3, saw=4]\n");

    while (1) {
        // print prompt
        dprintf(2, "-> ");
        // read input
        int w;
        scanf("%d", &w);
        if (w < wsin || w > wsaw) {
            dprintf(2, "invalid wave type\n");
        } else {
            send_data(w);
        }
    }
}
