#ifndef AUTH_H
#define AUTH_H

#include <sqlite3.h>

int auth_init_db();

int auth_register_user(const char *username, const char *password, int iterations);

int auth_verify_login(const char *username, const char *password);

int auth_change_password(const char *username,
                         const char *old_password,
                         const char *new_password,
                         int iterations);

int auth_username_exists(const char *username);

void auth_set_current_user(const char *username);
const char *auth_get_current_user(void);

const char *auth_get_last_error(void);

int auth_set_avatar_path(const char *username, const char *path);
const char *auth_get_current_avatar_path(void);

int auth_get_encryption_key(char out[33]);

#endif
