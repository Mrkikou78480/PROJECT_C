#include <gtk/gtk.h>
#include "ui.h"
#include "generator.h"
#include "manager.h"
#include "auth_ui.h"
#include "../core/auth.h"
#include "../core/config.h"

static GtkWidget *g_main_window = NULL;

static void on_authenticated_again(GtkWidget *win)
{
    GtkWidget *layout = create_home_layout(win);
    gtk_window_set_child(GTK_WINDOW(win), layout);
}

static void on_logout(GtkButton *btn, gpointer win)
{
    (void)btn;
    auth_set_current_user("");
    GtkApplication *app = gtk_window_get_application(GTK_WINDOW(win));
    GtkWidget *welcome = create_welcome_layout(app, (GtkWidget *)win, on_authenticated_again);
    gtk_window_set_child(GTK_WINDOW(win), welcome);
}

static GtkCssProvider *g_theme_provider = NULL;

static void ensure_theme_provider(void)
{
    if (g_theme_provider)
        return;
    g_theme_provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    if (display)
    {
        gtk_style_context_add_provider_for_display(
            display,
            GTK_STYLE_PROVIDER(g_theme_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
}

static void apply_theme_css_file(const char *css_path)
{
    ensure_theme_provider();
    if (!g_theme_provider)
        return;
    gtk_css_provider_load_from_path(g_theme_provider, css_path);
}

void ui_init_theme(void)
{
    if (strcmp(g_config.theme_default, "dark") == 0)
        apply_theme_css_file(g_config.theme_dark_css);
    else
        apply_theme_css_file(g_config.theme_light_css);
}

void ui_init_session(GtkWidget *main_window)
{
    g_main_window = main_window;
}

static void on_theme_light(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    apply_theme_css_file(g_config.theme_light_css);
}

static void on_theme_dark(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    apply_theme_css_file(g_config.theme_dark_css);
}

GtkWidget *ui_create_theme_switch(void)
{
    GtkWidget *theme_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(theme_box, "theme-switch");
    GtkWidget *light_btn = gtk_button_new_with_label("Thème clair");
    GtkWidget *dark_btn = gtk_button_new_with_label("Thème sombre");
    g_signal_connect(light_btn, "clicked", G_CALLBACK(on_theme_light), NULL);
    g_signal_connect(dark_btn, "clicked", G_CALLBACK(on_theme_dark), NULL);
    gtk_box_append(GTK_BOX(theme_box), light_btn);
    gtk_box_append(GTK_BOX(theme_box), dark_btn);
    return theme_box;
}

GtkWidget *create_home_layout(GtkWidget *window)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    const char *user = auth_get_current_user();
    char buf[256];
    if (user && *user)
        snprintf(buf, sizeof(buf), "Bienvenue %s !", user);
    else
        snprintf(buf, sizeof(buf), "Bienvenue !");

    GtkWidget *welcome_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(welcome_box, GTK_ALIGN_CENTER);

    const char *avatar_path = auth_get_current_avatar_path();
    if (avatar_path && *avatar_path)
    {
        GtkWidget *pic = gtk_picture_new_for_filename(avatar_path);
        gtk_widget_set_size_request(pic, 64, 64);
        gtk_box_append(GTK_BOX(welcome_box), pic);
    }

    GtkWidget *label = gtk_label_new(buf);
    gtk_widget_add_css_class(label, "hero-label");
    gtk_box_append(GTK_BOX(welcome_box), label);

    gtk_box_append(GTK_BOX(box), welcome_box);

    GtkWidget *btn_generate = gtk_button_new_with_label("Générer un mot de passe");
    gtk_box_append(GTK_BOX(box), btn_generate);

    GtkWidget *btn_manage = gtk_button_new_with_label("Gérer mes mots de passe");
    gtk_box_append(GTK_BOX(box), btn_manage);

    GtkWidget *btn_settings = gtk_button_new_with_label("Paramètres du compte");
    gtk_box_append(GTK_BOX(box), btn_settings);

    GtkWidget *btn_logout = gtk_button_new_with_label("Déconnexion");
    gtk_box_append(GTK_BOX(box), btn_logout);

    g_signal_connect(btn_generate, "clicked", G_CALLBACK(show_generator_window), window);
    g_signal_connect(btn_manage, "clicked", G_CALLBACK(show_manager_window), window);
    g_signal_connect(btn_settings, "clicked", G_CALLBACK(show_account_settings_window), window);
    g_signal_connect(btn_logout, "clicked", G_CALLBACK(on_logout), window);

    GtkWidget *theme_box = ui_create_theme_switch();
    gtk_box_append(GTK_BOX(box), theme_box);

    return box;
}

void show_generator_window(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    GtkWidget *gen_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(gen_window), "Générateur de mot de passe");
    gtk_window_set_default_size(GTK_WINDOW(gen_window), g_config.win_generator_width, g_config.win_generator_height);

    GtkWidget *layout = create_generator_layout();
    gtk_window_set_child(GTK_WINDOW(gen_window), layout);

    gtk_window_present(GTK_WINDOW(gen_window));
}

void show_manager_window(GtkButton *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    GtkWidget *mgr_window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(mgr_window), "Gestionnaire de mots de passe");
    gtk_window_set_default_size(GTK_WINDOW(mgr_window), g_config.win_manager_width, g_config.win_manager_height);
    GtkWidget *layout = create_manager_layout();
    gtk_window_set_child(GTK_WINDOW(mgr_window), layout);
    gtk_window_present(GTK_WINDOW(mgr_window));
}