// native_host.c
// Minimal Native Messaging host in C for Windows.
// Reads a length-prefixed JSON message from stdin, responds with a JSON message on stdout.
// This host supports a simple command: {"cmd":"find_duplicates"}

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sqlite3.h>

// Read 4-byte little-endian length then that many bytes from stdin
static int read_message(char **out_buf, uint32_t *out_len)
{
    uint8_t len_bytes[4];
    if (fread(len_bytes, 1, 4, stdin) != 4)
        return 0;
    uint32_t len = (uint32_t)len_bytes[0] | ((uint32_t)len_bytes[1] << 8) | ((uint32_t)len_bytes[2] << 16) | ((uint32_t)len_bytes[3] << 24);
    char *buf = (char *)malloc(len + 1);
    if (!buf)
        return 0;
    if (fread(buf, 1, len, stdin) != len)
    {
        free(buf);
        return 0;
    }
    buf[len] = '\0';
    *out_buf = buf;
    *out_len = len;
    return 1;
}

// Write a message (length-prefixed) to stdout
static int write_message(const char *msg)
{
    uint32_t len = (uint32_t)strlen(msg);
    uint8_t len_bytes[4];
    len_bytes[0] = (uint8_t)(len & 0xff);
    len_bytes[1] = (uint8_t)((len >> 8) & 0xff);
    len_bytes[2] = (uint8_t)((len >> 16) & 0xff);
    len_bytes[3] = (uint8_t)((len >> 24) & 0xff);
    if (fwrite(len_bytes, 1, 4, stdout) != 4)
        return 0;
    if (fwrite(msg, 1, len, stdout) != len)
        return 0;
    fflush(stdout);
    return 1;
}

// Query the SQLite DB for duplicate site+login pairs and return JSON array
static char *find_duplicates_json(const char *db_path)
{
    sqlite3 *db = NULL;
    char *result = NULL;
    if (sqlite3_open(db_path, &db) != SQLITE_OK)
        return NULL;
    const char *sql = "SELECT site, login, COUNT(*) as cnt FROM passwords GROUP BY site, login HAVING cnt>1;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        sqlite3_close(db);
        return NULL;
    }

    // Build JSON string
    size_t cap = 1024;
    result = (char *)malloc(cap);
    if (!result)
    {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return NULL;
    }
    strcpy(result, "[");
    size_t len = 1;
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *site = sqlite3_column_text(stmt, 0);
        const unsigned char *login = sqlite3_column_text(stmt, 1);
        int cnt = sqlite3_column_int(stmt, 2);
        const char *site_s = site ? (const char *)site : "";
        const char *login_s = login ? (const char *)login : "";
        // estimate needed
        char item[1024];
        int written = snprintf(item, sizeof(item), "%s{\"site\":\"%s\",\"login\":\"%s\",\"count\":%d}", first ? "" : " ,", site_s, login_s, cnt);
        if (written < 0)
            break;
        if (len + written + 2 > cap)
        {
            cap = (cap + written + 2) * 2;
            char *n = (char *)realloc(result, cap);
            if (!n)
                break;
            result = n;
        }
        strcat(result + len, item + (first ? 0 : 1)); // careful append (we included comma as prefix)
        // Fix: simpler append without complicated offset: use strcat normally
        len = strlen(result);
        first = 0;
    }
    // close array
    if (len + 3 > cap)
    {
        char *n = (char *)realloc(result, len + 3);
        if (n)
            result = n;
    }
    strcat(result, "]");

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

