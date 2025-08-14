/* filter.h - contains functions to create and apply filters */
#ifndef FILTER_H
#define FILTER_H
#include <complex.h>
typedef struct filter_s { /* This is the convolution kernel for a filter */
    float *kernel; /* Array of floats filter kernel */
    int length; /* The length of the filter kernel */
    int offset; /* The zero of the filter kernel. */
} Filter;

Filter *delta_function(int length);
complex float *ifft(complex float *samples, int length);

#endif
