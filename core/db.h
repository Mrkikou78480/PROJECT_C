#ifndef DB_H
#define DB_H

#include <sqlite3.h>

extern sqlite3 *db;

int db_init(const char *filename);
int db_add_password(const char *site, const char *login, const char *password);
void db_close();

#endif