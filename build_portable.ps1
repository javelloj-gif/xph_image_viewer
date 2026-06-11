param(
    [string]$OutputName = "lenteuv_portable"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$binaryPath = Join-Path $ProjectRoot "native\build\bin\Release\lenteuv.exe"

# Dynamically detect VC++ Redistributable directory
$pathsToSearch = @(
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC"
)
$vcRedistDir = ""
foreach ($p in $pathsToSearch) {
    if (Test-Path $p) {
        $subDirs = Get-ChildItem -Path $p -Directory | Sort-Object Name -Descending
        foreach ($d in $subDirs) {
            $candidate = Join-Path $d.FullName "x64\Microsoft.VC143.CRT"
            if (Test-Path $candidate) {
                $vcRedistDir = $candidate
                break
            }
        }
    }
    if ($vcRedistDir) { break }
}
if (-not $vcRedistDir) {
    # Fallback to default build tools path
    $vcRedistDir = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\14.42.34433\x64\Microsoft.VC143.CRT"
}

function PrintHeader { param([string]$s) Write-Host "`n== $s ==" -ForegroundColor Cyan }

# Step 1: Build
PrintHeader "Compilando lenteuv.exe (Release)"
& $cmake --build (Join-Path $ProjectRoot "native\build") --config Release 2>&1 | Out-Host
if (-not (Test-Path $binaryPath)) { Write-Host "[ERROR] Build fallo" -ForegroundColor Red; exit 1 }
Write-Host "[OK] $( (Get-Item $binaryPath).Length / 1KB ) KB" -ForegroundColor Green

# Step 2: Remove stale logs from build output
Remove-Item -LiteralPath (Join-Path (Split-Path $binaryPath) "kv_log.txt") -ErrorAction SilentlyContinue
Remove-Item -LiteralPath (Join-Path (Split-Path $binaryPath) "debug_ipc.txt") -ErrorAction SilentlyContinue

# Step 3: Create temp dir
$tempDir = Join-Path $ProjectRoot $OutputName
if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }
New-Item -ItemType Directory -Path $tempDir | Out-Null
$zipPath = Join-Path $ProjectRoot "$OutputName.zip"
Remove-Item -Force $zipPath -ErrorAction SilentlyContinue

# Step 4: Copy executable
PrintHeader "Copiando ejecutable"
Copy-Item $binaryPath -Destination (Join-Path $tempDir "lenteuv.exe")

# Step 5: Copy VC++ CRT DLLs
PrintHeader "Copiando VC++ CRT DLLs"
$vcDlls = @("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll")
foreach ($dll in $vcDlls) {
    $src = Join-Path $vcRedistDir $dll
    if (Test-Path $src) { Copy-Item $src -Destination (Join-Path $tempDir $dll) }
    else { Write-Host "  [WARN] $dll no encontrado" -ForegroundColor Yellow }
}

# Step 6: Create user_data skeleton
PrintHeader "Creando user_data"
$userDataDir = Join-Path $tempDir "user_data"
New-Item -ItemType Directory -Path (Join-Path $userDataDir "boards") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $userDataDir "cache") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $userDataDir "tags") | Out-Null
@"
{
 "settings": {
  "---general": "General",
  "ui_lang": ["en","UI language"],
  "page_on_app_launch_from_explorer": [2,[[2,"VIEWER"],[3,"BOARD"],[4,"LIBRARY"],[5,"WATERFALL"]],"Page on App Launch (when opened from Explorer)"],
  "page_to_open_on_drag_and_drop": [2,[[2,"VIEWER"],[3,"BOARD"],[4,"LIBRARY"],[5,"WATERFALL"]],"Page to open on Drag&Drop"],
  "debug_page": [1,[[1,"START"],[2,"VIEWER"],[3,"BOARD"],[4,"LIBRARY"],[5,"WATERFALL"]],"Page to open when run debug mode"],
  "run_on_windows_startup": [false,"Run on Windows Startup"],
  "do_not_show_start_dialog": [true,"Supress start dialog and run lite mode"],
  "show_fullscreen": [true,"Full-screen mode on application start"],
  "doubleclick_toggle": [true,"Toggle between full-screen and window mode via double click"],
  "hide_to_tray_on_close": [false,"Hide to tray on close"],
  "hide_on_app_start": [false,"Hide to tray on app start"],
  "single_instance": [true,"Single instance enforcement"],
  "desaturated_corner_buttons_and_corner_menus": [false,"Desaturated corner buttons and corner menus"],
  "show_console_output": [false,"Show standard (console) output overlay"],
  "effects": [true,"Animated effects"],
  "show_noise_cells": [false,"Show animated cells overlay"],
  "---diagnostics": "Diagnostics",
  "enable_file_log": [false,"Write log messages to kv_log.txt"],
  "debug_ipc": [false,"Write debug IPC logs to debug_ipc.txt"],
  "settings_window_opacity": [0.5,[0.1,1.0],"Settings window opacity"],
  "window_opacity": [0.85,[0.0,1.0],"Application window opacity"],
  "---viewerpage": "Viewer page",
  "animated_zoom": [false,"Animated zoom"],
  "draw_control_panel_backplate": [false,"Draw backplate for control panel"],
  "thumbnail_width": [50.0,[30.0,100.0],"Thumbnails size"],
  "zoom_on_mousewheel": [true,"Enable mouse wheel to zoom and Ctrl+mouse wheel to navigate through image list"],
  "draw_default_thumbnail": [true,"Show dummy-default thumbnail while generated one is not ready"],
  "show_thirds": [false,"Show thirds"],
  "show_cyberpunk": [false,"Cyberpunk frame"],
  "show_metadata": [true,"Show metadata"],
  "sort_mode": [0,"Sort mode"]
 }
}
"@ | Set-Content -Path (Join-Path $userDataDir "settings.json") -Encoding UTF8
"" | Set-Content -Path (Join-Path $userDataDir "session.txt") -Encoding UTF8

# Step 7: Copy locales, resources, cursors
PrintHeader "Copiando assets"
$dirs = @("locales", "resources", "cursors")
foreach ($d in $dirs) {
    $src = Join-Path $ProjectRoot $d
    if (Test-Path $src) {
        Copy-Item $src -Destination (Join-Path $tempDir $d) -Recurse
    }
}

# Step 8: Create README
@"
LenteUV Image Viewer v0.90 Alpha - Portable
=============================================
Autor: xPhang | GPL v2+ | github.com/sergkrumas/image_viewer

Ejecutar lenteuv.exe directamente (no requiere instalacion).
"@ | Out-File -FilePath (Join-Path $tempDir "README.txt") -Encoding UTF8

# Step 9: ZIP
PrintHeader "Comprimiendo en $OutputName.zip"
Start-Sleep -Seconds 2
Compress-Archive -Path "$tempDir\*" -DestinationPath $zipPath -CompressionLevel Optimal
$zipSize = (Get-Item $zipPath).Length
Write-Host "[OK] $OutputName.zip ($([math]::Round($zipSize/1MB,2)) MB)" -ForegroundColor Green

# Cleanup
Remove-Item -Recurse -Force $tempDir
Write-Host "`nPortable generado: $zipPath" -ForegroundColor Green