#include <gtk/gtk.h>
#include "manager.h"
#include "../core/db.h"
#include "../core/auth.h"
#include <sqlite3.h>
#include "../crypto/simplecrypt.h"
#include <stdio.h>
#include <string.h>

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
    char password[256];
    GtkWidget *label;
} ViewPasswordData;

typedef struct
{
    char site[128];
    char login[128];
    GtkListBox *list_box;
    GtkWidget *login_entry;
    GtkWidget *password_entry;
    GtkWidget *old_password_entry;
    GtkWindow *dialog;
    char stored_encrypted_password[256];
    int stored_encrypted_len;
} SaveEditData;

static void add_passwords_to_list(GtkListBox *list_box);
static void on_save_password(GtkButton *button, gpointer user_data);
static void on_delete_clicked(GtkButton *button, gpointer user_data);
static void on_edit_clicked(GtkButton *button, gpointer user_data);
static void on_edit_save_clicked(GtkButton *button, gpointer user_data);
static void on_back_clicked(GtkButton *button, gpointer user_data);
static void on_view_password_clicked(GtkButton *button, gpointer user_data);
static void on_verify_password_ok(GtkButton *button, gpointer user_data);

GtkWidget *create_manager_layout()
{
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

    GtkWidget *btn_back = gtk_button_new_with_label("Retour");
    gtk_box_append(GTK_BOX(box), btn_back);
    g_signal_connect(btn_back, "clicked", G_CALLBACK(on_back_clicked), NULL);

    return box;
}

static void on_save_password(GtkButton *button, gpointer user_data)
{
    GtkWidget **entries = (GtkWidget **)user_data;
    const char *site = gtk_editable_get_text(GTK_EDITABLE(entries[0]));
    const char *login = gtk_editable_get_text(GTK_EDITABLE(entries[1]));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(entries[2]));

    if (site && site[0] && login && login[0] && password && password[0])
    {
        char key[33] = {0};
        if (!auth_get_encryption_key(key))
        {
            GtkAlertDialog *alert = gtk_alert_dialog_new("Erreur clé de chiffrement utilisateur");
            gtk_alert_dialog_show(alert, GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button))));
            return;
        }

        char encrypted[256] = {0};
        simplecrypt_encrypt(key, password, encrypted, strlen(password));
        db_add_password(site, login, encrypted);

        GtkWidget *list_box = g_object_get_data(G_OBJECT(button), "list_box");
        if (list_box)
        {
            gtk_list_box_remove_all(GTK_LIST_BOX(list_box));
            add_passwords_to_list(GTK_LIST_BOX(list_box));
        }
    }
}

static void on_delete_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    DeleteData *data = (DeleteData *)user_data;
    db_delete_password(data->site, data->login);
    gtk_list_box_remove_all(data->list_box);
    add_passwords_to_list(data->list_box);
}

static void on_edit_save_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    SaveEditData *sd = (SaveEditData *)user_data;
    const char *new_login = gtk_editable_get_text(GTK_EDITABLE(sd->login_entry));
    const char *new_password = gtk_editable_get_text(GTK_EDITABLE(sd->password_entry));
    const char *old_password_input = gtk_editable_get_text(GTK_EDITABLE(sd->old_password_entry));

    char key[33] = {0};
    if (!auth_get_encryption_key(key))
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("Erreur clé de chiffrement utilisateur");
        gtk_alert_dialog_show(alert, sd->dialog);
        return;
    }

    int enc_len = sd->stored_encrypted_len;
    char decrypted[256] = {0};
    simplecrypt_decrypt(key, sd->stored_encrypted_password, decrypted, enc_len);
    decrypted[enc_len] = '\0';

    if (strcmp(decrypted, old_password_input) != 0)
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("Ancien mot de passe incorrect.");
        gtk_alert_dialog_show(alert, sd->dialog);
        return;
    }

    if (new_login && new_login[0] && new_password && new_password[0])
    {
        char encrypted[256] = {0};
        simplecrypt_encrypt(key, new_password, encrypted, strlen(new_password));
        db_update_entry(sd->site, sd->login, new_login, encrypted);
        gtk_list_box_remove_all(sd->list_box);
        add_passwords_to_list(sd->list_box);
    }

    if (sd->dialog)
        gtk_window_close(sd->dialog);
}

