#include <gtk/gtk.h>
#include "../core/password.h"
#include "generator.h"

static void on_generate_password(GtkButton *button, gpointer user_data);
static void on_back_clicked(GtkButton *button, gpointer user_data);

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

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(button_box, 10);

    GtkWidget *generate_btn = gtk_button_new_with_label("Générer");
    GtkWidget *back_btn = gtk_button_new_with_label("Retour");

    gtk_box_append(GTK_BOX(button_box), back_btn);
    gtk_box_append(GTK_BOX(button_box), generate_btn);
    gtk_box_append(GTK_BOX(box), button_box);

    GtkWidget **params = g_new(GtkWidget *, 4);
    params[0] = entry_taille;
    params[1] = entry_special;
    params[2] = entry_upper;
    params[3] = entry_digit;
    g_signal_connect(generate_btn, "clicked", G_CALLBACK(on_generate_password), params);
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), NULL);

    return box;
}

static void on_generate_password(GtkButton *button, gpointer user_data)
{
    (void)button;
    GtkWidget **params = (GtkWidget **)user_data;
    int taille = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[0]));
    int nb_special = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[1]));
    int nb_upper = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[2]));
    int nb_digit = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[3]));

    char *pwd = generate_password(taille, nb_special, nb_upper, nb_digit);

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Mot de passe généré");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);

    char msg[512];
    snprintf(msg, sizeof(msg), "Mot de passe généré : %s", pwd);
    gtk_box_append(GTK_BOX(box), gtk_label_new(msg));

    GtkWidget *btn_ok = gtk_button_new_with_label("OK");
    gtk_box_append(GTK_BOX(box), btn_ok);
    g_signal_connect_swapped(btn_ok, "clicked", G_CALLBACK(gtk_window_destroy), dialog);

    gtk_window_set_child(GTK_WINDOW(dialog), box);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_back_clicked(GtkButton *button, gpointer user_data)
{
    (void)user_data;
    GtkWidget *win = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    if (win)
        gtk_window_destroy(GTK_WINDOW(win));
}
