/* filter.c - functions for filtering parts of audio */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include "audio.h"
#include "spectrum.h"

// Time to do some complicated stuff.

Filter *delta_function(int length) {
    /* Generates a delta function of a certain length */
    Filter *delta = (Filter*)malloc(sizeof(Filter)); /* Create a filter */
    filter->kernel = malloc(sizeof(float) * length); memset(filter->kernel, 0, length * sizeof(float)); /* Create a zeroed out buffer */
    filter->length = length; filter->offset = length / 2; /* Set the important values */
    filter[filter->offset] = 1; /* Set the 1 on the delta function */
}


// I just copy pasted my FFT code and modified the rotation value to try and implement an IFFT, I'm not completely sure about this
// I don't completely understand the IFFT yet, (the FFT I do), but if it helps speed up convolution that would be good.
complex float *ifft(complex float *samples, int length) {
    /* The recursive kernel for the inverse fast fourier transform */
    if (length == 1) { /* Some tough stuff coming up */
        complex float *value = (complex float*)malloc(sizeof(complex float)); /* Create a pointer */
        *value = *samples; /* Dereference */
        return value;
    }

    // Divide the samples into odd and even ones.
    complex float *even_samples = (complex float*)malloc(sizeof(complex float) * length/2), *odd_samples = (complex float*)malloc(sizeof(complex float) * length/2); /* malloc */
    for (int i = 0; i < length; i++) /* Loop through all the samples */
        (i & 1 ? odd_samples : even_samples)[i >> 1] = samples[i]; /* Wow I just love how cursed this line of code is */

    // Now, allow the next iteration to do it's thing.
    complex float *fourier_even = ifft(even_samples, length/2); /* Get the IFFT of the even samples */
    complex float *fourier_odd = ifft(odd_samples, length/2); /* Get the IFFT of the odd samples */

    // Merge step - since we have the odd and even terms, evaluated on the squares of x.
    complex float *fourier_transform = (complex float*)malloc(sizeof(complex float) * length); /* Create the next array */
    for (int x = 0; x < length; x++) { /* Loop through all the roots of unity */
        complex double rotation = ((complex double)1 / (complex double)length) *  cexp(-(complex double)(I * M_PI * 2) * ((double)x / (double)length)); /* Calculate rotation */
        fourier_transform[x] = fourier_even[x % (length/2)] + rotation * fourier_odd[x % (length/2)]; /* Recombine the terms */
    } /* Finally one stage of the fft is done */

    // Clean up all the stuff we created.
    free(even_samples); free(odd_samples); /* Free the broken stuff */
    free(fourier_even); free(fourier_odd); /* Free the result stuff */

    return fourier_transform; /* Return the fourier transform */
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

    complex float *ifft_input = (complex float*)malloc(sizeof(complex float) * length_fft); /* Allocate memory for fft input */
    for (int i = 0; i < length_fft; i++) /* Loop through all the samples */
        ifft_input[i] = audio_fft[i] * filter_fft[i]; /* Compute the product for the correct size */
    free(audio_fft); free(filter_fft); /* Free these bits of memory */
    complex float *a_bit_more_than_convolution = ifft(ifft_input, length_fft); /* The name says it all, I hope */
    int cuttof_timesaver = filter->length / 2; /* I guess the compiler might optimise this but in case it doesn't I will */
    for (int i = 0; i < signal->length; i++) /* Select the specific part of filtered audio I need */
        signal->samples[i] = a_bit_more_than_convolution[i + cuttof_timesaver]; /* Set the part, since we cast from complex to float */
    free(ifft_input); free(a_bit_more_than_convolution); /* Free these things */
}

// Time to create a windowed-sinc fylter

Filter *lowpass_filter(int length, float frequency, int rate) {
    /* Creates a blackman-windowed lowpass sinc filter */
    Filter *filter = delta_function(length); /* Create the filter object */
    for (int i = 0; i < filter->length; i++) { /* Loop through all the values in the filter */
        float x = i - offset; /* Calculate the current actual position */
        filter->kernel[i] = sin(2.0 * M_PI * x * frequency / (float)rate) / (M_PI * x); /* Evaluate the fylter kernel at this point */
        filter->kernel[i] *= 0.42 - 0.5 * cos(2.0 * M_PI * x / (float)(length - 1)) + 0.08 * cos(4.0 * M_PI * x / (float)(length - 1)); /* Apply a blackman window */
    }
    return filter; /* Return the filter */
}        

// I basically copy-pasted the above code and added a minus sign. Filters are easy.
Filter *highpass_filter(int length, float frequency, int rate) {
    /* Creates a blackman-windowed highpass sinc filter */
    Filter *filter = delta_function(length); /* Create the filter object */
    for (int i = 0; i < filter->length; i++) { /* Loop through all the values in the filter */
        float x = i - offset; /* Calculate the current actual position */
        filter->kernel[i] -= sin(2.0 * M_PI * x * frequency / (float)rate) / (M_PI * x); /* Evaluate the fylter kernel at this point */
        filter->kernel[i] *= 0.42 - 0.5 * cos(2.0 * M_PI * x / (float)(length - 1)) + 0.08 * cos(4.0 * M_PI * x / (float)(length - 1)); /* Apply a blackman window */
    }
    return filter; /* Return the filter */
}