static void on_edit_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
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

    GtkWidget *login_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(login_entry), data->login);
    gtk_entry_set_placeholder_text(GTK_ENTRY(login_entry), "Identifiant");
    gtk_box_append(GTK_BOX(vbox), login_entry);

    GtkWidget *old_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(old_password_entry), "Ancien mot de passe du site");
    gtk_entry_set_visibility(GTK_ENTRY(old_password_entry), FALSE);
    gtk_box_append(GTK_BOX(vbox), old_password_entry);

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
    sd->old_password_entry = old_password_entry;
    sd->dialog = dialog;

    const char *owner = auth_get_current_user();
    sqlite3_stmt *stmt = NULL;
    const char *q = "SELECT password FROM passwords WHERE owner = ? AND site = ? AND login = ?;";
    if (sqlite3_prepare_v2(db, q, -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, owner ? owner : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, data->site, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, data->login, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const void *enc = sqlite3_column_blob(stmt, 0);
            int enc_len = sqlite3_column_bytes(stmt, 0);
            if (enc && enc_len > 0 && enc_len < (int)sizeof(sd->stored_encrypted_password))
            {
                memcpy(sd->stored_encrypted_password, enc, enc_len);
                sd->stored_encrypted_password[enc_len] = '\0';
                sd->stored_encrypted_len = enc_len;
            }
        }
        sqlite3_finalize(stmt);
    }

    g_object_set_data_full(G_OBJECT(dialog), "save-edit-data", sd, (GDestroyNotify)g_free);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_edit_save_clicked), sd);
    g_signal_connect_swapped(btn_cancel, "clicked", G_CALLBACK(gtk_window_close), dialog);

    gtk_window_present(dialog);
}

static void add_passwords_to_list(GtkListBox *list_box)
{
    sqlite3_stmt *stmt = NULL;
    const char *owner = auth_get_current_user();
    const char *q = "SELECT site, login, password FROM passwords WHERE owner = ?;";
    if (sqlite3_prepare_v2(db, q, -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, owner ? owner : "", -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *site = (const char *)sqlite3_column_text(stmt, 0);
            const char *login = (const char *)sqlite3_column_text(stmt, 1);
            const void *password_blob = sqlite3_column_blob(stmt, 2);
            int password_len = sqlite3_column_bytes(stmt, 2);
            char password[256] = {0};
            if (password_blob && password_len > 0 && password_len < 256)
            {
                memcpy(password, password_blob, password_len);
                password[password_len] = '\0';
            }

            GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

            char buf[512];
            snprintf(buf, sizeof(buf), "Site: %s | Identifiant: %s | Mot de passe: ********",
                     site ? site : "", login ? login : "");
            GtkWidget *label = gtk_label_new(buf);
            gtk_widget_add_css_class(label, "password-label");
            gtk_box_append(GTK_BOX(row_box), label);

            GtkWidget *view_button = gtk_button_new_with_label("Voir");
            ViewPasswordData *vdata = g_new0(ViewPasswordData, 1);
            strncpy(vdata->site, site ? site : "", sizeof(vdata->site) - 1);
            strncpy(vdata->login, login ? login : "", sizeof(vdata->login) - 1);
            strncpy(vdata->password, password, sizeof(vdata->password) - 1);
            vdata->label = label;
            g_object_set_data_full(G_OBJECT(view_button), "view-data", vdata, (GDestroyNotify)g_free);
            g_signal_connect(view_button, "clicked", G_CALLBACK(on_view_password_clicked), vdata);
            gtk_box_append(GTK_BOX(row_box), view_button);

            GtkWidget *edit_button = gtk_button_new_with_label("Modifier");
            EditData *edata = g_new0(EditData, 1);
            strncpy(edata->site, site ? site : "", sizeof(edata->site) - 1);
            strncpy(edata->login, login ? login : "", sizeof(edata->login) - 1);
            edata->list_box = list_box;
            g_object_set_data_full(G_OBJECT(edit_button), "edit-data", edata, (GDestroyNotify)g_free);
            g_signal_connect(edit_button, "clicked", G_CALLBACK(on_edit_clicked), edata);
            gtk_box_append(GTK_BOX(row_box), edit_button);

            GtkWidget *delete_button = gtk_button_new_with_label("Supprimer");
            DeleteData *ddata = g_new0(DeleteData, 1);
            strncpy(ddata->site, site ? site : "", sizeof(ddata->site) - 1);
            strncpy(ddata->login, login ? login : "", sizeof(ddata->login) - 1);
            ddata->list_box = list_box;
            g_object_set_data_full(G_OBJECT(delete_button), "delete-data", ddata, (GDestroyNotify)g_free);
            g_signal_connect(delete_button, "clicked", G_CALLBACK(on_delete_clicked), ddata);
            gtk_box_append(GTK_BOX(row_box), delete_button);

            gtk_list_box_append(list_box, row_box);
        }
        sqlite3_finalize(stmt);
    }
}

