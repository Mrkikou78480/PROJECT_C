#include <gtk/gtk.h>
#include "../core/password.h"
#include "generator.h"

GtkWidget *create_generator_layout()
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *entry_taille = gtk_spin_button_new_with_range(4, 128, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_taille), 12);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Taille du mot de passe"));
    gtk_box_append(GTK_BOX(box), entry_taille);

    GtkWidget *entry_special = gtk_spin_button_new_with_range(0, 20, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_special), 2);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Nombre de caractères spéciaux"));
    gtk_box_append(GTK_BOX(box), entry_special);

    GtkWidget *entry_upper = gtk_spin_button_new_with_range(0, 20, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_upper), 2);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Nombre de majuscules"));
    gtk_box_append(GTK_BOX(box), entry_upper);

    GtkWidget *entry_digit = gtk_spin_button_new_with_range(0, 20, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_digit), 2);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Nombre de chiffres"));
    gtk_box_append(GTK_BOX(box), entry_digit);

    GtkWidget *generate_btn = gtk_button_new_with_label("Générer");
    gtk_box_append(GTK_BOX(box), generate_btn);

    GtkWidget **params = g_new(GtkWidget *, 4);
    params[0] = entry_taille;
    params[1] = entry_special;
    params[2] = entry_upper;
    params[3] = entry_digit;
    g_signal_connect(generate_btn, "clicked", G_CALLBACK(on_generate_password), params);

    return box;
}

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