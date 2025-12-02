#!/usr/bin/env bash
set -euo pipefail

# build_mingw.sh — simple build script for MinGW/MSYS2
# Usage:
#   ./build_mingw.sh            # builds mon_app.exe
#   ./build_mingw.sh myapp.exe  # build with custom output name

OUT=${1:-mon_app.exe}
CC=${CC:-gcc}

# Source files in the order you provided (keeps linking order predictable)

SOURCES=(
  main.c
  gtk/main_gtk.c
  gtk/ui.c
  gtk/settings.c
  gtk/generator.c
  gtk/manager.c
  core/password.c
  core/db.c
  crypto/sha256.c
  gtk/auth_ui.c
  core/auth.c
  crypto/simplecrypt.c
)

# Compile flags: GTK cflags will be appended; -mwindows for GUI subsystem on MinGW

CFLAGS=( -mwindows -Wall -Wextra -g -Icrypto )

echo "Using pkg-config to get GTK4 flags..."
PKG_CFLAGS=$(pkg-config --cflags gtk4)
PKG_LIBS=$(pkg-config --libs gtk4)

echo "Compiling to ${OUT}..."

# Run the compiler
${CC} "${CFLAGS[@]}" ${PKG_CFLAGS} "${SOURCES[@]}" -o "${OUT}" ${PKG_LIBS} -lsqlite3 -ladvapi32

echo "Done: ${OUT}"
