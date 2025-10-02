#include <gtk/gtk.h>
#include "manager.h"
#include "../core/db.h"
#include <sqlite3.h>
#include <string.h>

// Structure pour transmettre les infos au callback
typedef struct {
    char site[128];
    char login[128];
    GtkListBox *list_box;
} DeleteData;

// Callback pour supprimer un mot de passe
void on_delete_clicked(GtkButton *button, gpointer user_data) {
    DeleteData *data = (DeleteData *)user_data;
    db_delete_password(data->site, data->login);
    // Rafraîchir la liste
    gtk_list_box_remove_all(data->list_box);
    add_passwords_to_list(data->list_box);
    g_free(data);
}

// Callback pour enregistrer un mot de passe
void on_save_password(GtkButton *button, gpointer user_data) {
    GtkWidget **entries = (GtkWidget **)user_data;
    const char *site = gtk_editable_get_text(GTK_EDITABLE(entries[0]));
    const char *login = gtk_editable_get_text(GTK_EDITABLE(entries[1]));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(entries[2]));

    if (site[0] && login[0] && password[0]) {
        db_add_password(site, login, password);

        // Message d'information GTK4
        GtkWidget *info = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(info), "Info");
        GtkWidget *label = gtk_label_new("Mot de passe enregistré !");
        gtk_window_set_child(GTK_WINDOW(info), label);
        gtk_window_present(GTK_WINDOW(info));

        // Rafraîchir la liste
        GtkWidget *list_box = g_object_get_data(G_OBJECT(button), "list_box");
        if (list_box) {
            gtk_list_box_remove_all(GTK_LIST_BOX(list_box));
            add_passwords_to_list(GTK_LIST_BOX(list_box));
        }
    }
}

void add_passwords_to_list(GtkListBox *list_box) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT site, login, password FROM passwords;", -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *site = (const char *)sqlite3_column_text(stmt, 0);
            const char *login = (const char *)sqlite3_column_text(stmt, 1);
            const char *password = (const char *)sqlite3_column_text(stmt, 2);

            GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

            char buf[512];
            snprintf(buf, sizeof(buf), "Site: %s | Identifiant: %s | Mot de passe: %s", site, login, password);
            GtkWidget *label = gtk_label_new(buf);
            gtk_box_append(GTK_BOX(row_box), label);

            GtkWidget *delete_button = gtk_button_new_with_label("Supprimer");
            DeleteData *data = g_new(DeleteData, 1);
            strncpy(data->site, site, sizeof(data->site));
            strncpy(data->login, login, sizeof(data->login));
            data->list_box = list_box;
            g_signal_connect(delete_button, "clicked", G_CALLBACK(on_delete_clicked), data);
            gtk_box_append(GTK_BOX(row_box), delete_button);

            gtk_list_box_append(list_box, row_box);
        }
        sqlite3_finalize(stmt);
    }
}

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