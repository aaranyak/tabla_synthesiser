/* amplitudes.c 
    Superposition: I contain amplitudes 
    -> YEAAAHHHHH
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include "audio.h"
#include "spectrum.h"
#include "gui.h"
#include "filter.h"

#define min(x, y) ((x) < (y) ? (x) : (y)) /* In case the fmin causes float problems */

void identify_dominant_frequencies(Signal *signal, float *frequencies, int num_freqs, float min_freq, float max_freq, int margin) {
    /* This isolates the frequencies with the highest amplitudes */

    // First, cast þe spell and prepeare þe en-chanted solution
/*  Double, double toil and trouble; Fire burn and caldron bubble.
    Fillet of a fenny snake, In the caldron boil and bake;
    Eye of newt and toe of frog, Wool of bat and tongue of dog,
    Adder's fork and blind-worm's sting, Lizard's leg and howlet's wing,
    For a charm of powerful trouble, Like a hell-broth boil and bubble.

    Double, double toil and trouble; Fire burn and caldron bubble.
    Cool it with a baboon's blood, Then the charm is firm and good.
*/
    // Now unleash þe Faste Fouryer Transforme
    Spectrum *spectrum = discrete_fourier_transform(signal); /* This returns some frequency and amplitude information */
    int min_index = min_freq / spectrum->resolution, max_index = min(max_freq / spectrum->resolution, spectrum->frequencies); /* Calculate the index range */
    for (int pos_index = 0; pos_index < num_freqs; pos_index++) { /* Loop through the number of frequencies */
        float current_max = 0; int current_index = 0; /* Just like that */
        for (int freq_index = min_index; freq_index < max_index; freq_index++) { /* Search between min and max index */
            float current_amplitude = spectrum->amplitudes[freq_index]; /* Get the current amplitude */
            float current_frequency = freq_index * spectrum->resolution; /* Calculate the current frequency */
            int skipthis = 0; for (int i = 0; i < pos_index; i++) if (abs(current_frequency - frequencies[i]) < margin) skipthis = 1;
            if (skipthis) continue; /* To check if this one is in a margin */
            if (current_amplitude > current_max) { /* We've bested */
                current_max = current_amplitude; /* Update the maximum */
                current_index = freq_index; /* Update the corresponding index */
            }
        } /* Now we have calculated the next maximum */
        frequencies[pos_index] = current_index * spectrum->resolution; /* Add the current frequency */
    }
}

void generate_amplitude_graph(Signal *signal, float *graph, float frequency, float demod_lpf, float width, int length) { 
    /* Calculates the graph values for the amplitude of one frequency component in that signal */
    Signal *component = copy_signal(signal); /* This will hold the component frequency */
    Filter *highpass = highpass_filter(length, frequency - width, component->rate); /* Calculate the highpass of the bandpass */
    Filter *lowpass = lowpass_filter(length, frequency + width, component->rate); /* Calculate the lowpass of the bandpass */
    fft_convolve(component, highpass); fft_convolve(component, lowpass); /* Isolate that component frequency using a bandpass */

    // Use an envelope demodulation technique
    for (int i = 0; i < component->length; i++)  /* Loop through all the samples in the signal */
        component->samples[i] = fabs(component->samples[i]); /* Take the absolute value */

    // Smooth out the peaks using an LPF
    Filter *demod_smooth = lowpass_filter(length, demod_lpf, component->rate); /* Calculate the lowpass for demodulation */
    fft_convolve(component, demod_smooth); /* Apply this lowpass */

    for (int i = 0; i < component->length; i++) /* Loop through all the samples in the signal */
        graph[i] = component->samples[i]; /* This should be obvious */

    delete_filter(highpass); delete_filter(lowpass); delete_filter(demod_smooth); delete_signal(component); /* Clean up */
}

void update_freq_onoff_callback(GtkWidget *button, int state, Analyser *analyser) {
    /* This updates the onoff of that frequency */
    GtkWidget *outside = gtk_widget_get_parent(gtk_widget_get_parent(button)); /* Get the outer widget */
    int freq_index = g_list_index(gtk_container_get_children(GTK_CONTAINER (analyser->freq_flow)), outside); /* Get index of freq */
    analyser->freq_onoffs[freq_index] = state; /* Update the state */
    gtk_widget_queue_draw(analyser->graph_canvas); /* Update the graph */
}
 
