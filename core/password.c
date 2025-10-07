#include "password.h"
#include <stdlib.h>
#include <time.h>
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

    srand((unsigned int)time(NULL));

    for (i = 0; i < num_special; i++)
        buffer[pos++] = special[rand() % (int)(sizeof(special) - 1)];

    for (i = 0; i < num_upper; i++)
        buffer[pos++] = upper[rand() % (int)(sizeof(upper) - 1)];

    for (i = 0; i < num_digit; i++)
        buffer[pos++] = digit[rand() % (int)(sizeof(digit) - 1)];

    for (i = pos; i < length; i++)
        buffer[pos++] = lower[rand() % (int)(sizeof(lower) - 1)];

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