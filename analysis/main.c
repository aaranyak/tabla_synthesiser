/* analyser - a self-written program that can analyse digital audio samples through a variety of systems
 * Including:
 *  -> Spectrogram
 *  -> Fourier Transform
 *  -> Change in amplitude
 *  -> I'll see if I need more later.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#include "load_file.h"

int main(int argc, char **argv) {
    /* This code launches the analyser tool */
    char *file_path = 0; if (argc - 1) file_path = argv[1]; /* Get first argument */
    if (!file_path) return 1; /* Only continue if a file has been provided */
    Signal *signal = load_wav(file_path); /* Attempt to load a file */
    if (!signal) return 1; /* Check if it loaded */
    render_samples(signal, 5000, 100, 2, 8, 4); /* Render the signal */
    delete_signal(signal); /* Delete this */
    return 0;
}