void generate_amplitudes(GtkButton *button, Analyser *analyser) {
    /* Runs all the generation required */

    // Identify the top n frequencies in the signal
    Signal *signal = trim_signal(analyser->recording->signal, analyser->clip_start, analyser->clip_end); /* Trim the signal */
    int num_freqs = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->search_num)); /* Get the number of frequencies to isolate */
    float min_freq = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->search_min)); /* Get the minimum frequency to start from */
    float max_freq = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->search_max)); /* Get the maximum to end the search at */
    float margin = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->search_margin)); /* Get the maximum to end the search at */
    float filter_width = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->filter_width)); /* Get the frequency domain width of the bandpass */
    float demod_lpf = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->demod_lpf)); /* Get the frequency of the demod lpf */
    int filter_length = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->filter_length)); /* Get the length of the filter */
    if (analyser->load_amplitudes) { /* Clear everything */
        free(analyser->dominant_frequencies);
        free(analyser->freq_onoffs); /* A nice name */
        free(analyser->freq_colours); /* For assigning some colours */
        free(analyser->amplitude_graphs); /* The actual data */
    }
    analyser->dominant_frequencies = (float*)malloc(sizeof(float) * num_freqs); /* This and that and all the normal stuff */
    identify_dominant_frequencies(signal, analyser->dominant_frequencies, num_freqs, min_freq, max_freq, margin); /* Put them in the array given there */

    // Put those in the GUI
    analyser->freq_onoffs = (int*)malloc(sizeof(int) * num_freqs); /* Malloc is an inherent truth that C# people will never understand */
    memset(analyser->freq_onoffs, 0, sizeof(int) * num_freqs);
    if (analyser->load_amplitudes) /* We need to clear all the frequency switches */
        while (gtk_container_get_children(GTK_CONTAINER (analyser->freq_flow))) /* As long as there are children */
            gtk_widget_destroy((GtkWidget*)(gtk_container_get_children(GTK_CONTAINER (analyser->freq_flow))->data)); /* delete them */
    analyser->freq_colours = (float*)malloc(sizeof(float) * num_freqs * 2); /* HSV colour for this */
    for (int index = 0; index < num_freqs; index++) { /* Loop through all the frequencies */
        GtkWidget *display_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); /* Create a new box */
        char display_text[32]; sprintf(display_text, "%.02fhz", analyser->dominant_frequencies[index]); /* Format the text */
        GtkWidget *onoff = gtk_switch_new(); /* Put this here */
        gtk_box_pack_start(GTK_BOX (display_container), onoff, 0, 0, 0); /* Put this in the beginning */
        gtk_box_pack_end(GTK_BOX (display_container), gtk_label_new(display_text), 0, 0, 5); /* Add this */
        gtk_container_add(GTK_CONTAINER (analyser->freq_flow), display_container); /* Add this to the flow box */
        gtk_widget_show_all(display_container); /* So that the new widgets are visible */
        g_signal_connect(onoff, "state-set", G_CALLBACK (update_freq_onoff_callback), analyser); /* Add updater */
        analyser->freq_colours[index << 1] = g_random_double(); /* Initialise a random hsv colour for the freq */
        analyser->freq_colours[index << 1 & 1] = g_random_double(); /* But with maximum saturation */
    }

    // Now, to do the amplitude graphs for each of these.
    analyser->amplitude_graphs = (float*)malloc(sizeof(float) * signal->length * num_freqs); /* This array is technically 2d */
    for (int freq_index = 0; freq_index < num_freqs; freq_index++) /* Loop through all the frequencies */
        generate_amplitude_graph(signal, &analyser->amplitude_graphs[signal->length * freq_index], analyser->dominant_frequencies[freq_index], demod_lpf, filter_width, filter_length); /* Generate that graph */

    analyser->load_amplitudes = 1; /* We have loaded */
    gtk_widget_queue_draw(analyser->graph_canvas); /* Redraw this */
    delete_signal(signal); /* Clean Up */
}    

