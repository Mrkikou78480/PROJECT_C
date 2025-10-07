<#
Installs native_host.exe and native-messaging-host.json into %LOCALAPPDATA% and writes HKCU registry entries for Chrome/Brave.
Run in PowerShell (no admin required for HKCU). Adjust $SourceExe/$SourceJson if building elsewhere.
#>

$ErrorActionPreference = 'Stop'

Write-Host "Installing native messaging host for PROJECT_C..."

$local = [Environment]::GetFolderPath('LocalApplicationData')
$destDir = Join-Path $local 'PROJECT_C'
If (-Not (Test-Path $destDir)) { New-Item -ItemType Directory -Path $destDir | Out-Null }

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceExe = Join-Path $scriptDir '..\..\core\native_host.exe'
$sourceJson = Join-Path $scriptDir '..\..\core\native-messaging-host.json'

If (-Not (Test-Path $sourceExe)) { Write-Error "Source exe not found: $sourceExe"; exit 1 }
If (-Not (Test-Path $sourceJson)) { Write-Error "Source json not found: $sourceJson"; exit 1 }

Copy-Item -Path $sourceExe -Destination $destDir -Force
Copy-Item -Path $sourceJson -Destination $destDir -Force

# If sqlite DLL is bundled next to exe, copy it too (optional)
$dll = Join-Path $scriptDir '..\..\Mon_appDist\libsqlite3-0.dll'
If (Test-Path $dll) {
  Copy-Item -Path $dll -Destination $destDir -Force
  Write-Host "Copied sqlite runtime DLL"
}

$manifestPath = Join-Path $destDir 'native-messaging-host.json'

Function Write-HostKey($browserKey) {
  Write-Host "Writing registry key: $browserKey"
  New-Item -Path $browserKey -Force | Out-Null
  Set-ItemProperty -Path $browserKey -Name '(Default)' -Value $manifestPath
}

# Chrome (stable) and Brave (change path if needed)
$chromeKey = 'HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.project_c.passwords'
$braveKey = 'HKCU:\Software\BraveSoftware\Brave-Browser\NativeMessagingHosts\com.project_c.passwords'

Write-HostKey $chromeKey
Write-HostKey $braveKey

Write-Host "Installation complete. Manifest and exe placed in: $destDir"
Write-Host "Reload or relaunch browser, then load the extension (unpacked) from browser_extension/"
