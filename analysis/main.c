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
#include <time.h>
#include "audio.h"
#include "load_file.h"
#include "gui.h"

int main(int argc, char **argv) {
    /* This code launches the analyser tool */
    g_random_set_seed(time(NULL));
    launch_gui(argc, argv); /* Open the GUI */
    return 0;
}
