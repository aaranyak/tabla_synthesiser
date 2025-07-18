/* header file for gui.c */
#ifndef GUI_H
#define GUI_H
#include "audio.h"
#include <gtk/gtk.h>
#define APP_ID "io.github.aaranyak.tabla_synth.analysis"

typedef struct record_info {
    /* Holds some info about recorded audio */
    Signal *signal; /* The audio data */
    char file_name[256]; /* The file name */
    // Implement filter details below.
} RecordInfo;

typedef struct synth_info {
    /* Holds some info about synthesized audio */
    Signal *signal; /* Final synthesis */
} SynthInfo; /* Implement this later */    

typedef struct analyser { /* This contains all analyser info */
    // First start with the loaded signal */
    int synthesised; /* If the audio is synthesised or loaded */
    RecordInfo *recording; /* Null if synthesized */
    SynthInfo *synthesis; /* Null if loaded from file */

    // Info about the analysis range
    size_t clip_start; /* The beginning of the range */
    size_t clip_end; /* The end of the range */

    GtkWidget *start_slide; /* The clip start widget */
    GtkWidget *end_slide; /* The clip end widget */

    // Spectrogram Info.
    int spec_length, spec_num; /* Spectrograminfo */
    float spec_res, spec_scale; /* Spectrogram Frequency Res and scaling factor*/
    float *spectrogram; /* The spec values */
    GtkWidget *spec_display, *spec_min_freq, *spec_max_freq, *spec_mouse_info; /* The canvas */
} Analyser;

int launch_gui(int argc, char **argv);
GtkWidget *audio_view(Analyser *analyser);
GtkWidget *spectrogram_view(Analyser *analyser);
#endif
