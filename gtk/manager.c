#include <gtk/gtk.h>
#include "manager.h"
#include "../core/db.h"
#include <sqlite3.h>
#include <string.h>
#include "function/manager_function.h"

GtkWidget *create_manager_layout() {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *entry_site = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_site), "Site");
    gtk_box_append(GTK_BOX(box), entry_site);

    GtkWidget *entry_login = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_login), "Identifiant");
    gtk_box_append(GTK_BOX(box), entry_login);

    GtkWidget *entry_password = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_password), "Mot de passe");
    gtk_box_append(GTK_BOX(box), entry_password);

    GtkWidget *btn_save = gtk_button_new_with_label("Enregistrer le mot de passe");
    gtk_box_append(GTK_BOX(box), btn_save);

    GtkWidget **entries = g_new(GtkWidget *, 3);
    entries[0] = entry_site;
    entries[1] = entry_login;
    entries[2] = entry_password;

    GtkWidget *list_box = gtk_list_box_new();
    gtk_box_append(GTK_BOX(box), gtk_label_new("Mots de passe enregistrés :"));
    gtk_box_append(GTK_BOX(box), list_box);
    add_passwords_to_list(GTK_LIST_BOX(list_box));
    g_object_set_data(G_OBJECT(btn_save), "list_box", list_box);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_password), entries);

    return box;
}