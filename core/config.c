#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

AppConfig g_config;

void config_set_defaults(void)
{
    strcpy(g_config.db_path, "data/passwords.db");
    strcpy(g_config.theme_default, "light");
    strcpy(g_config.theme_light_css, "css/light.css");
    strcpy(g_config.theme_dark_css, "css/dark.css");
    g_config.gen_default_length = 16;
    g_config.gen_default_special = 2;
    g_config.gen_default_upper = 4;
    g_config.gen_default_digit = 4;
    g_config.win_main_width = 600;
    g_config.win_main_height = 500;
    g_config.win_generator_width = 400;
    g_config.win_generator_height = 300;
    g_config.win_manager_width = 700;
    g_config.win_manager_height = 400;
    g_config.win_settings_width = 450;
    g_config.win_settings_height = 300;
}

static void trim(char *str)
{
    char *end;
    while (*str == ' ' || *str == '\t')
        str++;
    if (*str == 0)
        return;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
        end--;
    *(end + 1) = '\0';
}

int config_load(const char *filename)
{
    config_set_defaults();

    FILE *f = fopen(filename, "r");
    if (!f)
        return 0;

    char line[512];
    char section[64] = "";

    while (fgets(line, sizeof(line), f))
    {
        trim(line);
        if (line[0] == '\0' || line[0] == ';' || line[0] == '#')
            continue;

        if (line[0] == '[')
        {
            char *end = strchr(line, ']');
            if (end)
            {
                *end = '\0';
                strncpy(section, line + 1, sizeof(section) - 1);
            }
            continue;
        }

        char *eq = strchr(line, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        trim(key);
        trim(value);

        if (strcmp(section, "Database") == 0)
        {
            if (strcmp(key, "Path") == 0)
                strncpy(g_config.db_path, value, sizeof(g_config.db_path) - 1);
        }
        else if (strcmp(section, "Theme") == 0)
        {
            if (strcmp(key, "Default") == 0)
                strncpy(g_config.theme_default, value, sizeof(g_config.theme_default) - 1);
            else if (strcmp(key, "LightCSS") == 0)
                strncpy(g_config.theme_light_css, value, sizeof(g_config.theme_light_css) - 1);
            else if (strcmp(key, "DarkCSS") == 0)
                strncpy(g_config.theme_dark_css, value, sizeof(g_config.theme_dark_css) - 1);
        }
        else if (strcmp(section, "Generator") == 0)
        {
            if (strcmp(key, "DefaultLength") == 0)
                g_config.gen_default_length = atoi(value);
            else if (strcmp(key, "DefaultSpecial") == 0)
                g_config.gen_default_special = atoi(value);
            else if (strcmp(key, "DefaultUpper") == 0)
                g_config.gen_default_upper = atoi(value);
            else if (strcmp(key, "DefaultDigit") == 0)
                g_config.gen_default_digit = atoi(value);
        }
        else if (strcmp(section, "Window") == 0)
        {
            if (strcmp(key, "MainWidth") == 0)
                g_config.win_main_width = atoi(value);
            else if (strcmp(key, "MainHeight") == 0)
                g_config.win_main_height = atoi(value);
            else if (strcmp(key, "GeneratorWidth") == 0)
                g_config.win_generator_width = atoi(value);
            else if (strcmp(key, "GeneratorHeight") == 0)
                g_config.win_generator_height = atoi(value);
            else if (strcmp(key, "ManagerWidth") == 0)
                g_config.win_manager_width = atoi(value);
            else if (strcmp(key, "ManagerHeight") == 0)
                g_config.win_manager_height = atoi(value);
            else if (strcmp(key, "SettingsWidth") == 0)
                g_config.win_settings_width = atoi(value);
            else if (strcmp(key, "SettingsHeight") == 0)
                g_config.win_settings_height = atoi(value);
        }
    }

    fclose(f);
    return 1;
}
