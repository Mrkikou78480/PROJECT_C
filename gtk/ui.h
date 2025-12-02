#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>

void ui_init_theme(void);
void ui_init_session(GtkWidget *main_window);
void ui_start_session(void);
void ui_stop_session(void);
GtkWidget *ui_create_theme_switch(void);
GtkWidget *create_home_layout(GtkWidget *window);
void show_generator_window(GtkButton *button, gpointer user_data);
void show_manager_window(GtkButton *button, gpointer user_data);
void show_account_settings_window(GtkButton *button, gpointer user_data);

#endif