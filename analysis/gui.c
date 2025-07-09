/* gui.c - generates a graphical user interface for the analyser */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "audio.h"
#include "gui.h"

Analyser *init_analyser() {
    /* Creates the object and inits it */
    Analyser *analyser = (Analyser*)malloc(sizeof(Analyser)); /* Create one */
    analyser->synthesised = 0; /* Recorded Audio */
    analyser->recording = 0; /* No file loaded yet */
    analyser->synthesis = 0; /* Implement later */
    analyser->clip_start = 0; /* No start */
    analyser->clip_end = 0; /* Same */
    return analyser;
}

void app_start(GtkApplication *app, gpointer *user_data) {
    /* The thing that is called when the app is started */
    Analyser *analyser = init_analyser(); /* Create a new analyser object */

    // Create the main window
    GtkWidget *window = gtk_application_window_new(app); /* Create a new window for the app */
    gtk_window_set_title(GTK_WINDOW (window), "Audio Analyser Tool"); /* Set window title */
    gtk_window_set_default_size(GTK_WINDOW (window), 1536, 768); /* Some nice round numbers */

    // Put stuff inside it
    GtkWidget *scrolling = gtk_scrolled_window_new(0, 0); /* Create a scrollable window */
    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); /* Create a box */

    gtk_box_pack_start(GTK_BOX (container), audio_view(analyser), 0, 0, 20); /* A new box */
    
    gtk_container_add(GTK_CONTAINER (window), scrolling); /* Add the scrolled window inside */
    gtk_container_add(GTK_CONTAINER (scrolling), container); /* Add the container inside */
    gtk_widget_show_all(window); /* Display the window */
}

int launch_gui(int argc, char **argv) {
    /* Launches the gui for the analyser */
    GtkApplication *app; /* This is the app thing for the whatever we're about to launch */
    app = gtk_application_new(APP_ID, 0); /* Create a new application */
    g_signal_connect(app, "activate", G_CALLBACK (app_start), 0); /* Connect the app start signal */
    int status = g_application_run(G_APPLICATION(app), argc, argv); /* Run the app */
    g_object_unref(app); /* Delete this */
    return status; /* So that nothing goes wrong */
}
