#include <gtk/gtk.h>
#include "main_gtk.h"
#include "ui.h"

void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gestionnaire de mots de passe");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 300);

    GtkWidget *layout = create_home_layout(window);
    gtk_window_set_child(GTK_WINDOW(window), layout);

    gtk_window_present(GTK_WINDOW(window));
}