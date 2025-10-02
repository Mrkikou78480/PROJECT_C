#ifndef MANAGER_H
#define MANAGER_H

#include <gtk/gtk.h>

GtkWidget *create_manager_layout();
void on_save_password(GtkButton *button, gpointer user_data);
void add_passwords_to_list(GtkListBox *list_box);

#endif