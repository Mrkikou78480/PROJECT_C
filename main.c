#include <gtk/gtk.h>
#include "gtk/main_gtk.h"
#include "core/db.h"
#include "core/auth.h"

int main(int argc, char *argv[])
{
    db_init("data/passwords.db");
    auth_init_db();

    GtkApplication *app = gtk_application_new("com.example.PasswordManager", G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    db_close();
    return status;
}