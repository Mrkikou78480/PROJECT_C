#include "db.h"
#include <sqlite3.h>
#include "auth.h"
#include "../crypto/simplecrypt.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    sqlite3_exec(db, "ALTER TABLE passwords ADD COLUMN owner TEXT;", 0, 0, 0);
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
void db_migrate_encrypt_passwords()
{
    const char *owner = auth_get_current_user();
    if (!owner || !*owner)
        return;

    char key[33] = {0};
    if (!auth_get_encryption_key(key))
        return;

    sqlite3_stmt *stmt = NULL;
    const char *q = "SELECT site, login, password FROM passwords WHERE owner = ?;";
    if (sqlite3_prepare_v2(db, q, -1, &stmt, NULL) != SQLITE_OK)
        return;

    sqlite3_bind_text(stmt, 1, owner, -1, SQLITE_TRANSIENT);

    typedef struct
    {
        char site[128];
        char login[128];
        char password[256];
    } PasswordEntry;
    PasswordEntry *entries = NULL;
    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *site = (const char *)sqlite3_column_text(stmt, 0);
        const char *login = (const char *)sqlite3_column_text(stmt, 1);
        const void *pwd_blob = sqlite3_column_blob(stmt, 2);
        int pwd_len = sqlite3_column_bytes(stmt, 2);

        if (!pwd_blob || pwd_len <= 0 || pwd_len >= 256)
            continue;

        char password[256] = {0};
        memcpy(password, pwd_blob, pwd_len);
        password[pwd_len] = '\0';

        int is_plaintext = 1;
        for (int j = 0; j < pwd_len; j++)
        {
            unsigned char c = (unsigned char)password[j];
            if (c < 32 || c == 127)
            {
                is_plaintext = 0;
                break;
            }
        }

        if (is_plaintext)
        {
            entries = realloc(entries, sizeof(PasswordEntry) * (count + 1));
            strncpy(entries[count].site, site, sizeof(entries[count].site) - 1);
            strncpy(entries[count].login, login, sizeof(entries[count].login) - 1);
            strncpy(entries[count].password, password, sizeof(entries[count].password) - 1);
            count++;
        }
    }
    sqlite3_finalize(stmt);

    for (int i = 0; i < count; i++)
    {
        char encrypted[256] = {0};
        simplecrypt_encrypt(key, entries[i].password, encrypted, strlen(entries[i].password));
        db_update_password(entries[i].site, entries[i].login, encrypted);
    }

    if (entries)
        free(entries);
}

void db_close()
{
    if (db)
        sqlite3_close(db);
}