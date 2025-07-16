/* spectrogram.c - code for visualising a spectrogram */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include "audio.h"
#include "spectrum.h"
#include "gui.h"

void spec_gen_callback(GtkButton *button, Analyser *analyser) {
    /* Generates the spectrogram */
    if (!analyser->recording) return; /* Only generate if there is a recording */
    if (analyser->spectrogram) free(analyser->spectrogram); /* Delete the older one */
    analyser->spectrogram = 0; /* For now in case threads mess things up */
    Signal *signal = trim_signal(analyser->recording->signal, analyser->clip_start, analyser->clip_end); /* Trim the signal */
    printf("Number o Samples - %ld\n", signal->length); /* Print no samples. */
    GtkWidget *spinbutton = (GtkWidget*)gtk_container_get_children(GTK_CONTAINER (gtk_widget_get_parent(GTK_WIDGET (button))))->data;
    int spec_offset = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinbutton)); /* Set the value */
    spinbutton = (GtkWidget*)gtk_container_get_children(GTK_CONTAINER (gtk_widget_get_parent(GTK_WIDGET (button))))->next->data;
    analyser->spec_length = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinbutton)); /* Set the value */
    analyser->spectrogram = generate_spectrogram(signal, analyser->spec_length, spec_offset); /* Generate spectrogram */
    analyser->spec_num = (signal->length - analyser->spec_length) / spec_offset; /* Set the number of spectra in it */
    analyser->spec_res = (float)signal->rate / (float)analyser->spec_length; /* Calculate spec res */
    delete_signal(signal); /* Delete the trimmed version */
    gtk_widget_queue_draw(analyser->spec_display); /* YAY */
}

void set_cairo_hsv(cairo_t *canvas, float h, float s, float v) {
    /* Sets the rgb colour from an hsv - code copied from wikipedia. */
    float c = s * v; /* Calculate the chroma */
    h *= 6; /* for somehow we need this */
    float x = c * (1 - fabs(fmod(h, 2) - 1)); /* Some cursed maths I copied from wikipedia */
    float m = v - c; /* Something */
    switch ((int)h) { /* Does differend weird things depending on this */
        case 0:
            cairo_set_source_rgb(canvas, c + m, x + m, m); /* Huh? */ break;
        case 1:
            cairo_set_source_rgb(canvas, x + m, c + m, m); /* wha? */ break;
        case 2:
            cairo_set_source_rgb(canvas, m, c + m, x + m); break;
        case 3:
            cairo_set_source_rgb(canvas, m, x + m, c + m); break;
        case 4:
            cairo_set_source_rgb(canvas, x + m, m, c + m); break;
        case 5:
            cairo_set_source_rgb(canvas, c + m, m, x + m); break;
        case 6:
            cairo_set_source_rgb(canvas, c, 0, x); break;
    }
}
            

void draw_spectrogram(GtkWidget *drawing_area, cairo_t *canvas, Analyser *analyser) {
    /* Draws the spectrogram from the analyser to the canvas */
    int size_x = gtk_widget_get_allocated_width(drawing_area); /* Calculate the width of the canvas */
    int size_y = gtk_widget_get_allocated_height(drawing_area); /* Calculate the height of the canvas */
    float min_freq = gtk_spin_button_get_value(GTK_SPIN_BUTTON (analyser->spec_min_freq)); /* Get the min freq */
    float max_freq = gtk_spin_button_get_value(GTK_SPIN_BUTTON (analyser->spec_max_freq)); /* Get the max freq */
    cairo_set_antialias(canvas, CAIRO_ANTIALIAS_GOOD); /* Make antialiasing good */
    if (!analyser->spectrogram) { /* If there is none */
        cairo_set_source_rgb(canvas, 0, 0, 0); /* Colour the screen black */
        cairo_rectangle(canvas, 0, 0, size_x, size_y); /* Draw a rectangle */
        cairo_fill(canvas); /* Do the actual drawing */
        return; /* I don't like long else clauses */
    }

    // Draw the spectrogram.
    int num_freqs = analyser->spec_length / 2; /* The number of frequencies */
    int freq_offset = min_freq / analyser->spec_res; /* Frequency Offset */
    for (int y = 0; y < size_y; y++) { /* Loop through all the y coordinates */
        for (int x = 0; x < size_x; x++) { /* Loop through all the x values */
            int target_freq =  freq_offset + ((float)y / (float)size_y) * ((max_freq - min_freq) / analyser->spec_res); /* Calculate the current target frequency */
            int target_spec = (x * analyser->spec_num) / size_x; /* Calculate the current target spectrum */
            float amplitude = analyser->spectrogram[target_spec * num_freqs + target_freq] * analyser->spec_scale; /* Get amplitude */
            set_cairo_hsv(canvas, 1.3 - amplitude * 0.7, 1, 0.1 + amplitude * 0.9); /* In Full Colour */
            cairo_rectangle(canvas, x, size_y - y, 1, 1); /* A 1px rect */
            cairo_fill(canvas); /* Yay drawing one pixel */
        } /* Odd indentation I know, but what can I do? */
    }
}

void change_spec_scale(GtkRange *range, Analyser *analyser) {
    /* Updates the clip-start value */
    analyser->spec_scale = gtk_range_get_value(range); /* set the scale */
    gtk_widget_queue_draw(analyser->spec_display); /* Redraw canvas */
}

