#include <gtk/gtk.h>
#include "ui.h"
#include "generator.h"
#include "manager.h"

// Fenêtre d'accueil avec deux boutons
GtkWidget *create_home_layout(GtkWidget *window)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *label = gtk_label_new("Bienvenue dans le gestionnaire de mots de passe !");
    gtk_box_append(GTK_BOX(box), label);

    GtkWidget *btn_generate = gtk_button_new_with_label("Générer un mot de passe");
    gtk_box_append(GTK_BOX(box), btn_generate);

    GtkWidget *btn_manage = gtk_button_new_with_label("Gérer mes mots de passe");
    gtk_box_append(GTK_BOX(box), btn_manage);

    g_signal_connect(btn_generate, "clicked", G_CALLBACK(show_generator_window), window);
    g_signal_connect(btn_manage, "clicked", G_CALLBACK(show_manager_window), window);

    return box;
}

void show_generator_window(GtkButton *button, gpointer user_data)
{
    GtkWidget *gen_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(gen_window), "Générateur de mot de passe");
    gtk_window_set_default_size(GTK_WINDOW(gen_window), 400, 250);

    GtkWidget *layout = create_generator_layout();
    gtk_window_set_child(GTK_WINDOW(gen_window), layout);

    gtk_window_present(GTK_WINDOW(gen_window));
}

void show_manager_window(GtkButton *button, gpointer user_data)
{
    GtkWidget *mgr_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(mgr_window), "Gestionnaire de mots de passe");
    gtk_window_set_default_size(GTK_WINDOW(mgr_window), 400, 300);

    GtkWidget *layout = create_manager_layout();
    gtk_window_set_child(GTK_WINDOW(mgr_window), layout);

    gtk_window_present(GTK_WINDOW(mgr_window));
}