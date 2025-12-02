#include <gtk/gtk.h>
#include "ui.h"
#include "../core/auth.h"

typedef struct
{
    GtkWidget *old_pass;
    GtkWidget *new_pass;
    GtkWidget *new_pass2;
    GtkWidget *avatar_entry;
} SettingsCtx;

static void show_info(const char *msg)
{
    GtkWidget *w = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(w), "Information");
    gtk_window_set_child(GTK_WINDOW(w), gtk_label_new(msg));
    gtk_window_present(GTK_WINDOW(w));
}

static void on_change_password(GtkButton *button, gpointer user_data)
{
    (void)button;
    SettingsCtx *ctx = (SettingsCtx *)user_data;
    const char *old_p = gtk_editable_get_text(GTK_EDITABLE(ctx->old_pass));
    const char *new_p = gtk_editable_get_text(GTK_EDITABLE(ctx->new_pass));
    const char *new_p2 = gtk_editable_get_text(GTK_EDITABLE(ctx->new_pass2));

    if (!old_p || !*old_p || !new_p || !*new_p || !new_p2 || !*new_p2)
    {
        show_info("Tous les champs sont requis.");
        return;
    }
    if (g_strcmp0(new_p, new_p2) != 0)
    {
        show_info("Les nouveaux mots de passe ne correspondent pas.");
        return;
    }

    const char *user = auth_get_current_user();
    if (!user || !*user)
    {
        show_info("Aucun utilisateur connecté.");
        return;
    }
    const int iterations = 100000;
    if (auth_change_password(user, old_p, new_p, iterations))
        show_info("Mot de passe mis à jour.");
    else
        show_info(auth_get_last_error());
}

static void on_file_chooser_response(GObject *source, GAsyncResult *result, gpointer user_data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    SettingsCtx *ctx = (SettingsCtx *)user_data;

    GFile *file = gtk_file_dialog_open_finish(dialog, result, NULL);
    if (file)
    {
        char *path = g_file_get_path(file);
        if (path)
        {
            gtk_editable_set_text(GTK_EDITABLE(ctx->avatar_entry), path);
            g_free(path);
        }
        g_object_unref(file);
    }
}

static void on_browse_avatar(GtkButton *button, gpointer user_data)
{
    (void)button;
    SettingsCtx *ctx = (SettingsCtx *)user_data;

    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Choisir une image");

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.gif");
    gtk_file_filter_add_pattern(filter, "*.bmp");

    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));

    GtkRoot *root = gtk_widget_get_root(GTK_WIDGET(button));
    GtkWindow *window = GTK_WINDOW(root);

    gtk_file_dialog_open(dialog, window, NULL, on_file_chooser_response, ctx);

    g_object_unref(filter);
    g_object_unref(filters);
    g_object_unref(dialog);
}

static void on_save_avatar(GtkButton *button, gpointer user_data)
{
    (void)button;
    SettingsCtx *ctx = (SettingsCtx *)user_data;
    const char *path = gtk_editable_get_text(GTK_EDITABLE(ctx->avatar_entry));
    const char *user = auth_get_current_user();
    if (!user || !*user)
    {
        show_info("Aucun utilisateur connecté.");
        return;
    }
    if (auth_set_avatar_path(user, path && *path ? path : NULL))
        show_info("Photo de profil mise à jour.");
    else
        show_info(auth_get_last_error());
}

void show_account_settings_window(GtkButton *button, gpointer user_data)
{
    (void)button;
    GtkWidget *parent = GTK_WIDGET(user_data);
    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Paramètres du compte");
    gtk_window_set_default_size(GTK_WINDOW(win), 420, 260);
    gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(parent));

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_window_set_child(GTK_WINDOW(win), box);

    gtk_box_append(GTK_BOX(box), gtk_label_new("Modifier le mot de passe du gestionnaire :"));

    GtkWidget *old_pass = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(old_pass), "Ancien mot de passe");
    gtk_entry_set_visibility(GTK_ENTRY(old_pass), FALSE);
    gtk_box_append(GTK_BOX(box), old_pass);

    GtkWidget *new_pass = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(new_pass), "Nouveau mot de passe");
    gtk_entry_set_visibility(GTK_ENTRY(new_pass), FALSE);
    gtk_box_append(GTK_BOX(box), new_pass);

    GtkWidget *new_pass2 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(new_pass2), "Confirmer le nouveau mot de passe");
    gtk_entry_set_visibility(GTK_ENTRY(new_pass2), FALSE);
    gtk_box_append(GTK_BOX(box), new_pass2);

    GtkWidget *btn_change = gtk_button_new_with_label("Modifier le mot de passe");
    gtk_box_append(GTK_BOX(box), btn_change);

    gtk_box_append(GTK_BOX(box), gtk_label_new("Photo de profil (image locale) :"));

    // Créer une boîte horizontale pour le champ de texte et le bouton Parcourir
    GtkWidget *avatar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(box), avatar_box);

    GtkWidget *avatar_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(avatar_entry), "Chemin vers l'image...");
    gtk_widget_set_hexpand(avatar_entry, TRUE);
    gtk_box_append(GTK_BOX(avatar_box), avatar_entry);

    GtkWidget *btn_browse = gtk_button_new_with_label("Parcourir...");
    gtk_box_append(GTK_BOX(avatar_box), btn_browse);

    GtkWidget *btn_avatar = gtk_button_new_with_label("Enregistrer la photo de profil");
    gtk_box_append(GTK_BOX(box), btn_avatar);

    SettingsCtx *ctx = g_new0(SettingsCtx, 1);
    ctx->old_pass = old_pass;
    ctx->new_pass = new_pass;
    ctx->new_pass2 = new_pass2;
    ctx->avatar_entry = avatar_entry;
    g_signal_connect(btn_change, "clicked", G_CALLBACK(on_change_password), ctx);
    g_signal_connect(btn_browse, "clicked", G_CALLBACK(on_browse_avatar), ctx);
    g_signal_connect(btn_avatar, "clicked", G_CALLBACK(on_save_avatar), ctx);
    g_object_set_data_full(G_OBJECT(win), "settings-ctx", ctx, (GDestroyNotify)g_free);

    gtk_window_present(GTK_WINDOW(win));
}
