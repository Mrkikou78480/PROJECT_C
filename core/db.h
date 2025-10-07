#ifndef DB_H
#define DB_H

#include <sqlite3.h>

extern sqlite3 *db;

int db_init(const char *filename);
int db_add_password(const char *site, const char *login, const char *password);
int db_delete_password(const char *site, const char *login);
int db_update_password(const char *site, const char *login, const char *new_password);
int db_update_entry(const char *site, const char *old_login, const char *new_login, const char *new_password);
void db_close();

#endif