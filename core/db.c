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
int db_update_password(const char *site, const char *login, const char *new_password)
{
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE passwords SET password='%s' WHERE site='%s' AND login='%s';", new_password, site, login);
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur modification: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}

int db_update_entry(const char *site, const char *old_login, const char *new_login, const char *new_password)
{
    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE passwords SET login='%s', password='%s' WHERE site='%s' AND login='%s';",
             new_login, new_password, site, old_login);
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur modification entrée: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}
int db_delete_password(const char *site, const char *login)
{
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM passwords WHERE site='%s' AND login='%s';", site, login);
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur suppression: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}
void db_close()
{
    if (db)
        sqlite3_close(db);
}