// Escape minimal JSON special characters in a C string (" and \\)
static char *escape_json(const char *s)
{
    if (!s)
        return NULL;
    size_t len = strlen(s);
    size_t cap = len * 2 + 1;
    char *out = (char *)malloc(cap);
    if (!out)
        return NULL;
    size_t oi = 0;
    for (size_t i = 0; i < len; ++i)
    {
        char c = s[i];
        if (c == '\\' || c == '"')
        {
            if (oi + 2 + 1 > cap)
            {
                cap = (cap + 16) * 2;
                char *n = (char *)realloc(out, cap);
                if (!n)
                {
                    free(out);
                    return NULL;
                }
                out = n;
            }
            out[oi++] = '\\';
            out[oi++] = c;
        }
        else if ((unsigned char)c < 0x20)
        {
            char buf[7];
            int written = snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
            if (oi + written + 1 > cap)
            {
                cap = (cap + written + 16) * 2;
                char *n = (char *)realloc(out, cap);
                if (!n)
                {
                    free(out);
                    return NULL;
                }
                out = n;
            }
            for (int k = 0; k < written; ++k)
                out[oi++] = buf[k];
        }
        else
        {
            if (oi + 1 + 1 > cap)
            {
                cap = (cap + 16) * 2;
                char *n = (char *)realloc(out, cap);
                if (!n)
                {
                    free(out);
                    return NULL;
                }
                out = n;
            }
            out[oi++] = c;
        }
    }
    out[oi] = '\0';
    return out;
}

// Query DB for a password by site+login and return a JSON string (caller must free)
static char *get_password_json(const char *db_path, const char *site, const char *login)
{
    sqlite3 *db = NULL;
    char *result = NULL;
    if (sqlite3_open(db_path, &db) != SQLITE_OK)
        return NULL;
    const char *sql = "SELECT password FROM passwords WHERE site = ? AND login = ? LIMIT 1;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        sqlite3_close(db);
        return NULL;
    }
    sqlite3_bind_text(stmt, 1, site ? site : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, login ? login : "", -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const unsigned char *pw = sqlite3_column_text(stmt, 0);
        const char *pw_s = pw ? (const char *)pw : "";
        char *esc = escape_json(pw_s);
        if (!esc)
        {
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return NULL;
        }
        size_t need = strlen(esc) + 64;
        result = (char *)malloc(need);
        if (result)
            snprintf(result, need, "{\"ok\":true,\"password\":\"%s\"}", esc);
        free(esc);
    }
    else
    {
        result = strdup("{\"ok\":false,\"error\":\"not_found\"}");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

int main(int argc, char **argv)
{
    // Determine DB path relative to executable or a default path
    const char *db_path = "c:/Users/pauls/Desktop/ESGI/DEUXIEME ANNEE/C/PROJET/PROJECT_C/data/passwords.db";
    char *msg = NULL;
    uint32_t mlen = 0;
    if (!read_message(&msg, &mlen))
        return 1;

    // Very tiny JSON parsing: support find_duplicates and get_password
    if (strstr(msg, "\"cmd\":\"find_duplicates\""))
    {
        char *json = find_duplicates_json(db_path);
        if (!json)
        {
            write_message("{\"ok\":false,\"error\":\"db_error\"}");
        }
        else
        {
            size_t need = strlen(json) + 32;
            char *out = (char *)malloc(need);
            if (out)
            {
                snprintf(out, need, "{\"ok\":true,\"data\":%s}", json);
                write_message(out);
                free(out);
            }
            else
            {
                write_message("{\"ok\":false,\"error\":\"mem\"}");
            }
            free(json);
        }
    }
    else if (strstr(msg, "\"cmd\":\"get_password\""))
    {
        const char *site_key = "\"site\":\"";
        const char *login_key = "\"login\":\"";
        const char *p_site = strstr(msg, site_key);
        const char *p_login = strstr(msg, login_key);
        char site_buf[512] = {0};
        char login_buf[512] = {0};
        if (p_site)
        {
            p_site += strlen(site_key);
            const char *end = strchr(p_site, '"');
            if (end && (size_t)(end - p_site) < sizeof(site_buf))
                memcpy(site_buf, p_site, end - p_site);
        }
        if (p_login)
        {
            p_login += strlen(login_key);
            const char *end = strchr(p_login, '"');
            if (end && (size_t)(end - p_login) < sizeof(login_buf))
                memcpy(login_buf, p_login, end - p_login);
        }

        char *json = get_password_json(db_path, site_buf, login_buf);
        if (!json)
        {
            write_message("{\"ok\":false,\"error\":\"db_error\"}");
        }
        else
        {
            write_message(json);
            free(json);
        }
    }
    else
    {
        write_message("{\"ok\":false,\"error\":\"unknown_cmd\"}");
    }
    free(msg);
    return 0;
}
