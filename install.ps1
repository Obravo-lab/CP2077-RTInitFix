# =============================================================================
# Cyberpunk 2077 - Fix "Problème d'initialisation du Ray Tracing"
# Pour systèmes AMD + NVIDIA dual-GPU (v2.31+)
# =============================================================================
# Ce fix intercepte les appels DXGI/D3D12/Streamline pour masquer le GPU NVIDIA
# au jeu, évitant ainsi le dialog d'erreur RT au démarrage.
# =============================================================================

param(
    [string]$GamePath = "",
    [switch]$Silent
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$version   = "v1.0"

function Write-Title {
    Write-Host ""
    Write-Host "=========================================================" -ForegroundColor Cyan
    Write-Host "  Cyberpunk 2077 - RT Init Fix $version" -ForegroundColor Cyan
    Write-Host "  Fix pour GPU AMD + NVIDIA (dual-GPU)" -ForegroundColor Cyan
    Write-Host "=========================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Step { param($msg) Write-Host "  >> $msg" -ForegroundColor White }
function Write-OK   { param($msg) Write-Host "  [OK] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "  [!]  $msg" -ForegroundColor Yellow }
function Write-Err  { param($msg) Write-Host "  [ERR] $msg" -ForegroundColor Red }

# ──────────────────────────────────────────────────────────────────────────────
# 1. Recherche du dossier d'installation Cyberpunk 2077
# ──────────────────────────────────────────────────────────────────────────────
function Find-GamePath {
    # GOG - registre 64 et 32 bits
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

    # Steam - AppID 1091500
    $steamPaths = @(
        (Get-ItemProperty "HKCU:\SOFTWARE\Valve\Steam" -ErrorAction SilentlyContinue)?.SteamPath,
        "C:\Program Files (x86)\Steam",
        "C:\Program Files\Steam"
    ) | Where-Object { $_ }
    foreach ($s in $steamPaths) {
        $p = "$s\steamapps\common\Cyberpunk 2077"
        if (Test-Path "$p\bin\x64\Cyberpunk2077.exe") { return $p }
    }

    # Epic Games
    $epicManifests = "$env:PROGRAMDATA\Epic\EpicGamesLauncher\Data\Manifests"
    if (Test-Path $epicManifests) {
        Get-ChildItem "$epicManifests\*.item" -ErrorAction SilentlyContinue | ForEach-Object {
            try {
                $manifest = Get-Content $_.FullName -Raw | ConvertFrom-Json
                if ($manifest.AppName -like "*Cyberpunk*" -or $manifest.DisplayName -like "*Cyberpunk*") {
                    $p = $manifest.InstallLocation
                    if ($p -and (Test-Path "$p\bin\x64\Cyberpunk2077.exe")) { return $p }
                }
            } catch {}
        }
    }

    # Chemins courants
    $commonPaths = @(
        "E:\GOG\Cyberpunk 2077", "D:\GOG\Cyberpunk 2077", "C:\GOG\Cyberpunk 2077",
        "E:\Games\Cyberpunk 2077", "D:\Games\Cyberpunk 2077", "C:\Games\Cyberpunk 2077",
        "E:\SteamLibrary\steamapps\common\Cyberpunk 2077",
        "D:\SteamLibrary\steamapps\common\Cyberpunk 2077",
        "C:\Program Files\GOG Galaxy\Games\Cyberpunk 2077",
        "D:\Program Files\GOG Galaxy\Games\Cyberpunk 2077"
    )
    foreach ($p in $commonPaths) {
        if (Test-Path "$p\bin\x64\Cyberpunk2077.exe") { return $p }
    }

    return $null
}

# ──────────────────────────────────────────────────────────────────────────────
# MAIN
# ──────────────────────────────────────────────────────────────────────────────
Write-Title

# Trouver le jeu
if ($GamePath -eq "") {
    Write-Step "Recherche de Cyberpunk 2077..."
    $GamePath = Find-GamePath
}

if (-not $GamePath -or -not (Test-Path "$GamePath\bin\x64\Cyberpunk2077.exe")) {
    Write-Warn "Cyberpunk 2077 introuvable automatiquement."
    Write-Host ""
    $GamePath = Read-Host "  Entrez le chemin complet d'installation`n  (ex: E:\GOG\Cyberpunk 2077)"
    if (-not (Test-Path "$GamePath\bin\x64\Cyberpunk2077.exe")) {
        Write-Err "Cyberpunk2077.exe introuvable dans $GamePath\bin\x64\"
        if (-not $Silent) { Read-Host "`nAppuyez sur Entrée pour quitter" }
        exit 1
    }
}

Write-OK "Jeu trouvé: $GamePath"
$binx64 = "$GamePath\bin\x64"

# Vérifier les fichiers source
$filesToInstall = @("sl.interposer.dll", "nvapi64.dll", "GFSDK_Aftermath_Lib.x64.dll")
Write-Step "Vérification des fichiers du fix..."
foreach ($f in $filesToInstall) {
    if (-not (Test-Path "$scriptDir\files\$f")) {
        Write-Err "Fichier manquant: files\$f"
        Write-Err "Assurez-vous que tous les fichiers du package sont présents."
        if (-not $Silent) { Read-Host "`nAppuyez sur Entrée pour quitter" }
        exit 1
    }
}
Write-OK "Tous les fichiers du fix sont présents."

# Créer dossier de backup
$backupDir = "$binx64\backup_rtfix"
if (-not (Test-Path $backupDir)) {
    New-Item -ItemType Directory -Path $backupDir | Out-Null
}

# Installer les DLLs
Write-Host ""
Write-Step "Installation des DLLs..."
foreach ($f in $filesToInstall) {
    $src = "$scriptDir\files\$f"
    $dst = "$binx64\$f"
    $bak = "$backupDir\$f"

    if ((Test-Path $dst) -and -not (Test-Path $bak)) {
        Copy-Item $dst $bak -Force
        Write-OK "Backup: $f -> backup_rtfix\$f"
    }
    Copy-Item $src $dst -Force
    Write-OK "Installé: $f"
}

# Patch UserSettings.json
Write-Host ""
Write-Step "Patch UserSettings.json..."
$settingsPath = "$env:LOCALAPPDATA\CD Projekt Red\Cyberpunk 2077\UserSettings.json"

if (Test-Path $settingsPath) {
    $bakSettings = "$backupDir\UserSettings.json"
    if (-not (Test-Path $bakSettings)) {
        Copy-Item $settingsPath $bakSettings -Force
        Write-OK "Backup: UserSettings.json"
    }

    $content = Get-Content $settingsPath -Raw
    # Patch DLSS_UNSUPPORTED false -> true (désactive les tentatives init DLSS/RT)
    $patched = [regex]::Replace(
        $content,
        '("name":\s*"DLSS_UNSUPPORTED"[^}]*?"value":\s*)false',
        '${1}true'
    )
    if ($patched -ne $content) {
        Set-Content $settingsPath $patched -Encoding UTF8 -NoNewline
        Write-OK "DLSS_UNSUPPORTED = true"
    } else {
        Write-Warn "DLSS_UNSUPPORTED déjà à true ou introuvable (ignoré)"
    }
} else {
    Write-Warn "UserSettings.json introuvable (le jeu n'a peut-être jamais été lancé)"
    Write-Warn "Le patch JSON sera appliqué au prochain lancement."
}

# Supprimer le log de debug (si présent d'une session précédente)
$logFile = "$binx64\sl_proxy_debug.txt"
if (Test-Path $logFile) { Remove-Item $logFile -Force }

Write-Host ""
Write-Host "=========================================================" -ForegroundColor Green
Write-Host "  Installation terminée avec succès !" -ForegroundColor Green
Write-Host ""
Write-Host "  Pour désinstaller: lancez uninstall.bat" -ForegroundColor Gray
Write-Host "  Backup dans: $backupDir" -ForegroundColor Gray
Write-Host "=========================================================" -ForegroundColor Green
Write-Host ""

if (-not $Silent) { Read-Host "Appuyez sur Entrée pour quitter" }
