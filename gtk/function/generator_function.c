#include <gtk/gtk.h>
#include "../../core/db.h"
#include <sqlite3.h>
#include <string.h>
#include "../generator.h"
#include "../manager.h"
#include "generator_function.h"

void on_generate_password(GtkButton *button, gpointer user_data) 
{
    GtkWidget **params = (GtkWidget **)user_data;
    int taille = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[0]));
    int nb_special = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[1]));
    int nb_upper = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[2]));
    int nb_digit = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[3]));

    char *pwd = generate_password(taille, nb_special, nb_upper, nb_digit);

    GtkWidget *dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Mot de passe généré : %s", pwd);
    gtk_window_present(GTK_WINDOW(dialog));
}