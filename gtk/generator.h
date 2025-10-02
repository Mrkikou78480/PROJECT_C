#ifndef GENERATOR_H
#define GENERATOR_H

#include <gtk/gtk.h>

GtkWidget *create_generator_layout();
void on_generate_password(GtkButton *button, gpointer user_data);

#endif