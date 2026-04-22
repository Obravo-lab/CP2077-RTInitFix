# Cyberpunk 2077 - RT Init Fix - DÉSINSTALLATEUR

param([string]$GamePath = "", [switch]$Silent)
$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

function Write-OK   { param($msg) Write-Host "  [OK] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "  [!]  $msg" -ForegroundColor Yellow }
function Write-Err  { param($msg) Write-Host "  [ERR] $msg" -ForegroundColor Red }

Write-Host ""
Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host "  Cyberpunk 2077 - RT Init Fix - Désinstallation" -ForegroundColor Cyan
Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host ""

function Find-GamePath {
    $gogKeys = @(
        "HKLM:\SOFTWARE\GOG.com\Games\1423049311",
        "HKLM:\SOFTWARE\WOW6432Node\GOG.com\Games\1423049311"
    )
    foreach ($k in $gogKeys) {
        try {
            $p = (Get-ItemProperty $k -ErrorAction Stop).path
            if ($p -and (Test-Path "$p\bin\x64\Cyberpunk2077.exe")) { return $p }
        } catch {}
    }
    $commonPaths = @(
        "E:\GOG\Cyberpunk 2077", "D:\GOG\Cyberpunk 2077", "C:\GOG\Cyberpunk 2077",
        "E:\Games\Cyberpunk 2077", "D:\Games\Cyberpunk 2077"
    )
    foreach ($p in $commonPaths) {
        if (Test-Path "$p\bin\x64\Cyberpunk2077.exe") { return $p }
    }
    return $null
}

if ($GamePath -eq "") { $GamePath = Find-GamePath }
if (-not $GamePath -or -not (Test-Path "$GamePath\bin\x64\Cyberpunk2077.exe")) {
    $GamePath = Read-Host "  Chemin d'installation Cyberpunk 2077"
    if (-not (Test-Path "$GamePath\bin\x64\Cyberpunk2077.exe")) {
        Write-Err "Jeu introuvable dans $GamePath"
        if (-not $Silent) { Read-Host "Entrée pour quitter" }
        exit 1
    }
}

$binx64    = "$GamePath\bin\x64"
$backupDir = "$binx64\backup_rtfix"

if (-not (Test-Path $backupDir)) {
    Write-Warn "Dossier backup_rtfix introuvable — rien à restaurer."
    if (-not $Silent) { Read-Host "Entrée pour quitter" }
    exit 0
}

Write-Host "  Jeu: $GamePath" -ForegroundColor White
Write-Host ""

# Restaurer les DLLs depuis le backup
$files = @("sl.interposer.dll", "nvapi64.dll", "GFSDK_Aftermath_Lib.x64.dll")
foreach ($f in $files) {
    $bak = "$backupDir\$f"
    $dst = "$binx64\$f"
    if (Test-Path $bak) {
        Copy-Item $bak $dst -Force
        Write-OK "Restauré: $f"
    } else {
        # Pas de backup = fichier n'existait pas avant; supprimer la version installée
        if (Test-Path $dst) { Remove-Item $dst -Force }
        Write-OK "Supprimé (pas d'original): $f"
    }
}

# Restaurer UserSettings.json
$settingsPath = "$env:LOCALAPPDATA\CD Projekt Red\Cyberpunk 2077\UserSettings.json"
$bakSettings  = "$backupDir\UserSettings.json"
if (Test-Path $bakSettings) {
    Copy-Item $bakSettings $settingsPath -Force
    Write-OK "UserSettings.json restauré"
}

# Nettoyer le log de debug
$logFile = "$binx64\sl_proxy_debug.txt"
if (Test-Path $logFile) { Remove-Item $logFile -Force; Write-OK "Log debug supprimé" }

# Supprimer le dossier backup
Remove-Item $backupDir -Recurse -Force
Write-OK "Dossier backup_rtfix supprimé"

Write-Host ""
Write-Host "=========================================================" -ForegroundColor Green
Write-Host "  Désinstallation terminée !" -ForegroundColor Green
Write-Host "=========================================================" -ForegroundColor Green
Write-Host ""
if (-not $Silent) { Read-Host "Appuyez sur Entrée pour quitter" }
