/* audio.h - contains basic definitions for audio stuff */
#ifndef AUDIO_H
#define AUDIO_H

typedef struct signal_s { /* This is the struct containing some basic signal info */
    float *samples; /* Sample values */
    size_t length; /* Number of samples */
    int rate; /* Sample rate - samples per second */
} Signal;

Signal *empty_signal(int rate, size_t length);
void delete_signal(Signal *signal);
int add_sine_wave(Signal *signal, float amplitude, float frequency, float phase);
int superimpose(Signal *target, Signal *other);
int change_amplitude(Signal *signal, float factor);
void render_samples(Signal *signal, int offset, int samples, int together, int height, int scale);

#endif

