#include "auth.h"
#include "db.h"
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "bcrypt.lib")

// Constants
#define SALT_LEN 16
#define HASH_LEN 32 // SHA-256

// Some MinGW environments may miss this flag macro; define it if absent.
#ifndef BCRYPT_ALG_HANDLE_HMAC_FLAG
#define BCRYPT_ALG_HANDLE_HMAC_FLAG 0x00000008
#endif

static char g_auth_last_error[256] = "";
static void set_auth_error(const char *msg)
{
    if (!msg)
    {
        g_auth_last_error[0] = '\0';
        return;
    }
    snprintf(g_auth_last_error, sizeof(g_auth_last_error), "%s", msg);
}
const char *auth_get_last_error(void)
{
    return g_auth_last_error;
}

static char g_auth_current_user[128] = "";
void auth_set_current_user(const char *username)
{
    if (!username)
        username = "";
    snprintf(g_auth_current_user, sizeof(g_auth_current_user), "%s", username);
}
const char *auth_get_current_user(void)
{
    return g_auth_current_user;
}

static int pbkdf2_sha256(const unsigned char *password, size_t password_len,
                         const unsigned char *salt, size_t salt_len,
                         int iterations,
                         unsigned char *out_key, size_t out_len)
{
    int ok = 0;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    // PBKDF2 uses HMAC-SHA256 as PRF => open the hash provider with HMAC flag
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (status != 0 || hAlg == NULL)
    {
        char buf[96];
        snprintf(buf, sizeof(buf), "BCryptOpenAlgorithmProvider failed (0x%08lx)", (unsigned long)status);
        set_auth_error(buf);
        return 0;
    }
    status = BCryptDeriveKeyPBKDF2(
        hAlg,
        (PUCHAR)password, (ULONG)password_len,
        (PUCHAR)salt, (ULONG)salt_len,
        (ULONGLONG)iterations,
        out_key, (ULONG)out_len,
        0);
    ok = (status == 0);
    if (!ok)
    {
        char buf[96];
        snprintf(buf, sizeof(buf), "BCryptDeriveKeyPBKDF2 failed (0x%08lx)", (unsigned long)status);
        set_auth_error(buf);
    }
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return ok;
}

static int random_bytes(unsigned char *buf, size_t len)
{
    if (!buf || len == 0)
    {
        set_auth_error("random_bytes invalid args");
        return 0;
    }
    NTSTATUS s = BCryptGenRandom(NULL, buf, (ULONG)len, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (s != 0)
        set_auth_error("BCryptGenRandom failed");
    return s == 0 ? 1 : 0;
}

int auth_init_db()
{
    const char *sql = "CREATE TABLE IF NOT EXISTS users (\n"
                      "  username TEXT PRIMARY KEY,\n"
                      "  iterations INTEGER NOT NULL,\n"
                      "  salt BLOB NOT NULL,\n"
                      "  hash BLOB NOT NULL\n"
                      ");";
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "auth_init_db error: %s\n", sqlite3_errmsg(db));
        set_auth_error("auth_init_db failed");
        return 0;
    }
    // Optional: create unique index on username if table was created without primary key previously
    rc = sqlite3_exec(db, "CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username ON users(username);", 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "auth_init_db index error: %s\n", sqlite3_errmsg(db));
        // non-fatal
    }
    return 1;
}

int auth_register_user(const char *username, const char *password, int iterations)
{
    if (!username || !password || iterations <= 0)
    {
        set_auth_error("invalid args");
        return 0;
    }
    unsigned char salt[SALT_LEN];
    unsigned char hash[HASH_LEN];
    if (!random_bytes(salt, sizeof(salt)))
        return 0;
    if (!pbkdf2_sha256((const unsigned char *)password, strlen(password), salt, sizeof(salt), iterations, hash, sizeof(hash)))
        return 0;

    const char *sql = "INSERT INTO users (username, iterations, salt, hash) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *st = NULL;
    int prc = sqlite3_prepare_v2(db, sql, -1, &st, NULL);
    if (prc != SQLITE_OK)
    {
        fprintf(stderr, "auth_register_user prepare error: %s\n", sqlite3_errmsg(db));
        set_auth_error("prepare failed");
        return 0;
    }
    sqlite3_bind_text(st, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 2, iterations);
    sqlite3_bind_blob(st, 3, salt, (int)sizeof(salt), SQLITE_TRANSIENT);
    sqlite3_bind_blob(st, 4, hash, (int)sizeof(hash), SQLITE_TRANSIENT);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    if (rc == SQLITE_DONE)
    {
        auth_set_current_user(username);
        return 1;
    }
    if (rc == SQLITE_CONSTRAINT)
        fprintf(stderr, "auth_register_user: username '%s' already exists (constraint).\n", username), set_auth_error("username exists");
    else
    {
        char buf[200];
        snprintf(buf, sizeof(buf), "step rc=%d: %s", rc, sqlite3_errmsg(db));
        set_auth_error(buf);
        fprintf(stderr, "auth_register_user step error: rc=%d, msg=%s\n", rc, sqlite3_errmsg(db));
    }
    return 0;
}

int auth_verify_login(const char *username, const char *password)
{
    if (!username || !password)
        return 0;
    const char *sql = "SELECT iterations, salt, hash FROM users WHERE username = ?;";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK)
        return 0;
    sqlite3_bind_text(st, 1, username, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(st);
    int ok = 0;
    if (rc == SQLITE_ROW)
    {
        int iterations = sqlite3_column_int(st, 0);
        const void *salt = sqlite3_column_blob(st, 1);
        int salt_len = sqlite3_column_bytes(st, 1);
        const void *hash = sqlite3_column_blob(st, 2);
        int hash_len = sqlite3_column_bytes(st, 2);
        if (salt && hash && salt_len == SALT_LEN && hash_len == HASH_LEN)
        {
            unsigned char calc[HASH_LEN];
            if (pbkdf2_sha256((const unsigned char *)password, strlen(password), (const unsigned char *)salt, (size_t)salt_len, iterations, calc, sizeof(calc)))
            {
                ok = (memcmp(calc, hash, HASH_LEN) == 0);
                if (ok)
                    auth_set_current_user(username);
            }
        }
    }
    sqlite3_finalize(st);
    return ok;
}

int auth_username_exists(const char *username)
{
    if (!username)
        return 0;
    const char *sql = "SELECT 1 FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK)
        return 0;
    sqlite3_bind_text(st, 1, username, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_ROW ? 1 : 0;
}
