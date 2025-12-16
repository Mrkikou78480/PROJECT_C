#ifndef CONFIG_H
#define CONFIG_H

typedef struct
{
    char db_path[256];
    char theme_default[32];
    char theme_light_css[256];
    char theme_dark_css[256];
    int gen_default_length;
    int gen_default_special;
    int gen_default_upper;
    int gen_default_digit;
    int win_main_width;
    int win_main_height;
    int win_generator_width;
    int win_generator_height;
    int win_manager_width;
    int win_manager_height;
    int win_settings_width;
    int win_settings_height;
} AppConfig;

extern AppConfig g_config;

int config_load(const char *filename);
void config_set_defaults(void);

#endif
