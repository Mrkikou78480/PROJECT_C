#include "auth.h"
#include "db.h"
#include "sha256.h"
#include "../crypto/simplecrypt.h"
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <string.h>

#define SALT_LEN 16
#define HASH_LEN 32 // SHA-256

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
static char g_auth_avatar_path[260] = "";

static void load_avatar_for_user(const char *username)
{
    g_auth_avatar_path[0] = '\0';
    if (!username || !*username)
        return;

    const char *sql = "SELECT avatar_path FROM users WHERE username = ?;";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK)
        return;
    sqlite3_bind_text(st, 1, username, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st) == SQLITE_ROW)
    {
        const unsigned char *p = sqlite3_column_text(st, 0);
        if (p)
            snprintf(g_auth_avatar_path, sizeof(g_auth_avatar_path), "%s", (const char *)p);
    }
    sqlite3_finalize(st);
}

void auth_set_current_user(const char *username)
{
    if (!username)
        username = "";
    snprintf(g_auth_current_user, sizeof(g_auth_current_user), "%s", username);
    load_avatar_for_user(username);
}
const char *auth_get_current_user(void)
{
    return g_auth_current_user;
}

const char *auth_get_current_avatar_path(void)
{
    return g_auth_avatar_path;
}

// Retourne la clé de chiffrement (SHA-256 du mot de passe maître) pour l'utilisateur courant
// Le buffer doit faire au moins 33 octets (32 + 1 pour \0)
int auth_get_encryption_key(char out[33])
{
    // On va chercher le hash du mot de passe stocké pour l'utilisateur courant
    const char *user = auth_get_current_user();
    if (!user || !*user)
        return 0;
    const char *sql = "SELECT hash FROM users WHERE username = ?;";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK)
        return 0;
    sqlite3_bind_text(st, 1, user, -1, SQLITE_TRANSIENT);
    int ok = 0;
    if (sqlite3_step(st) == SQLITE_ROW)
    {
        const void *hash = sqlite3_column_blob(st, 0);
        int hash_len = sqlite3_column_bytes(st, 0);
        if (hash && hash_len == 32)
        {
            memcpy(out, hash, 32);
            out[32] = '\0';
            ok = 1;
        }
    }
    sqlite3_finalize(st);
    return ok;
}

static void hmac_sha256(const unsigned char *key, size_t key_len,
                        const unsigned char *data, size_t data_len,
                        unsigned char out[HASH_LEN])
{
    unsigned char k_ipad[64];
    unsigned char k_opad[64];
    unsigned char tk[HASH_LEN];
    size_t i;

    if (key_len > 64)
    {
        sha256(key, key_len, tk);
        key = tk;
        key_len = HASH_LEN;
    }

    for (i = 0; i < 64; ++i)
    {
        unsigned char kc = (i < key_len) ? key[i] : 0x00;
        k_ipad[i] = kc ^ 0x36;
        k_opad[i] = kc ^ 0x5c;
    }

    sha256_ctx ctx;
    unsigned char inner[HASH_LEN];

    sha256_init(&ctx);
    sha256_update(&ctx, k_ipad, 64);
    sha256_update(&ctx, data, data_len);
    sha256_final(&ctx, inner);

    sha256_init(&ctx);
    sha256_update(&ctx, k_opad, 64);
    sha256_update(&ctx, inner, HASH_LEN);
    sha256_final(&ctx, out);
}

