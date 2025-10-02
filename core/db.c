#include "db.h"
#include <sqlite3.h>
#include <stdio.h>

sqlite3 *db = NULL;

int db_init(const char *filename)
{
    if (sqlite3_open(filename, &db) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur ouverture DB: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    const char *sql = "CREATE TABLE IF NOT EXISTS passwords (site TEXT, login TEXT, password TEXT);";
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur création table: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}

int db_add_password(const char *site, const char *login, const char *password)
{
    char sql[512];
    snprintf(sql, sizeof(sql), "INSERT INTO passwords VALUES ('%s', '%s', '%s');", site, login, password);
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur insertion: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}

void db_close()
{
    if (db)
        sqlite3_close(db);
}