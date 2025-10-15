#include <gtk/gtk.h>
#include "main_gtk.h"
#include "ui.h"
#include "auth_ui.h"
#include "../core/auth.h"

// File-scope callback to avoid GCC nested function trampolines on Windows
static void on_ok(GtkWidget *win)
{
    GtkWidget *layout = create_home_layout(win);
    gtk_window_set_child(GTK_WINDOW(win), layout);
}

void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gestionnaire de mots de passe");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 300);

    // Initialize auth DB table (ignore failure silently here; db_init happens in main)
    auth_init_db();

    GtkWidget *auth = create_auth_layout(app, window, on_ok);
    gtk_window_set_child(GTK_WINDOW(window), auth);

    gtk_window_present(GTK_WINDOW(window));
}