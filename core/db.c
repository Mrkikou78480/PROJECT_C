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
int db_reencrypt_passwords_for_current_user(const char *old_key, const char *new_key)
{
    const char *owner = auth_get_current_user();
    if (!owner || !*owner || !old_key || !new_key)
        return 0;

    sqlite3_stmt *stmt = NULL;
    const char *q = "SELECT site, login, password FROM passwords WHERE owner = ?;";
    if (sqlite3_prepare_v2(db, q, -1, &stmt, NULL) != SQLITE_OK)
        return 0;
    sqlite3_bind_text(stmt, 1, owner, -1, SQLITE_TRANSIENT);

    typedef struct
    {
        char site[128];
        char login[128];
        int pwd_len;
        char encrypted[256];
    } Row;

    Row *rows = NULL;
    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *site = (const char *)sqlite3_column_text(stmt, 0);
        const char *login = (const char *)sqlite3_column_text(stmt, 1);
        const void *penc = sqlite3_column_blob(stmt, 2);
        int enc_len = sqlite3_column_bytes(stmt, 2);
        if (!site || !login || !penc || enc_len <= 0 || enc_len >= 256)
            continue;

        rows = realloc(rows, sizeof(Row) * (count + 1));
        strncpy(rows[count].site, site, sizeof(rows[count].site) - 1);
        rows[count].site[sizeof(rows[count].site) - 1] = '\0';
        strncpy(rows[count].login, login, sizeof(rows[count].login) - 1);
        rows[count].login[sizeof(rows[count].login) - 1] = '\0';
        rows[count].pwd_len = enc_len;
        memcpy(rows[count].encrypted, penc, enc_len);
        rows[count].encrypted[enc_len] = '\0';
        count++;
    }
    sqlite3_finalize(stmt);

    int ok = 1;
    for (int i = 0; i < count; ++i)
    {
        char decrypted[256] = {0};
        simplecrypt_decrypt(old_key, rows[i].encrypted, decrypted, (size_t)rows[i].pwd_len);

        int looks_text = 1;
        for (int k = 0; k < rows[i].pwd_len; ++k)
        {
            unsigned char c = (unsigned char)decrypted[k];
            if (c < 32 || c == 127)
            {
                looks_text = 0;
                break;
            }
        }
        if (!looks_text)
        {
            ok = 0;
            break;
        }

        char reenc[256] = {0};
        simplecrypt_encrypt(new_key, decrypted, reenc, (size_t)rows[i].pwd_len);

        char sql[768];
        snprintf(sql, sizeof(sql), "UPDATE passwords SET password='%s' WHERE owner='%s' AND site='%s' AND login='%s';",
                 reenc, owner, rows[i].site, rows[i].login);
        if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
        {
            ok = 0;
            break;
        }
    }

    if (rows)
        free(rows);
    return ok;
}

void db_close()
{
    if (db)
        sqlite3_close(db);
}