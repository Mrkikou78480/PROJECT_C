#ifndef AUTH_UI_H
#define AUTH_UI_H

#include <gtk/gtk.h>

// Creates and returns a widget that contains login/register UI.
// On successful login or registration, it invokes a callback to show the main UI.
GtkWidget *create_auth_layout(GtkApplication *app, GtkWidget *main_window, void (*on_authenticated)(GtkWidget *window));

#endif
