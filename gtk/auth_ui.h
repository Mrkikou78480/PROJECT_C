#ifndef AUTH_UI_H
#define AUTH_UI_H

#include <gtk/gtk.h>

GtkWidget *create_welcome_layout(GtkApplication *app, GtkWidget *main_window, void (*on_authenticated)(GtkWidget *window));

GtkWidget *create_login_layout(GtkApplication *app, GtkWidget *main_window, void (*on_authenticated)(GtkWidget *window));

GtkWidget *create_register_layout(GtkApplication *app, GtkWidget *main_window, void (*on_authenticated)(GtkWidget *window));

#endif
