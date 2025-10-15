#ifndef AUTH_H
#define AUTH_H

#include <sqlite3.h>

// Initialize users table if not exists. Uses global sqlite3* db from db.h
int auth_init_db();

// Register a new user: returns 1 on success, 0 on failure (e.g., username exists)
int auth_register_user(const char *username, const char *password, int iterations);

// Verify login: returns 1 if username/password correct, 0 otherwise
int auth_verify_login(const char *username, const char *password);

// Check if a username already exists
int auth_username_exists(const char *username);

// Track current authenticated user
void auth_set_current_user(const char *username); // pass NULL/"" to clear
const char *auth_get_current_user(void);          // returns "" if none

// Return last error message from auth operations (static buffer). May be empty.
const char *auth_get_last_error(void);

#endif
