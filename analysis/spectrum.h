/* spectrum.h - contains functions for spectral analysis */
#ifndef SPECTRUM_H
#define SPECTRUM_H
#include <math.h>
#include <complex.h>
#include "audio.h"
typedef struct spectrum_s {
    /* Holds the fourier transform of a signal */
    float *amplitudes; /* Amplitudes of the stuff */
    float *offsets; /* Phase offsets */
    float resolution; /* Frequency resolution */
    int frequencies; /* Number of frequencies */
} Spectrum; /* The fourier transform struct */

complex float *fft(float *samples, int length);
Spectrum *discrete_fourier_transform(Signal *signal);
float *generate_spectrogram(Signal *signal, int fft_samples, int offset);
void delete_spectrum(Spectrum *spectrum);
#endif
