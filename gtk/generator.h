#ifndef GENERATOR_H
#define GENERATOR_H

#include <gtk/gtk.h>

GtkWidget *create_generator_layout();
char *generate_password(int taille, int nb_special, int nb_upper, int nb_digit);

#endif