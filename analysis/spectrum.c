/* spectrum.c - functions for spectral analysis */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "audio.h"
#include "spectrum.h"

// Fourier Transform Stuff

/* Currently, I am implementing the fast fourier transform by allocating memory at each function
 * If this turns out to be unoptimal, I can probably optimise by having pre-allocated buffers for this
 * Which would require allocating log_2(n) buffers to split the original samples, and then log_2(n) buffers for the calculation
 * This should be implemented in a separate function that handles allocation and freeing, while the fft kernel has access to parts of it.
*/

complex float *fft(float *samples, int length) {
    /* The recursive kernel for the fast fourier transform */
    if (length == 1) { /* Some tough stuff coming up */
        complex float *value = (complex float*)malloc(sizeof(complex float)); /* Create a pointer */
        *value = *samples; /* Dereference */
        return value;
    }
    
    // Divide the samples into odd and even ones.
    float *even_samples = (float*)malloc(sizeof(float) * length/2), *odd_samples = (float*)malloc(sizeof(float) * length/2); /* malloc */
    for (int i = 0; i < length; i++) /* Loop through all the samples */
        (i & 1 ? odd_samples : even_samples)[i >> 1] = samples[i]; /* Wow I just love how cursed this line of code is */

    // Now, allow the next iteration to do it's thing.
    complex float *fourier_even = fft(even_samples, length/2); /* Get the FFT of the even samples */
    complex float *fourier_odd = fft(odd_samples, length/2); /* Get the FFT of the odd samples */
    
    // Merge step - since we have the odd and even terms, evaluated on the squares of x.
    complex float *fourier_transform = (complex float*)malloc(sizeof(complex float) * length); /* Create the next array */
    for (int x = 0; x < length; x++) { /* Loop through all the roots of unity */
        complex double rotation = cexp((complex double)(I * M_PI * 2) * ((double)x / (double)length)); /* Calculate rotation */
        fourier_transform[x] = fourier_even[x % (length/2)] + rotation * fourier_odd[x % (length/2)]; /* Recombine the terms */
    } /* Finally one stage of the fft is done */
    
    // Clean up all the stuff we created.
    free(even_samples); free(odd_samples); /* Free the broken stuff */
    free(fourier_even); free(fourier_odd); /* Free the result stuff */

    return fourier_transform; /* Return the fourier transform */
}

Spectrum *discrete_fourier_transform(Signal *signal) {
    /* Calculates the discrete fourier transform of a signal and returns a spectrum */

    int is_zero_padded = signal->length ^ (signal->length & -signal->length); /* Check if the number of samples is a power of 2 */    
    int fft_length = is_zero_padded ? 1 << (int)round(log2(signal->length)) : signal->length; /* Calculate closest power of 2 */
    float *fft_input = (float*)malloc(sizeof(float) * fft_length); memset(fft_input, 0, sizeof(float) * fft_length); /* init 0 buffer */
    memcpy(fft_input, signal->samples, sizeof(float) * fmin(fft_length, signal->length)); /* Copy the samples into the buffer */
    
    complex float *fourier_transform = fft(fft_input, fft_length); /* Compute fourier transform */

    // Now, time to sort out the ampltudes and frequencies and stuff.
    Spectrum *spectrum = (Spectrum*)malloc(sizeof(Spectrum)); /* Return a spectrum */
    spectrum->amplitudes = (float*)malloc(sizeof(float) * fft_length / 2); /* Only half of the stuff are reliable */
    spectrum->offsets = (float*)malloc(sizeof(float) * fft_length / 2); /* Only half of the stuff are reliable */
    spectrum->frequencies = fft_length / 2; /* Ditto */
    spectrum->resolution = (float)signal->rate / (float)fft_length; /* Gap between frequencies */
    for (int i = 0; i < fft_length / 2; i++) { /* Loop through all bins to get the fft */
        spectrum->amplitudes[i] = 2.0 * cabsf(fourier_transform[i]) / fft_length; /* Calulate amplitude of wave */
        spectrum->offsets[i] = cargf(fourier_transform[i]) / signal->rate; /* Calculate phase offset of the wave */
    }

    return spectrum;
}

void delete_spectrum(Spectrum *spectrum) {
    /* Deletes a spectrum object */
    free(spectrum->amplitudes);
    free(spectrum->offsets);
    free(spectrum);
}
