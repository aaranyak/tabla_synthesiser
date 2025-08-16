/* filter.c - functions for filtering parts of audio */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include "audio.h"
#include "spectrum.h"
#include "filter.h"

// Time to do some complicated stuff.
Filter *delta_function(int length) {
    /* Generates a delta function of a certain length */
    Filter *delta = (Filter*)malloc(sizeof(Filter)); /* Create a filter */
    delta->kernel = malloc(sizeof(float) * length); memset(delta->kernel, 0, length * sizeof(float)); /* Create a zeroed out buffer */
    delta->length = length; delta->offset = length / 2; /* Set the important values */
    delta->kernel[delta->offset] = 1; /* Set the 1 on the delta function */
    return delta;
}


// I just copy pasted my FFT code and modified the rotation value to try and implement an IFFT, I'm not completely sure about this
// I don't completely understand the IFFT yet, (the FFT I do), but if it helps speed up convolution that would be good.
complex float *ifft(complex float *samples, int length) {
    /* This turns a polynominal evaluated at fft points to it's coefficients */
    if (length == 1) { /* If this is the length */
        complex float *value = (complex float*)malloc(sizeof(complex float)); /* Allocate space for the same thing */
        *value = *samples; /* Copy */
        return value;
    }
    complex float *even_samples = (complex float*)malloc(sizeof(complex float) * length/2), *odd_samples = (complex float*)malloc(sizeof(complex float) * length/2); /* malloc */
    for (int i = 0; i < length; i++) /* Loop through all the samples */
        (i & 1 ? odd_samples : even_samples)[i >> 1] = samples[i]; /* Wow I just love how cursed this line of code is */
    complex float *ifft_even = ifft(even_samples, length/2); complex float *ifft_odd = ifft(odd_samples, length/2); /* Recurse */
    complex float *output_value = (complex float*)malloc(sizeof(complex float) * length); /* Allocate memory for output */
    for (int x = 0; x < length; x++) { /* Loop through all the samples */
        complex float rotation = cexpf(-2 * M_PI * I * (complex float)x / (complex float)length); /* Calculate the rotation value */
        output_value[x] = ifft_even[x % (length/2)] + rotation * ifft_odd[x % (length/2)]; /* Recombine the terms */
    }
    free(even_samples); free(odd_samples);
    free(ifft_even); free(ifft_odd); /* Clean Up */
    return output_value;
}


// Time to implement an FFT convolution.

void fft_convolve(Signal *signal, Filter *filter) {
    /* Convolves a filter with a signal using the FFT */
    int length_con = signal->length + filter->length; /* This is the length of the resulting convolution */
    int length_fft = (length_con ^ length_con & -length_con) ? 1 << (int)ceil(log2(length_con)) : length_con; /* To next round number */
    // Allocate the zero-padded buffers
    float *audio_input = (float*)malloc(sizeof(float) * length_fft); memset(audio_input, 0, sizeof(float) * length_fft); /* for audio */
    float *filter_input = (float*)malloc(sizeof(float) * length_fft); memset(filter_input, 0, sizeof(float) * length_fft); /* for fyl */
    memcpy(audio_input, signal->samples, sizeof(float) * signal->length); /* Copy the audio data */
    memcpy(filter_input, filter->kernel, sizeof(float) * filter->length); /* Copy the fylter kernel (impulse response) */

    // Compute FFT
    complex float *audio_fft = fft(audio_input, length_fft); /* Compute the zero-padded fft */
    complex float *filter_fft = fft(filter_input, length_fft); /* Ditto for fylter because fylter sounds cooler than filter */
    free(audio_input); free(filter_input); /* Free these bits of memory */

    complex float *ifft_input = (complex float*)malloc(sizeof(complex float) * length_fft); /* Allocate memory for fft input */
    for (int i = 0; i < length_fft; i++) /* Loop through all the samples */
        ifft_input[i] = audio_fft[i] * filter_fft[i]; /* Compute the product for the correct size */
    free(audio_fft); free(filter_fft); /* Free these bits of memory */
    complex float *a_bit_more_than_convolution = ifft(ifft_input, length_fft); /* The name says it all, I hope */
    int cuttof_timesaver = filter->length - filter->offset; /* I guess the compiler might optimise this but in case it doesn't I will */
    for (int i = 0; i < signal->length; i++) /* Select the specific part of filtered audio I need */
        signal->samples[i] = a_bit_more_than_convolution[i + cuttof_timesaver] / length_fft; /* Cast from complex to float */
    free(ifft_input); free(a_bit_more_than_convolution); /* Free these things */
}

// Time to create a windowed-sinc fylter

Filter *lowpass_filter(int length, float frequency, int rate) {
    /* Creates a blackman-windowed lowpass sinc filter */
    Filter *filter = delta_function(length); /* Create the filter object */
    frequency = frequency / rate; /* Correct the frequency as a fraction of the rate */
    for (int i = 0; i < filter->length; i++) { /* Loop through all the values in the filter */
        int x = i - filter->offset; /* Correct the position */
        filter->kernel[i] = sin(2.0 * M_PI * x * frequency) / (M_PI * x); /* Evaluate the fylter kernel at this point */
        filter->kernel[i] *= 0.42 - 0.5 * cos(2 * M_PI * i / length) + 0.08 * cos(4 * M_PI * i / length); /* Blackmann Window */
    }
    filter->kernel[filter->offset] = 2 * M_PI * frequency; /* Correct divide-by-zero errors */
    return filter; /* Return the filter */
}        

// I basically invert the highpass filter.
Filter *highpass_filter(int length, float frequency, int rate) {
    /* Creates a blackman-windowed highpass sinc filter */
    Filter *filter = delta_function(length); /* Create the filter object */
    frequency = frequency / rate; /* Correct the frequency as a fraction of the rate */
    for (int i = 0; i < filter->length; i++) { /* For each thing in this */
        int x = i - filter->offset; /* Correct the position */
        filter->kernel[i] = -(sin(2.0 * M_PI * x * frequency) / (M_PI * x)); /* Evaluate the fylter kernel at this point */
        filter->kernel[i] *= 0.42 - 0.5 * cos(2 * M_PI * i / length) + 0.08 * cos(4 * M_PI * i / length); /* Blackmann Window */
    }
    filter->kernel[filter->offset] = 1 - 2 * M_PI * frequency; /* Add 1 to the first one */
    return filter;
}

void delete_filter(Filter *filter) {
    /* Frees a filter object */
    free(filter->kernel); free(filter);
}