static int pbkdf2_sha256(const unsigned char *password, size_t password_len,
                         const unsigned char *salt, size_t salt_len,
                         int iterations,
                         unsigned char *out_key, size_t out_len)
{
    if (!password || !salt || iterations <= 0 || !out_key || out_len == 0)
    {
        set_auth_error("pbkdf2_sha256 invalid args");
        return 0;
    }

    unsigned int blocks = (unsigned int)((out_len + HASH_LEN - 1) / HASH_LEN);
    unsigned int i, j;
    unsigned char U[HASH_LEN];
    unsigned char T[HASH_LEN];
    unsigned char counter[4];
    size_t pos = 0;

    for (i = 1; i <= blocks; ++i)
    {
        counter[0] = (unsigned char)((i >> 24) & 0xFF);
        counter[1] = (unsigned char)((i >> 16) & 0xFF);
        counter[2] = (unsigned char)((i >> 8) & 0xFF);
        counter[3] = (unsigned char)(i & 0xFF);

        unsigned char sc[256];
        if (salt_len + 4 > sizeof(sc))
        {
            set_auth_error("salt too long");
            return 0;
        }
        memcpy(sc, salt, salt_len);
        memcpy(sc + salt_len, counter, 4);

        hmac_sha256(password, password_len, sc, salt_len + 4, U);
        memcpy(T, U, HASH_LEN);

        for (j = 2; j <= (unsigned int)iterations; ++j)
        {
            hmac_sha256(password, password_len, U, HASH_LEN, U);
            for (unsigned int k = 0; k < HASH_LEN; ++k)
                T[k] ^= U[k];
        }

        size_t to_copy = (pos + HASH_LEN <= out_len) ? HASH_LEN : (out_len - pos);
        memcpy(out_key + pos, T, to_copy);
        pos += to_copy;
    }

    return 1;
}

static int random_bytes(unsigned char *buf, size_t len)
{
    if (!buf || len == 0)
    {
        set_auth_error("random_bytes invalid args");
        return 0;
    }

    HCRYPTPROV hProv = 0;
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        set_auth_error("CryptAcquireContext failed");
        return 0;
    }
    if (!CryptGenRandom(hProv, (DWORD)len, buf))
    {
        set_auth_error("CryptGenRandom failed");
        CryptReleaseContext(hProv, 0);
        return 0;
    }
    CryptReleaseContext(hProv, 0);
    return 1;
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
    rc = sqlite3_exec(db, "CREATE UNIQUE INDEX IF NOT EXISTS idx_users_username ON users(username);", 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "auth_init_db index error: %s\n", sqlite3_errmsg(db));
    }
    rc = sqlite3_exec(db, "ALTER TABLE users ADD COLUMN avatar_path TEXT;", 0, 0, 0);
    if (rc != SQLITE_OK)
    {
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

int auth_change_password(const char *username,
                         const char *old_password,
                         const char *new_password,
                         int iterations)
{
    if (!username || !old_password || !new_password || iterations <= 0)
    {
        set_auth_error("change_password invalid args");
        return 0;
    }

    if (!auth_verify_login(username, old_password))
    {
        set_auth_error("Ancien mot de passe incorrect");
        return 0;
    }

    unsigned char salt[SALT_LEN];
    unsigned char hash[HASH_LEN];
    if (!random_bytes(salt, sizeof(salt)))
        return 0;
    if (!pbkdf2_sha256((const unsigned char *)new_password, strlen(new_password),
                       salt, sizeof(salt), iterations, hash, sizeof(hash)))
        return 0;

    const char *sql = "UPDATE users SET iterations = ?, salt = ?, hash = ? WHERE username = ?;";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK)
    {
        set_auth_error("change_password prepare failed");
        return 0;
    }
    sqlite3_bind_int(st, 1, iterations);
    sqlite3_bind_blob(st, 2, salt, (int)sizeof(salt), SQLITE_TRANSIENT);
    sqlite3_bind_blob(st, 3, hash, (int)sizeof(hash), SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 4, username, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    if (rc == SQLITE_DONE)
        return 1;

    set_auth_error("change_password update failed");
    return 0;
}

int auth_set_avatar_path(const char *username, const char *path)
{
    if (!username || !*username)
    {
        set_auth_error("avatar: invalid username");
        return 0;
    }
    const char *sql = "UPDATE users SET avatar_path = ? WHERE username = ?;";
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) != SQLITE_OK)
    {
        set_auth_error("avatar update prepare failed");
        return 0;
    }
    if (path && *path)
        sqlite3_bind_text(st, 1, path, -1, SQLITE_TRANSIENT);
    else
        sqlite3_bind_null(st, 1);
    sqlite3_bind_text(st, 2, username, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    if (rc == SQLITE_DONE)
    {
        snprintf(g_auth_avatar_path, sizeof(g_auth_avatar_path), "%s", path ? path : "");
        return 1;
    }
    set_auth_error("avatar update failed");
    return 0;
}
