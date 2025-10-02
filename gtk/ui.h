#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>

GtkWidget *create_home_layout(GtkWidget *window);
void show_generator_window(GtkButton *button, gpointer user_data);
void show_manager_window(GtkButton *button, gpointer user_data);

#endif