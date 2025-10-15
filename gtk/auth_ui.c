// Clean implementation of auth UI with password confirmation
#include <gtk/gtk.h>
#include "auth_ui.h"
#include "ui.h"
#include "../core/auth.h"
#include <string.h>

typedef struct
{
    GtkWidget *window;
    GtkWidget *user;
    GtkWidget *pass;
    GtkWidget *pass2;
    void (*ok)(GtkWidget *);
} Ctx;

static void show_error(GtkWidget *parent, const char *msg)
{
    GtkWidget *d = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(d), "Erreur");
    gtk_window_set_child(GTK_WINDOW(d), gtk_label_new(msg));
    gtk_window_present(GTK_WINDOW(d));
}

static void on_login(GtkButton *b, gpointer data)
{
    Ctx *c = (Ctx *)data;
    const char *u = gtk_editable_get_text(GTK_EDITABLE(c->user));
    const char *p = gtk_editable_get_text(GTK_EDITABLE(c->pass));
    if (!u || !*u || !p || !*p)
    {
        show_error(c->window, "Champs requis");
        return;
    }
    if (auth_verify_login(u, p))
    {
        if (c->ok)
            c->ok(c->window);
    }
    else
    {
        show_error(c->window, "Identifiants invalides");
    }
}

static void on_register(GtkButton *b, gpointer data)
{
    Ctx *c = (Ctx *)data;
    const char *u = gtk_editable_get_text(GTK_EDITABLE(c->user));
    const char *p = gtk_editable_get_text(GTK_EDITABLE(c->pass));
    const char *p2 = gtk_editable_get_text(GTK_EDITABLE(c->pass2));
    if (!u || !*u || !p || !*p || !p2 || !*p2)
    {
        show_error(c->window, "Champs requis");
        return;
    }
    if (strcmp(p, p2) != 0)
    {
        show_error(c->window, "Les mots de passe ne correspondent pas");
        return;
    }
    if (auth_username_exists(u))
    {
        show_error(c->window, "Nom d'utilisateur déjà pris");
        return;
    }
    const int iterations = 100000;
    if (auth_register_user(u, p, iterations))
    {
        if (c->ok)
            c->ok(c->window);
    }
    else
    {
        const char *emsg = auth_get_last_error();
        if (!emsg || !*emsg)
            emsg = "Inscription échouée";
        show_error(c->window, emsg);
    }
}

GtkWidget *create_auth_layout(GtkApplication *app, GtkWidget *main_window, void (*on_authenticated)(GtkWidget *window))
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    gtk_box_append(GTK_BOX(box), gtk_label_new("Connexion / Inscription"));
    GtkWidget *entry_user = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_user), "Nom d'utilisateur");
    gtk_box_append(GTK_BOX(box), entry_user);

    GtkWidget *entry_pass = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass), "Mot de passe");
    gtk_entry_set_visibility(GTK_ENTRY(entry_pass), FALSE);
    gtk_box_append(GTK_BOX(box), entry_pass);

    GtkWidget *entry_pass2 = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_pass2), "Confirmer le mot de passe");
    gtk_entry_set_visibility(GTK_ENTRY(entry_pass2), FALSE);
    gtk_box_append(GTK_BOX(box), entry_pass2);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *btn_login = gtk_button_new_with_label("Se connecter");
    GtkWidget *btn_register = gtk_button_new_with_label("S'inscrire");
    gtk_box_append(GTK_BOX(row), btn_login);
    gtk_box_append(GTK_BOX(row), btn_register);
    gtk_box_append(GTK_BOX(box), row);

    Ctx *ctx = g_new0(Ctx, 1);
    ctx->window = main_window;
    ctx->user = entry_user;
    ctx->pass = entry_pass;
    ctx->pass2 = entry_pass2;
    ctx->ok = on_authenticated;

    g_signal_connect(btn_login, "clicked", G_CALLBACK(on_login), ctx);
    g_signal_connect(btn_register, "clicked", G_CALLBACK(on_register), ctx);
    g_object_set_data_full(G_OBJECT(box), "auth-ctx", ctx, (GDestroyNotify)g_free);

    return box;
}