void mouse_move_callback(GtkWidget *drawing_area, GdkEventMotion *event, Analyser *analyser) {
    /* Update the mouse data */
    if (!analyser->spectrogram) return; /* Convenience */
    int size_x = gtk_widget_get_allocated_width(drawing_area); /* Calculate the width of the canvas */
    int size_y = gtk_widget_get_allocated_height(drawing_area); /* Calculate the height of the canvas */
    float min_freq = gtk_spin_button_get_value(GTK_SPIN_BUTTON (analyser->spec_min_freq)); /* Get the min freq */
    float max_freq = gtk_spin_button_get_value(GTK_SPIN_BUTTON (analyser->spec_max_freq)); /* Get the max freq */

    float frequency = min_freq + ((1 - (float)event->y / (float)size_y)) * (max_freq - min_freq); /* Calculate mouse frequency */
    float time = (float)(analyser->clip_start + ((float)event->x / (float)size_x) * (analyser->clip_end - analyser->clip_start)) / (float)analyser->recording->signal->rate; /* Calculate time */
    char label_text[256]; sprintf(label_text, "Frequency - %.4f, Time - %.4f", frequency, time); /* Calc text */
    gtk_label_set_text(GTK_LABEL (analyser->spec_mouse_info), label_text); /* Set the text */
}

GtkWidget *spectrogram_view(Analyser *analyser) {
    /* View that generates a spectrogram */
    // Declare
    GtkWidget *container; /* The thing that contains everything */
    GtkWidget *top_row, *offset_spin, *length_spin, *min_freq, *max_freq, *generate, *mouse_info; /* The top row stuff like settings and generate button */
    GtkWidget *bottom_row, *canvas, *scale_volume; /* This is the bottom row and canvas where the thing is rendered */
    // Create the widgets
    container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); /* Create a VBox */
    top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); /* Create a HBox */
    offset_spin = gtk_spin_button_new_with_range(1, 2048, 1); /* Create spinbutton for this */ 
    length_spin = gtk_spin_button_new_with_range(64, 16384, 1); /* Create spinbutton for this */
    min_freq = gtk_spin_button_new_with_range(0, 44100, 1); /* Spin to set the min display freq */
    max_freq = gtk_spin_button_new_with_range(0, 44100, 1); /* Spin to set the max display freq */
    generate = gtk_button_new_with_label("Generate Spectrogram"); /* Create the generate button */
    
    bottom_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); /* Another one */
    gtk_box_set_homogeneous(GTK_BOX (bottom_row), 0); /* Make better looking box */
    scale_volume = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.0, 64.0, 0.001); /* Add a scale for helping better specs */
    gtk_range_set_inverted(GTK_RANGE (scale_volume), 1); /* Invert scale */
    gtk_range_set_value(GTK_RANGE (scale_volume), analyser->spec_scale); /* Set this */
    canvas = gtk_drawing_area_new(); /* Figure out the size of this later */
    analyser->spec_display = canvas; analyser->spec_min_freq = min_freq; analyser->spec_max_freq = max_freq;/* Easy updating */
    gtk_widget_set_size_request(canvas, -1, 256); /* Set horizontal size */
    mouse_info = gtk_label_new("Frequency - , Time - "); /* Some info to debug */
    analyser->spec_mouse_info = mouse_info; /* for updating later */

    // Connect signals
    gtk_widget_add_events(canvas, GDK_POINTER_MOTION_MASK); /* Detect mouse motion */
    g_signal_connect(generate, "clicked", G_CALLBACK (spec_gen_callback), analyser); /* Callback to the function that does the spec gen */
    g_signal_connect(canvas, "draw", G_CALLBACK (draw_spectrogram), analyser); /* Connect the draw signal */
    g_signal_connect(canvas, "motion-notify-event", G_CALLBACK (mouse_move_callback), analyser); /* Connect the mouse signal */
    g_signal_connect_swapped(max_freq, "value-changed", G_CALLBACK(gtk_widget_queue_draw), canvas); /* Redraw when changed */
    g_signal_connect_swapped(min_freq, "value-changed", G_CALLBACK(gtk_widget_queue_draw), canvas); /* Redraw when changed */
    g_signal_connect(scale_volume, "value-changed", G_CALLBACK (change_spec_scale), analyser); /* Add callback to update volume scale */

    // Packing time
    gtk_box_pack_start(GTK_BOX (container), top_row, 0, 0, 10); /* Pack in the top row */
    gtk_box_pack_start(GTK_BOX (container), bottom_row, 0, 0, 10); /* Pack in the bottom row */
    gtk_box_pack_start(GTK_BOX (top_row), offset_spin, 1, 0, 20); /* Pack it in the top row */
    gtk_box_pack_start(GTK_BOX (top_row), length_spin, 1, 0, 20); /* Pack it in the top row */
    gtk_box_pack_start(GTK_BOX (top_row), min_freq, 1, 0, 20); /* Pack it in the top row */
    gtk_box_pack_start(GTK_BOX (top_row), max_freq, 1, 0, 20); /* Pack it in the top row */
    gtk_box_pack_start(GTK_BOX (top_row), generate, 1, 0, 20); /* Pack it in the top row */
    gtk_box_pack_start(GTK_BOX (top_row), mouse_info, 1, 0, 20); /* Pack it in the top row */
    gtk_box_pack_start(GTK_BOX (bottom_row), canvas, 1, 1, 20); /* Pack it into the bottom row */
    gtk_box_pack_start(GTK_BOX (bottom_row), scale_volume, 0, 0, 20); /* Pack it into the bottom row */

    return container;
}

