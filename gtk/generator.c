#include <gtk/gtk.h>
#include "../core/password.h"
#include "generator.h"

static void on_generate_password(GtkButton *button, gpointer user_data);
static void on_back_clicked(GtkButton *button, gpointer user_data);
static void on_taille_changed(GtkSpinButton *spin, gpointer user_data);
static void on_copy_clicked(GtkButton *button, gpointer user_data);

GtkWidget *create_generator_layout()
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *entry_taille = gtk_spin_button_new_with_range(4, 128, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_taille), 12);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Taille du mot de passe"));
    gtk_box_append(GTK_BOX(box), entry_taille);

    GtkWidget *entry_special = gtk_spin_button_new_with_range(0, 12, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_special), 2);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Nombre de caractères spéciaux"));
    gtk_box_append(GTK_BOX(box), entry_special);

    GtkWidget *entry_upper = gtk_spin_button_new_with_range(0, 12, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_upper), 2);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Nombre de majuscules"));
    gtk_box_append(GTK_BOX(box), entry_upper);

    GtkWidget *entry_digit = gtk_spin_button_new_with_range(0, 12, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry_digit), 2);
    gtk_box_append(GTK_BOX(box), gtk_label_new("Nombre de chiffres"));
    gtk_box_append(GTK_BOX(box), entry_digit);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(button_box, 10);

    GtkWidget *generate_btn = gtk_button_new_with_label("Générer");
    GtkWidget *back_btn = gtk_button_new_with_label("Retour");

    gtk_box_append(GTK_BOX(button_box), back_btn);
    gtk_box_append(GTK_BOX(button_box), generate_btn);
    gtk_box_append(GTK_BOX(box), button_box);

    GtkWidget **params = g_new(GtkWidget *, 4);
    params[0] = entry_taille;
    params[1] = entry_special;
    params[2] = entry_upper;
    params[3] = entry_digit;

    g_signal_connect(entry_taille, "value-changed", G_CALLBACK(on_taille_changed), params);

    g_signal_connect(generate_btn, "clicked", G_CALLBACK(on_generate_password), params);
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), NULL);

    return box;
}

static void on_generate_password(GtkButton *button, gpointer user_data)
{
    (void)button;
    GtkWidget **params = (GtkWidget **)user_data;
    int taille = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[0]));
    int nb_special = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[1]));
    int nb_upper = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[2]));
    int nb_digit = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(params[3]));

    if (taille < 4)
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("La taille doit être au minimum 4 caractères.");
        gtk_alert_dialog_show(alert, NULL);
        return;
    }

    if (taille > 128)
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("La taille ne peut pas dépasser 128 caractères.");
        gtk_alert_dialog_show(alert, NULL);
        return;
    }

    if (nb_special < 0 || nb_upper < 0 || nb_digit < 0)
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("Les nombres ne peuvent pas être négatifs.");
        gtk_alert_dialog_show(alert, NULL);
        return;
    }

    if (nb_special > taille)
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("Le nombre de caractères spéciaux ne peut pas dépasser la taille du mot de passe.");
        gtk_alert_dialog_show(alert, NULL);
        return;
    }

    if (nb_upper > taille)
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("Le nombre de majuscules ne peut pas dépasser la taille du mot de passe.");
        gtk_alert_dialog_show(alert, NULL);
        return;
    }

    if (nb_digit > taille)
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("Le nombre de chiffres ne peut pas dépasser la taille du mot de passe.");
        gtk_alert_dialog_show(alert, NULL);
        return;
    }

    int total_required = nb_special + nb_upper + nb_digit;
    if (total_required > taille)
    {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "La somme des contraintes (%d + %d + %d = %d) dépasse la taille du mot de passe (%d).\n"
                 "Réduisez le nombre de caractères spéciaux, majuscules ou chiffres.",
                 nb_special, nb_upper, nb_digit, total_required, taille);
        GtkAlertDialog *alert = gtk_alert_dialog_new(msg);
        gtk_alert_dialog_show(alert, NULL);
        return;
    }

    char *pwd = generate_password(taille, nb_special, nb_upper, nb_digit);

    if (!pwd)
    {
        GtkAlertDialog *alert = gtk_alert_dialog_new("Erreur lors de la génération du mot de passe.");
        gtk_alert_dialog_show(alert, NULL);
        return;
    }

    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Mot de passe généré");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);

    gtk_box_append(GTK_BOX(box), gtk_label_new("Mot de passe généré :"));

    GtkWidget *pwd_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(pwd_entry), FALSE);
    gtk_entry_set_visibility(GTK_ENTRY(pwd_entry), TRUE);
    gtk_editable_set_text(GTK_EDITABLE(pwd_entry), pwd);
    gtk_box_append(GTK_BOX(box), pwd_entry);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);

    GtkWidget *btn_copy = gtk_button_new_with_label("Copier");
    g_object_set_data(G_OBJECT(btn_copy), "pwd-entry", pwd_entry);
    g_signal_connect(btn_copy, "clicked", G_CALLBACK(on_copy_clicked), dialog);

    GtkWidget *btn_ok = gtk_button_new_with_label("OK");
    g_signal_connect_swapped(btn_ok, "clicked", G_CALLBACK(gtk_window_destroy), dialog);

    gtk_box_append(GTK_BOX(btn_box), btn_copy);
    gtk_box_append(GTK_BOX(btn_box), btn_ok);
    gtk_box_append(GTK_BOX(box), btn_box);

    gtk_window_set_child(GTK_WINDOW(dialog), box);
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_back_clicked(GtkButton *button, gpointer user_data)
{
    (void)user_data;
    GtkWidget *win = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW);
    if (win)
        gtk_window_destroy(GTK_WINDOW(win));
}

static void on_taille_changed(GtkSpinButton *spin, gpointer user_data)
{
    GtkWidget **params = (GtkWidget **)user_data;
    int nouvelle_taille = gtk_spin_button_get_value_as_int(spin);

    gtk_spin_button_set_range(GTK_SPIN_BUTTON(params[1]), 0, nouvelle_taille);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(params[2]), 0, nouvelle_taille);
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(params[3]), 0, nouvelle_taille);
}

static void on_copy_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *pwd_entry = g_object_get_data(G_OBJECT(button), "pwd-entry");
    GtkWidget *dialog = GTK_WIDGET(user_data);
    (void)dialog;
    const char *text = gtk_editable_get_text(GTK_EDITABLE(pwd_entry));

    GdkDisplay *display = gdk_display_get_default();
    if (!display)
        return;
    GdkClipboard *clipboard = gdk_display_get_clipboard(display);
    if (clipboard)
        gdk_clipboard_set_text(clipboard, text);

    gtk_button_set_label(GTK_BUTTON(button), "Copié !");
}