void draw_amplitudes(GtkWidget *drawing_area, cairo_t *canvas, Analyser *analyser) {
    /* Render all the on frequencies in the amplitudes graph */
    int size_x = gtk_widget_get_allocated_width(drawing_area); /* Calculate the width of the canvas */
    int size_y = gtk_widget_get_allocated_height(drawing_area); /* Calculate the height of the canvas */

    // Render Background
    cairo_set_source_rgb(canvas, 1, 1, 1); /* Colour the screen white */
    cairo_rectangle(canvas, 0, 0, size_x, size_y); /* Draw a rectangle */
    cairo_fill(canvas); /* Do the actual drawing */

    // Now, start drawing the graphs
    if (!analyser->load_amplitudes) return; /* Only continue if the graphstuff have been loaded */
    int num_freqs = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->search_num)); /* Get the number of frequencies to isolate */
    float scale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(analyser->graph_scale)); /* Scaling factor for the amplitudes */
    int time_length = analyser->clip_end - analyser->clip_start; /* Length of the isolated range */

    cairo_set_line_width(canvas, 1); /* A 1px line */
    for (int freq_index = 0; freq_index < num_freqs; freq_index++) { /* Loop through all of the frequencies in the list */
        if (!analyser->freq_onoffs[freq_index]) continue; /* Only render if this frequency is on */
        for (int x = 0; x < size_x; x++) { /* Loop through all the pixels in the canvas */
            int time_index = x * time_length / size_x; /* Calculate the position to index from */
            int time_index_next = (x + 1) * time_length / size_x; /* Calculate the position to index from */
            float amplitude = analyser->amplitude_graphs[freq_index * time_length + time_index] * scale; /* Get the amplitude for this pixel */
            float amplitude_next = analyser->amplitude_graphs[freq_index * time_length + time_index_next] * scale; /* Get the amplitude for this pixel */
            set_cairo_hsv(canvas, analyser->freq_colours[freq_index << 1], 1, analyser->freq_colours[freq_index << 1 + 1]); /* Set the colour of the point */
            cairo_move_to(canvas, x, size_y - amplitude * size_y); /* Start line from this point */
            cairo_line_to(canvas, x + 1, size_y - amplitude_next * size_y); /* End line at this point */
            cairo_stroke(canvas); /* Draw the line */
        }
    }
}

void graph_info_callback(GtkWidget *drawing_area, GdkEventMotion *event, Analyser *analyser) {
    /* Update the mouse data */
    if (!analyser->load_amplitudes) return; /* Convenience */
    int size_x = gtk_widget_get_allocated_width(drawing_area); /* Calculate the width of the canvas */
    int size_y = gtk_widget_get_allocated_height(drawing_area); /* Calculate the height of the canvas */ 
    float graph_scale = gtk_spin_button_get_value(GTK_SPIN_BUTTON (analyser->graph_scale)); /* Get the max freq */
    float time = (float)(analyser->clip_start + ((float)event->x / (float)size_x) * (analyser->clip_end - analyser->clip_start)) / (float)analyser->recording->signal->rate; /* Calculate time */
    float amplitude = (size_y - event->y) / size_y / graph_scale; /* Calculate amplitude */
    char label_text[256]; sprintf(label_text, "Time - %.4f, Amplitude - %.4f", time, amplitude); /* Calc text */
    gtk_label_set_text(GTK_LABEL (analyser->graph_info), label_text); /* Set the text */
}


