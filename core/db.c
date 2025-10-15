#include "db.h"
#include <sqlite3.h>
#include "auth.h"
#include <stdio.h>

sqlite3 *db = NULL;
int db_init(const char *filename)
{
    if (sqlite3_open(filename, &db) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur ouverture DB: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    const char *sql = "CREATE TABLE IF NOT EXISTS passwords (owner TEXT NOT NULL, site TEXT, login TEXT, password TEXT);";
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur création table: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    // Migration: if an old table exists without 'owner', try to add it
    sqlite3_exec(db, "ALTER TABLE passwords ADD COLUMN owner TEXT;", 0, 0, 0); // ignore error if exists
    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_passwords_owner ON passwords(owner);", 0, 0, 0);
    sqlite3_exec(db, "CREATE UNIQUE INDEX IF NOT EXISTS idx_passwords_owner_site_login ON passwords(owner, site, login);", 0, 0, 0);
    return 1;
}
int db_add_password(const char *site, const char *login, const char *password)
{
    const char *owner = auth_get_current_user();
    char sql[640];
    snprintf(sql, sizeof(sql), "INSERT INTO passwords(owner, site, login, password) VALUES ('%s','%s','%s','%s');", owner, site, login, password);
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur insertion: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}
int db_update_password(const char *site, const char *login, const char *new_password)
{
    const char *owner = auth_get_current_user();
    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE passwords SET password='%s' WHERE owner='%s' AND site='%s' AND login='%s';", new_password, owner, site, login);
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur modification: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}

int db_update_entry(const char *site, const char *old_login, const char *new_login, const char *new_password)
{
    const char *owner = auth_get_current_user();
    char sql[768];
    snprintf(sql, sizeof(sql), "UPDATE passwords SET login='%s', password='%s' WHERE owner='%s' AND site='%s' AND login='%s';",
             new_login, new_password, owner, site, old_login);
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "Erreur modification entrée: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    return 1;
}
int db_delete_password(const char *site, const char *login)
{
    const char *owner = auth_get_current_user();
    char sql[512];
    snprintf(sql, sizeof(sql), "DELETE FROM passwords WHERE owner='%s' AND site='%s' AND login='%s';", owner, site, login);
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