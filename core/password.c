#include "password.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

char *generate_password(int length, int num_special, int num_upper, int num_digit)
{
    static char buffer[129];
    const char *special = "!@#$%^&*()-_=+[]{}|;:,.<>?";
    const char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *lower = "abcdefghijklmnopqrstuvwxyz";
    const char *digit = "0123456789";
    int i = 0, pos = 0;

    if (length > 128)
        length = 128;
    if (num_special + num_upper + num_digit > length)
        return NULL;

    /* Seed rand() once to avoid reseeding on every call (which can
       produce the same sequence when called multiple times per second). */
    static int seeded = 0;
    if (!seeded)
    {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }

    for (i = 0; i < num_special; i++)
        buffer[pos++] = special[rand() % (int)strlen(special)];

    for (i = 0; i < num_upper; i++)
        buffer[pos++] = upper[rand() % (int)strlen(upper)];

    for (i = 0; i < num_digit; i++)
        buffer[pos++] = digit[rand() % (int)strlen(digit)];

    for (i = pos; i < length; i++)
        buffer[pos++] = lower[rand() % (int)strlen(lower)];

    for (i = 0; i < length; i++)
    {
        int j = rand() % length;
        char tmp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = tmp;
    }

    buffer[length] = '\0';
    return buffer;
}