static void on_back_clicked(GtkButton *button, gpointer user_data)
{
    (void)user_data;
    GtkWidget *win = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    if (win)
        gtk_window_destroy(GTK_WINDOW(win));
}

static void on_verify_password_ok(GtkButton *button, gpointer user_data)
{
    (void)button;
    ViewPasswordData *vdata = (ViewPasswordData *)user_data;
    GtkWidget *entry = g_object_get_data(G_OBJECT(button), "password-entry");
    GtkWidget *dialog = g_object_get_data(G_OBJECT(button), "dialog");

    const char *pwd = gtk_editable_get_text(GTK_EDITABLE(entry));
    const char *user = auth_get_current_user();

    if (!user || !*user || !pwd || !*pwd || !auth_verify_login(user, pwd))
    {
        GtkWidget *err = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(err), "Erreur");
        gtk_window_set_child(GTK_WINDOW(err), gtk_label_new("Mot de passe incorrect."));
        gtk_window_present(GTK_WINDOW(err));
        return;
    }

    char key[33] = {0};
    if (!auth_get_encryption_key(key))
    {
        GtkWidget *err = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(err), "Erreur");
        gtk_window_set_child(GTK_WINDOW(err), gtk_label_new("Erreur clé de chiffrement."));
        gtk_window_present(GTK_WINDOW(err));
        return;
    }

    char display_password[256] = {0};
    size_t pwd_len = strlen(vdata->password);
    simplecrypt_decrypt(key, vdata->password, display_password, pwd_len);
    display_password[pwd_len] = '\0';

    if (!g_utf8_validate(display_password, -1, NULL))
    {
        GtkWidget *err = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(err), "Erreur");
        gtk_window_set_child(GTK_WINDOW(err), gtk_label_new("Impossible de déchiffrer."));
        gtk_window_present(GTK_WINDOW(err));
        if (dialog)
            gtk_window_destroy(GTK_WINDOW(dialog));
        return;
    }

    char buf[1024];
    snprintf(buf, sizeof(buf), "Site: %s | Identifiant: %s | Mot de passe: %s",
             vdata->site, vdata->login, display_password);
    gtk_label_set_text(GTK_LABEL(vdata->label), buf);

    if (dialog)
        gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_view_password_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    ViewPasswordData *vdata = (ViewPasswordData *)user_data;

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Vérification");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 150);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    gtk_box_append(GTK_BOX(box), gtk_label_new("Entrez votre mot de passe maître :"));

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Mot de passe");
    gtk_box_append(GTK_BOX(box), entry);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    GtkWidget *btn_cancel = gtk_button_new_with_label("Annuler");
    GtkWidget *btn_ok = gtk_button_new_with_label("Valider");
    gtk_box_append(GTK_BOX(button_box), btn_cancel);
    gtk_box_append(GTK_BOX(button_box), btn_ok);
    gtk_box_append(GTK_BOX(box), button_box);

    g_object_set_data(G_OBJECT(btn_ok), "password-entry", entry);
    g_object_set_data(G_OBJECT(btn_ok), "dialog", dialog);
    g_signal_connect(btn_ok, "clicked", G_CALLBACK(on_verify_password_ok), vdata);
    g_signal_connect_swapped(btn_cancel, "clicked", G_CALLBACK(gtk_window_destroy), dialog);

    gtk_window_present(GTK_WINDOW(dialog));
}