GtkWidget *amplitudes_view(Analyser *analyser) {
    /* Shows a view of the change in amplitudes of the major frequencies */

    GtkWidget *container, *graph_row, *data_row; /* The thing that returnes everything */
    GtkWidget *buttons_grid, *flow, *flow_container, *search_min, *search_max, *search_num, *filter_length, *filter_width, *generate, *search_margin, *demod_lpf, *graph_scale, *graph_info; /* The frequency searching settings */
    GtkWidget *graph_canvas;
    
    // Create the widgets
    container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); /* Create the main container */
    data_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); /* Create the top row container */
    graph_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0); /* Create the bottom row container */
    flow_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); /* This is so that the flow box resizes properly */
    flow = gtk_flow_box_new(); /* Create a new flow box */
    buttons_grid = gtk_grid_new(); /* Create a grid a layout for this one */
    search_min = gtk_spin_button_new_with_range(0, 44100, 1); /* Spin to set min search freq */
    search_max = gtk_spin_button_new_with_range(0, 44100, 1); /* Spin to set the max search freq */
    search_num = gtk_spin_button_new_with_range(0, 20, 1); /* Spin to set the number of searches */
    search_margin = gtk_spin_button_new_with_range(0, 128, 0.001); /* Gap to leave between frequencies */
    filter_length = gtk_spin_button_new_with_range(256, 1048576, 1); /* Length of the filte */
    filter_width = gtk_spin_button_new_with_range(0, 128, 0.001); /* Filter frequency width */
    demod_lpf = gtk_spin_button_new_with_range(1, 44100, 0.1); /* Demodulator LPF Frequency */
    graph_scale = gtk_spin_button_new_with_range(1, 20, 0.01); /* Amplitude Graph Scale */
    generate = gtk_button_new_with_label("Generate Graph"); /* Do all the required generation */
    graph_info = gtk_label_new("Time - , Amplitude - ");

    // Now for the canvas
    graph_canvas = gtk_drawing_area_new(); /* Create a new drawing area for drawing the graphs in */ 


    // Format stuff
    gtk_grid_set_row_spacing(GTK_GRID (buttons_grid), 10); /* Add some spacing */
    gtk_grid_set_column_spacing(GTK_GRID (buttons_grid), 10); /* Add some spacing */
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX (flow), GTK_SELECTION_NONE); /* No Selection */
    gtk_widget_set_size_request(graph_canvas, -1, 512); /* Make the graph big */

    // Dump everything into the state struct
    analyser->search_min = search_min; analyser->search_max = search_max; analyser->search_num = search_num; analyser->freq_flow = flow; analyser->search_margin = search_margin; analyser->filter_width = filter_width; analyser->filter_length = filter_length; analyser->graph_canvas = graph_canvas; analyser->demod_lpf = demod_lpf; analyser->graph_scale = graph_scale; analyser->graph_info = graph_info;

    // Time to connect signals.
    analyser->load_amplitudes = 0;
    gtk_widget_add_events(graph_canvas, GDK_POINTER_MOTION_MASK); /* Detect mouse motion */
    g_signal_connect(generate, "clicked", G_CALLBACK (generate_amplitudes), analyser); /* There you go */
    g_signal_connect(graph_canvas, "motion-notify-event", G_CALLBACK (graph_info_callback), analyser); /* Connect the mouse signal */
    g_signal_connect(graph_canvas, "draw", G_CALLBACK (draw_amplitudes), analyser); /* The drawing signal */
    g_signal_connect_swapped(graph_scale, "value-changed", G_CALLBACK (gtk_widget_queue_draw), graph_canvas); /* Redraw the graph */
    
    // Pack all the stuff
    gtk_box_pack_start(GTK_BOX (container), data_row, 0, 0, 20); /* Pack in the settings row */
    gtk_box_pack_start(GTK_BOX (data_row), buttons_grid, 0, 0, 20); /* Make this not fill */
    gtk_grid_attach(GTK_GRID (buttons_grid), search_num, 0, 0, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), search_min, 1, 0, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), search_max, 2, 0, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), search_margin, 3, 0, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), filter_length, 0, 1, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), filter_width, 1, 1, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), demod_lpf, 2, 1, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), graph_scale, 3, 1, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), generate, 0, 2, 1, 1); /* Add this to the grid */
    gtk_grid_attach(GTK_GRID (buttons_grid), graph_info, 1, 2, 3, 1); /* Add this to the grid */
    gtk_box_pack_end(GTK_BOX (data_row), flow_container, 1, 1, 20); /* Make this fill */
    gtk_box_pack_end(GTK_BOX (data_row), flow, 1, 1, 20); /* Make this fill */
    gtk_box_pack_start(GTK_BOX (container), graph_row, 0, 0, 20); /* Pack in the graph row */
    gtk_box_pack_start(GTK_BOX (graph_row), graph_canvas, 1, 1, 20); /* Pack in the graph */

    return container; /* because otherwise the program would segfault */
}
