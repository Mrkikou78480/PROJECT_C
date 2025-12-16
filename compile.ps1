# build.ps1 — Script de compilation direct (sans fichiers .o)
# Équivalent de build_mingw.sh pour PowerShell

$OUT = if ($args.Count -gt 0) { $args[0] } else { "mon_app.exe" }

# Liste des fichiers sources (dans l'ordre)
$SOURCES = @(
    "main.c",
    "gtk/main_gtk.c",
    "gtk/ui.c",
    "gtk/settings.c",
    "gtk/generator.c",
    "gtk/manager.c",
    "core/password.c",
    "core/db.c",
    "core/config.c",
    "crypto/sha256.c",
    "gtk/auth_ui.c",
    "core/auth.c",
    "crypto/simplecrypt.c"
)

Write-Host "Using pkg-config to get GTK4 flags..." -ForegroundColor Cyan
$PKG_CFLAGS = & pkg-config --cflags gtk4
$PKG_LIBS = & pkg-config --libs gtk4

if ($LASTEXITCODE -ne 0) {
    Write-Host "Erreur: pkg-config ne trouve pas gtk4" -ForegroundColor Red
    exit 1
}

Write-Host "Compiling to $OUT..." -ForegroundColor Yellow

# Séparer les flags en tableau pour éviter les problèmes avec PowerShell
$cflagsArray = $PKG_CFLAGS -split '\s+' | Where-Object { $_ -ne '' }
$libsArray = $PKG_LIBS -split '\s+' | Where-Object { $_ -ne '' }

# Compiler directement en un seul exécutable (pas de fichiers .o)
& gcc -mwindows -Wall -Wextra -g -Icrypto @cflagsArray $SOURCES -o $OUT @libsArray -lsqlite3 -ladvapi32

if ($LASTEXITCODE -eq 0) {
    Write-Host "Done: $OUT" -ForegroundColor Green
} else {
    Write-Host "Erreur de compilation" -ForegroundColor Red
    exit 1
}
