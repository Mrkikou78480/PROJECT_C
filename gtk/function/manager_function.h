#ifndef MANAGER_FUNCTION_H
#define MANAGER_FUNCTION_H

#include <gtk/gtk.h>

void on_delete_clicked(GtkButton *button, gpointer user_data);
void on_save_password(GtkButton *button, gpointer user_data);
void add_passwords_to_list(GtkListBox *list_box);

#endif