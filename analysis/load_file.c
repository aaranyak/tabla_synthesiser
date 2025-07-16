/* load_file.c - loads the data from a .wav file into a signal */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "audio.h"

typedef struct wav_chunk { /* Holds .wav file chunk data */
    char id[4]; /* Chunk id */
    int length; /* Chunk length */
    char *data; /* Chunk data */
} WavChunk;

int read_wav_headers(char *file_data, int file_length) {
    /* Reads the wav file headers and runs some checks on them */
    if (file_length < 12) return 1; /* Check length */
    char riff[5] = "    ", wave[5] = "    "; /* Checkstrings */
    memcpy(riff, file_data, 4); memcpy(wave, file_data + 8, 4); /* Copy this stuff */
    if (strcmp(riff, "RIFF") || strcmp(wave, "WAVE")) return 1; /* Check these values */
    return 0; /* Everything alright */
}

void read_wav_chunks(char *file_data, int file_length, WavChunk chunks[10]) {
    /* Dis kod reeds een da choonks */
    int read_head = 12; /* Skip riff stuff and get to reading the wav */
    int current_chunk = 0; /* The index of the current chunk */
    while (read_head < file_length) { /* Until we find the end of the file */
        if (file_length < read_head + 8) break; /* Check for off-by-one errors */
        WavChunk *chunk = &chunks[current_chunk]; /* Get the current chunk */
        memcpy(chunk->id, file_data + read_head, 4); read_head += 4; /* Copy in the id */
        memcpy(&chunk->length, file_data + read_head, 4); read_head += 4; /* Copy in the chunk length */
        chunk->data = file_data + read_head; /* Pointer to the chunk thing */
        read_head += chunk->length; /* Move on to the next chunk */
        current_chunk++; /* Next Choonk */
    }
    chunks[current_chunk].length = 0; /* pseudo-null-terminator */
}

WavChunk *get_wav_chunk(WavChunk chunks[10], char chunk_id[4]) {
    /* Get the chunk with the given id */
    int current_chunk = -1; /* The current chunk */
    while (chunks[++current_chunk].length) { /* Until we reach our pseudo-null-terminator */
        if (!strncmp(chunks[current_chunk].id, chunk_id, 4)) return &chunks[current_chunk]; /* Check if this is the choonk we want */
    } return 0; /* No choonk */
}

int decode_fmt_chunk(WavChunk *fmt_chunk, unsigned int *format, unsigned int *sample_rate, unsigned int *channels) {
    /* Decode the format and get the above info */
    int read_head = 0; /* Read head for reading stuff of an unorganized buffer */
    if (fmt_chunk->length < 16) return 1; /* Error! Error! */
    uint16_t pcm_check = 0; memcpy(&pcm_check, fmt_chunk->data + read_head, 2); read_head += 2; /* Read in the pcm check */
    if (pcm_check != 1) return 1; /* Don't support formats other than pcm */
    *channels = 0; memcpy(channels, fmt_chunk->data + read_head, 2); read_head += 2; /* Read in the number of channels */
    memcpy(sample_rate, fmt_chunk->data + read_head, 4); read_head += 4; /* Read in the sample rate */
    read_head += 6; /* Skip two values I don't care about */
    *format = 0; memcpy(format, fmt_chunk->data + read_head, 2); /* Read in the bits per sample */
    return 0; /* No error */
}

float sample_to_float(char *sample, int format) {
    /* Converts a sample of given size to a float between 1 and -1 */
    switch (format) { /* Different code depending on the format */
        case 8: { /* If this is an 8bit thing */
            unsigned char raw_value = (unsigned char)*sample; /* Convert it to an unsigned */
            return (float)raw_value / 128.0 - 1.0; /* Convert that to a value between 1 and -1 */
        } case 16: { /* If this is a 16bit value */
            int16_t raw_value; memcpy(&raw_value, sample, 2); /* Read in the sample value */
            return (float)raw_value / 32768.0; /* Convert to float */
        } case 24: { /* This is slightly harder than the others */
            int32_t raw_value = 0; memcpy(&raw_value, sample, 3); /* Copy in the raw value */
            if (raw_value >> 23) raw_value |= 0xFF000000; /* Correct the sign of the value */
            return (float)raw_value / 8388608.0; /* Convert it to a float */
        } case 32: { /* This is the easiest of them all */
            int raw_value; memcpy(&raw_value, sample, 4); /* Copy in the raw value */
            return (float)raw_value / 2147483648.0; /* Convert to a float between -1 and 1 */
        } default: return 0;
    }
}

Signal *load_wav(char *file_path) {
    /* Parses a .wav file */
    int file_length; char *file_data; /* stuffs */
    FILE *handle = fopen(file_path, "rb"); /* Read from the file */
    if (!handle) return 0; /* File not found error */
    for (file_length = 0; !feof(handle); fgetc(handle)) file_length++; rewind(handle); /* Get the file length */
    file_data = (char*)malloc(file_length); for (int i = 0; i < file_length; i++) file_data[i] = fgetc(handle); /* load file */
    fclose(handle); /* Now what? */
    
    // Read headers
    int error; /* Error checking flag */
    error = read_wav_headers(file_data, file_length); /* Check to make sure nothing went wrong */
    if (error) goto wav_parse_error; /* Error check */

    // Get a list of chunks
    WavChunk chunks[10]; read_wav_chunks(file_data, file_length, chunks); /* Read in the chunks from the file */

    // Parse the fmt
    WavChunk *fmt_chunk = get_wav_chunk(chunks, "fmt "); /* Find the format chunk */
    if (!fmt_chunk) goto wav_parse_error; /* Error check */
    
    unsigned int format, sample_rate, channels; /* Stuff we get from the parser */
    error = decode_fmt_chunk(fmt_chunk, &format, &sample_rate, &channels); /* Parse the format chunk */ 
    if (error) goto wav_parse_error; /* Error check */

    // Now What? (ZZ9 Plural Z Alpha)

    // Parse the data chunk
    WavChunk *data_chunk = get_wav_chunk(chunks, "data"); /* Find the format chunk */
    if (!data_chunk) goto wav_parse_error; /* Error check */
    int num_samples = data_chunk->length / (channels * (format >> 3)); /* Calculate the number of samples */
    Signal *signal = empty_signal(sample_rate, num_samples); /* Create a signal with given properties */
    int sample_size = format >> 3; /* Calculate sample size in bytes */
    char *sample; /* Pointer to the sample */
    for (int index = 0; index < num_samples; index++) { /* Loop through all the samples */
        sample = data_chunk->data + index * sample_size * channels; /* Get the position of the sample */
        signal->samples[index] = sample_to_float(sample, format); /* Convert the sample to a float and add it */
    } 
    free(file_data); /* Get rid of this stuff */
    return signal; /* Oh Well... */

wav_parse_error: /* In case the parsing fails */
    printf("Sorry, the wav file could not be parsed\n");
    free(file_data); /* Clean up */
    return 0;
}
