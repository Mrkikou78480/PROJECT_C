#include <gtk/gtk.h>
#include "../../core/db.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include "../manager.h"
#include "manager_function.h"

typedef struct
{
    char site[128];
    char login[128];
    GtkListBox *list_box;
} DeleteData;

typedef struct
{
    char site[128];
    char login[128];
    GtkListBox *list_box;
} EditData;

typedef struct
{
    char site[128];
    char login[128];
    GtkListBox *list_box;
    GtkWidget *login_entry;
    GtkWidget *password_entry;
    GtkWindow *dialog;
} SaveEditData;
void on_delete_clicked(GtkButton *button, gpointer user_data)
{
    DeleteData *data = (DeleteData *)user_data;
    db_delete_password(data->site, data->login);
    gtk_list_box_remove_all(data->list_box);
    add_passwords_to_list(data->list_box);
}
static void on_edit_save_clicked(GtkButton *button, gpointer user_data)
{
    SaveEditData *sd = (SaveEditData *)user_data;
    const char *new_login = gtk_editable_get_text(GTK_EDITABLE(sd->login_entry));
    const char *new_password = gtk_editable_get_text(GTK_EDITABLE(sd->password_entry));

    if (new_login && new_login[0] && new_password && new_password[0])
    {
        db_update_entry(sd->site, sd->login, new_login, new_password);
        gtk_list_box_remove_all(sd->list_box);
        add_passwords_to_list(sd->list_box);
    }
    if (sd->dialog)
    {
        gtk_window_close(sd->dialog);
    }
}
void on_edit_clicked(GtkButton *button, gpointer user_data)
{
    EditData *data = (EditData *)user_data;
    GtkWindow *dialog = GTK_WINDOW(gtk_window_new());
    gtk_window_set_title(dialog, "Modifier le mot de passe");

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);
    gtk_window_set_child(dialog, vbox);

    char title[256];
    snprintf(title, sizeof(title), "Site: %s", data->site);
    GtkWidget *info = gtk_label_new(title);
    gtk_box_append(GTK_BOX(vbox), info);
    /* Champ pour l'identifiant */
    GtkWidget *login_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(login_entry), data->login);
    gtk_entry_set_placeholder_text(GTK_ENTRY(login_entry), "Identifiant");
    gtk_box_append(GTK_BOX(vbox), login_entry);
    /* Champ pour le mot de passe */
    GtkWidget *password_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(password_entry), "");
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Nouveau mot de passe");
    gtk_box_append(GTK_BOX(vbox), password_entry);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(vbox), hbox);

    GtkWidget *btn_cancel = gtk_button_new_with_label("Annuler");
    GtkWidget *btn_save = gtk_button_new_with_label("Enregistrer");
    gtk_box_append(GTK_BOX(hbox), btn_cancel);
    gtk_box_append(GTK_BOX(hbox), btn_save);

    SaveEditData *sd = g_new0(SaveEditData, 1);
    strncpy(sd->site, data->site, sizeof(sd->site) - 1);
    strncpy(sd->login, data->login, sizeof(sd->login) - 1);
    sd->list_box = data->list_box;
    sd->login_entry = login_entry;
    sd->password_entry = password_entry;
    sd->dialog = dialog;

    g_object_set_data_full(G_OBJECT(dialog), "save-edit-data", sd, (GDestroyNotify)g_free);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_edit_save_clicked), sd);
    g_signal_connect_swapped(btn_cancel, "clicked", G_CALLBACK(gtk_window_close), dialog);

    gtk_window_present(dialog);
}
void on_save_password(GtkButton *button, gpointer user_data)
{
    GtkWidget **entries = (GtkWidget **)user_data;
    const char *site = gtk_editable_get_text(GTK_EDITABLE(entries[0]));
    const char *login = gtk_editable_get_text(GTK_EDITABLE(entries[1]));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(entries[2]));

    if (site[0] && login[0] && password[0])
    {
        db_add_password(site, login, password);
        GtkWidget *info = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(info), "Info");
        GtkWidget *label = gtk_label_new("Mot de passe enregistré !");
        gtk_window_set_child(GTK_WINDOW(info), label);
        gtk_window_present(GTK_WINDOW(info));

        GtkWidget *list_box = g_object_get_data(G_OBJECT(button), "list_box");
        if (list_box)
        {
            gtk_list_box_remove_all(GTK_LIST_BOX(list_box));
            add_passwords_to_list(GTK_LIST_BOX(list_box));
        }
    }
}
void add_passwords_to_list(GtkListBox *list_box)
{
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT site, login, password FROM passwords;", -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *site = (const char *)sqlite3_column_text(stmt, 0);
            const char *login = (const char *)sqlite3_column_text(stmt, 1);
            const char *password = (const char *)sqlite3_column_text(stmt, 2);

            GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

            char buf[512];
            snprintf(buf, sizeof(buf), "Site: %s | Identifiant: %s | Mot de passe: %s", site, login, password);
            GtkWidget *label = gtk_label_new(buf);
            gtk_box_append(GTK_BOX(row_box), label);
            GtkWidget *edit_button = gtk_button_new_with_label("Modifier");
            EditData *edata = g_new0(EditData, 1);
            strncpy(edata->site, site ? site : "", sizeof(edata->site) - 1);
            strncpy(edata->login, login ? login : "", sizeof(edata->login) - 1);
            edata->list_box = list_box;
            g_object_set_data_full(G_OBJECT(edit_button), "edit-data", edata, (GDestroyNotify)g_free);
            g_signal_connect(edit_button, "clicked", G_CALLBACK(on_edit_clicked), edata);
            gtk_box_append(GTK_BOX(row_box), edit_button);

            GtkWidget *delete_button = gtk_button_new_with_label("Supprimer");
            DeleteData *data = g_new0(DeleteData, 1);

            strncpy(data->site, site ? site : "", sizeof(data->site) - 1);
            data->site[sizeof(data->site) - 1] = '\0';
            strncpy(data->login, login ? login : "", sizeof(data->login) - 1);
            data->login[sizeof(data->login) - 1] = '\0';
            data->list_box = list_box;
            g_object_set_data_full(G_OBJECT(delete_button), "delete-data", data, (GDestroyNotify)g_free);
            g_signal_connect(delete_button, "clicked", G_CALLBACK(on_delete_clicked), data);
            gtk_box_append(GTK_BOX(row_box), delete_button);
            gtk_list_box_append(list_box, row_box);
        }
        sqlite3_finalize(stmt);
    }
}