/* audio_gui.c - contains code to visualise the audio file */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "audio.h"
#include "load_file.h"
#include "gui.h"

void file_uploaded_callback(GtkFileChooserButton *button, Analyser *analyser) {
    /* Load a file from the chooser */
    
    // First retrieve the file
    GFile *file_object = gtk_file_chooser_get_file(GTK_FILE_CHOOSER (button)); /* Get the filename */
    if (!file_object) return; /* Make sure the file path is not null */

    // Try loading it.
    char *file_path = g_file_get_path(file_object); /* Get the file path */
    RecordInfo *recording = (RecordInfo*)malloc(sizeof(RecordInfo)); /* Create a recording object */
    recording->signal = 0; /* For testing later */
    recording->signal = load_wav(file_path); /* Try to load in the audio file */
    if (!recording->signal) { /* Cleanup time ! */
        free(recording);
        g_object_unref(file_object);
        return;
    }
    strcpy(recording->file_name, g_file_get_basename(file_object)); /* Donewithfilefornow */
    g_object_unref(file_object); /* Done */

    // Replace older one.
    if (analyser->recording) { /* If there was one before */
        delete_signal(analyser->recording->signal); free(analyser->recording); /* Free it */
    }
    analyser->recording = recording; /* Add the new one */

    // Update the text
    
    GtkWidget *previous_box = gtk_widget_get_parent(GTK_WIDGET (button)); /* Get the parent */    
    GtkWidget *text = (GtkWidget*)(gtk_container_get_children(GTK_CONTAINER (previous_box))->next->data); /* Get the text widget */
    char target_text[512]; sprintf(target_text, "Audio File - %s, Sample Rate - %dhz, Length - %fs", recording->file_name, recording->signal->rate, (float)(recording->signal->length) / recording->signal->rate); /* Calc the text */
    gtk_label_set_text(GTK_LABEL(text), target_text); /* Update the label */
    gtk_widget_show_all(text); /* Display it */

    // Update clipping
    analyser->clip_end = recording->signal->length; /* Full */
}

void draw_waveform(GtkWidget *drawing_area, cairo_t *canvas, Analyser *analyser) {
    /* Updates the drawing of the waveform */
    // First, calculate the size of the canvas.
    int size_x = gtk_widget_get_allocated_width(drawing_area); /* Calculate the width of the canvas */
    int size_y = gtk_widget_get_allocated_height(drawing_area); /* Calculate the height of the canvas */

    // Now, start by drawing a background.
    cairo_set_source_rgb(canvas, 0.21875, 0.08203125, 0.359375); /* Set the colour of the background */
    cairo_rectangle(canvas, 0, 0, size_x, size_y); /* Fill the thing with this rect */
    cairo_fill(canvas); /* Do the actual drawing */

    if (!analyser->recording) return; /* Don't draw more */
    // Now, one by one, calculate the samples inside a pixel and draw a line from max to min */
    cairo_set_antialias(canvas, CAIRO_ANTIALIAS_NONE);
    float samples_per_pixel = (float)analyser->recording->signal->length / size_x; /* Samples per pixel */
    for (int index = 0; index < size_x; index++) { /* Loop through all the pixels in the y axis */
        int sample_start = (int)(samples_per_pixel * index); /* Where to start */
        int sample_end = (int)(samples_per_pixel * index + samples_per_pixel); /* Where to end */
        float max = analyser->recording->signal->samples[sample_start]; /* Max sample */
        float min = analyser->recording->signal->samples[sample_start]; /* Max sample */
        // Find the max and min samples
        for (int sample = sample_start; sample < sample_end; sample++) { /* Loop through all in range */
            float sample_value = analyser->recording->signal->samples[sample]; /* Get the sample value */
            if (sample_value > max) max = sample_value; /* Update max */
            if (sample_value < min) min = sample_value; /* Update min */
        }
        // Now time to draw the line.
        cairo_set_line_width(canvas, 1); /* A 1px line */
        cairo_set_source_rgb(canvas, 0.9, 0.9, 0.9); /* Nearly white */
        cairo_move_to(canvas, index, (size_y / 2) - (int)(max * (float)size_y * 0.5)); /* Move to the max point */
        cairo_line_to(canvas, index, (size_y / 2) - (int)(min * (float)size_y * 0.5)); /* Move to the min point */
        cairo_stroke(canvas); /* Draw the stroke */
    }
    cairo_move_to(canvas, 0, size_y / 2); /* Start drawing main line */
    cairo_line_to(canvas, size_x, size_y / 2); /* Start drawing main line */
    cairo_stroke(canvas); /* Draw the stroke */
}

guint update_waveform_drawing(GtkWidget *canvas) {
    /* Remember to keep updating this */
    if (!GTK_IS_WIDGET(canvas)) return 0;
    gtk_widget_queue_draw(canvas); /* Draw */
    return 1; /* Call this again */
}

GtkWidget *audio_view(Analyser *analyser) {
    /* This widget contains the view that allows loading of audio clips and displaying them */

    GtkWidget *container; /* Contains the whole view */
    GtkWidget *top_row, *file_upload, *file_info; /* Contains the button and the text */
    GtkWidget *bottom_row, *canvas; /* Contains the bottom row and the render canvas */

    // Create Widgets
    container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); /* Another one */
    top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); 
    file_upload = gtk_file_chooser_button_new("Load Audio File", GTK_FILE_CHOOSER_ACTION_OPEN); /* Create the file loader button */
    file_info = gtk_label_new("Audio File - [NONE], Sample Rate - [NONE], Length - [NONE]"); /* Holds info */
    bottom_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); /* Add this */
    canvas = gtk_drawing_area_new();  /* Create a new drawing area for this */
    gtk_widget_set_size_request(canvas, -1, 128); /* Set the height of this thing */
    g_signal_connect(canvas, "draw", G_CALLBACK (draw_waveform), analyser); /* Connect draw signal */
    g_idle_add((GSourceFunc)update_waveform_drawing, canvas); /* Keep updating the drawing */

    // Configure Chooser
    GtkFileFilter *filter = gtk_file_filter_new(); /* Create a new filter */
    gtk_file_filter_add_pattern(filter, "*.wav"); /* Configure only wav files */
    gtk_file_filter_add_pattern(filter, "*.wave"); /* Configure only wav files */
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER (file_upload), filter); /* Add the filter */
    g_signal_connect(file_upload, "file-set", G_CALLBACK (file_uploaded_callback), analyser); /* Connect the upload signal */

    // Pack widgets
    gtk_box_pack_start(GTK_BOX (container), top_row, 0, 0, 10); /* Put it at the beginning */
    gtk_box_pack_start(GTK_BOX (top_row), file_upload, 0, 0, 20); /* Add the file button */
    gtk_box_pack_end(GTK_BOX (top_row), file_info, 0, 0, 20); /* Add the file info */
    gtk_box_pack_start(GTK_BOX (container), bottom_row, 0, 0, 10); /* Add the waveform */
    gtk_box_pack_start(GTK_BOX (bottom_row), canvas, 1, 1, 20); /* Add the waveform drawing */

    return container; /* remember */
}
