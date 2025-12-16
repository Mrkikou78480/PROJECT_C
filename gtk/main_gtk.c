#include <gtk/gtk.h>
#include "main_gtk.h"
#include "ui.h"
#include "auth_ui.h"
#include "../core/auth.h"
#include "../core/config.h"
static void on_ok(GtkWidget *win)
{

    GtkWidget *layout = create_home_layout(win);
    gtk_window_set_child(GTK_WINDOW(win), layout);
}
void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Gestionnaire de mots de passe");
    gtk_window_set_default_size(GTK_WINDOW(window), g_config.win_main_width, g_config.win_main_height);
    auth_init_db();

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "css/app_theme.css");
    gtk_style_context_add_provider_for_display(
        gtk_widget_get_display(window),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    ui_init_theme();
    ui_init_session(window);
    GtkWidget *welcome = create_welcome_layout(app, window, on_ok);
    gtk_window_set_child(GTK_WINDOW(window), welcome);
    gtk_window_present(GTK_WINDOW(window));
}