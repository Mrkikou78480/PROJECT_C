PROJECT_C - Native Messaging host & extension

Quick install (Windows, HKCU):

1. Build native host with your Makefile (creates core/native_host.exe). Ensure libsqlite3-0.dll is available alongside the exe or on PATH.

2. Run the helper installer in PowerShell (from project root):

   .\scripts\install_native_host.ps1

   This copies the built native_host.exe and native-messaging-host.json into %LOCALAPPDATA%\PROJECT_C and writes HKCU registry entries for Chrome and Brave.

3. Load the extension in Brave/Chrome (Developer -> Load unpacked) and point to the browser_extension/ directory.

4. Click the extension icon to open the popup. Enter Site and Login and click "Get password".

Debug & test tips:

- If the native host returns errors about sqlite DLL missing, copy the appropriate libsqlite3-0.dll into the same folder as native_host.exe or add its directory to PATH.
- Open the extension service worker console: chrome://extensions -> Service worker (under the extension) to see logs from background.js.
- For low-level testing, you can send a length-prefixed JSON message to the exe from PowerShell (see scripts/test_native.ps1 if provided).
  PROJECT_C - Native Messaging host (C) and browser extension

Overview

- This repo contains a minimal native messaging host implemented in C (`core/native_host.c`) which queries the project's SQLite DB and returns JSON.
- A WebExtension skeleton is provided in `browser_extension/`.

Build native host (Windows, MSVC)

1. Open a Developer Command Prompt for Visual Studio.
2. Compile:

   cl /I"C:\Path\To\SQLite\Include" core\native_host.c sqlite3.lib /link /OUT:core\native_host.exe

If using MinGW / gcc:

gcc -I/path/to/sqlite/include -L/path/to/sqlite/lib -o core/native_host.exe core/native_host.c -lsqlite3

Install native host manifest (Windows)

1. Copy `core/native-messaging-host.json` to a stable location (e.g. `C:\Program Files\PROJECT_C\native-messaging-host.json`) and update the `path` field to the absolute path of `native_host.exe`.
2. Add a registry key for Chrome (example PowerShell):

   New-Item -Path HKCU:\Software\Google\Chrome\NativeMessagingHosts -Name com.project_c.passwords -Force | Out-Null
   Set-Content -Path "HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.project_c.passwords" -Value (Get-Content -Raw "C:\Program Files\PROJECT_C\native-messaging-host.json")

   (For Firefox use HKCU:\Software\Mozilla\NativeMessagingHosts)

Load the extension

1. In Chrome/Edge, go to chrome://extensions, enable Developer mode, and "Load unpacked" pointing to `browser_extension/`.
2. In Firefox, use about:debugging -> This Firefox -> Load Temporary Add-on and pick `browser_extension/manifest.json`.

Testing without extension
You can test the native host by sending a length-prefixed message on stdin. A small helper script or utility is recommended; one-shot tests can be done using Python but user requested pure C—so consider writing a tiny C test harness that writes the 4-byte length and JSON to the host's stdin and reads the response.

Security

- Native messaging exposes local data to any installed extension listed in `allowed_origins`. Only install trusted extensions.
