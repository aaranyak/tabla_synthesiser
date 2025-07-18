/* audio.c - Some helpful stuff you can do with audio */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "audio.h"

Signal *empty_signal(int rate, size_t length) {
    /* Generates an empty signal of a given sample rate and length */
    Signal *signal = (Signal*)malloc(sizeof(Signal)); /* Create an object for the signal */
    signal->rate = rate; signal->length = length; /* Set values */
    signal->samples = (float*)malloc(sizeof(float) * length); /* Allocate memory for the samples */
    memset(signal->samples, 0, sizeof(float) * length); /* Set all the bytes in the memory to 0, this works for floats, but only 0.0 */
    return signal; /* Returns the signal */
}

void delete_signal(Signal *signal) {
    /* Frees the memory in a signal */
    free(signal->samples); free(signal); /* Wow that was simple! */
}

int add_sine_wave(Signal *signal, float amplitude, float frequency, float phase) {
    /* Adds a sine wave of given frequency and amplitude to the signal */
    for (size_t i = 0; i < signal->length; i++)
        /* This adds a sine wave to the signal */
        signal->samples[i] += (float)(sin(((double)2.0 * (double)M_PI * ((double)i / (double)signal->rate) * frequency) + (double)phase) * amplitude); /* Evaluate the oscillater at that position */
    return 0;
}

int superimpose(Signal *target, Signal *other) {
    /* Superimposes the second signal onto the first one */
    if (target->rate != other->rate) return 1; /* Check if they match sample rates */
    for (size_t i = 0; i < target->length; i++) /* Loop through all in the first signal */
        if (i < other->length) target->samples[i] += other->samples[i]; /* Superimpose */
    return 0; /* No errors */
}

int change_amplitude(Signal *signal, float factor) {
    /* Scales the samples by a specific factor */
    for (size_t i = 0; i < signal->length; i++) /* Loop through all the samples */
        signal->samples[i] *= factor; /* Scale the sample */
    return 0; /* No errors */
}

Signal *trim_signal(Signal *original, size_t start, size_t end) {
    /* Trims a signal and creates a new one */
    Signal *trimmed = empty_signal(original->rate, end - start); /* Create signal of correct size */
    memcpy(trimmed->samples, &original->samples[start], sizeof(float) * (end - start)); /* Copy the samples into the next array */
    return trimmed;
}

void render_samples(Signal *signal, int offset, int samples, int together, int height, int scale) {
    /* Renders a signal to the terminal */
    int *mins = (int*)malloc(sizeof(int) * samples); /* Now we take the means */
    int *maxes = (int*)malloc(sizeof(int) * samples); /* Now we take the means */
    for (int i = 0; i < samples; i++) { /* Loop through each height */
        float min = signal->samples[offset + i*together]*scale; /* Take the min and max */
        float max = signal->samples[offset + i*together]*scale; /* Take the min and max */
        for (int j = 0; j < together; j++) { /* Loop through subsamples */
            if (signal->samples[offset + i * together + j]*scale > max) max = signal->samples[offset + i * together + j]*scale;
            if (signal->samples[offset + i * together + j]*scale < min) min = signal->samples[offset + i * together + j]*scale;
        } mins[i] = min * height; maxes[i] = max * height; /* Set min and max */ 
    }
    printf("\n");
    for (int i = height; i >= -height; i--) {
        for (int j = 0; j < samples; j++) { /* Columns of stuffs */
            if (i == 0) printf("-"); /* Middle line */
            else if (i >= mins[j] && i <= maxes[j]) printf("|"); /* Here you go */
            else printf(" "); /* Nothing here */
        } printf("\n"); /* Newline */
    }
    free(mins); free(maxes);
}
