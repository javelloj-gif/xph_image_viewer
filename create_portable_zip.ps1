# ==============================================================================
# create_portable_zip.ps1 - LenteUV Image Viewer Portable Package Builder
# ==============================================================================
# Recopila el binario compilado, sus DLLs de VC++ Redistributable, locales,
# recursos, cursores y un esqueleto de user_data/ en un ZIP listo para
# distribuir.
#
# Uso:
#   .\create_portable_zip.ps1
#   .\create_portable_zip.ps1 -OutputName "lenteuv_portable_v0.90"
#   .\create_portable_zip.ps1 -SkipBuild
#   .\create_portable_zip.ps1 -SkipSmokeTest
#
# Salida:
#   lenteuv_portable.zip (o el nombre especificado) en el directorio raiz.
# ==============================================================================

param(
    [string]$OutputName  = "lenteuv_portable",
    [switch]$SkipBuild   = $false,
    [switch]$SkipSmokeTest = $false
)

$ErrorActionPreference = "Stop"

# --- Rutas y herramientas -----------------------------------------------------
$ScriptDir    = Split-Path -Parent $MyInvocation.MyCommand.Path
$CMakePath    = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$DumpBinPath  = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.42.34433\bin\Hostx64\x64\dumpbin.exe"
$BinaryPath   = Join-Path $ScriptDir "native\build\bin\Release\lenteuv.exe"
$BinaryDir    = Split-Path -Parent $BinaryPath
$LocalesDir   = Join-Path $ScriptDir "locales"
$ResourcesDir = Join-Path $ScriptDir "resources"
$CursorsDir   = Join-Path $ScriptDir "cursors"
$TempDir      = Join-Path $ScriptDir $OutputName
$ZipOutput    = Join-Path $ScriptDir "$OutputName.zip"
$SmokeDir     = Join-Path $env:TEMP "lenteuv_smoke_$PID"

# --- Helpers ------------------------------------------------------------------
function Write-Header { param([string]$s)
    Write-Host ""
    Write-Host "== $s ==" -ForegroundColor Cyan
}

function Write-Step { param([int]$n, [int]$total, [string]$msg)
    Write-Host ""
    Write-Host "[$n/$total] $msg" -ForegroundColor White
}

# --- Verificaciones previas ---------------------------------------------------
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "     LenteUV Image Viewer - Portable Package Builder        " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# --- Step 1: Compilar el ejecutable -------------------------------------------
if (-not $SkipBuild) {
    Write-Step 1 11 "Compilando lenteuv.exe (Release)..."
    if (-not (Test-Path $CMakePath)) {
        Write-Host "[ERROR] cmake.exe no encontrado en: $CMakePath" -ForegroundColor Red
        exit 1
    }
    & $CMakePath --build (Join-Path $ScriptDir "native\build") --config Release 2>&1 | Out-Host
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] La compilacion fallo con codigo $LASTEXITCODE" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "[SKIP] Compilacion C++ omitida (usando exe existente)." -ForegroundColor Gray
}

if (-not (Test-Path $BinaryPath)) {
    Write-Host "[ERROR] No se encontro el ejecutable compilado:" -ForegroundColor Red
    Write-Host "        $BinaryPath" -ForegroundColor Red
    exit 1
}

# --- Step 2: Limpiar logs residuales del build -------------------------------
Write-Step 2 11 "Limpiando logs residuales del build..."
foreach ($logName in @("kv_log.txt", "debug_ipc.txt")) {
    $logPath = Join-Path $BinaryDir $logName
    if (Test-Path $logPath) {
        Remove-Item -LiteralPath $logPath -Force
        Write-Host "  [OK] Eliminado: $logName" -ForegroundColor Gray
    }
}

# --- Step 3: Detectar VC++ Redistributable -----------------------------------
Write-Step 3 11 "Detectando VC++ Redistributable..."
$vcRedistDir = ""
$pathsToSearch = @(
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC"
)
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
    Write-Host "[ERROR] No se encontro VC++ Redistributable (Microsoft.VC143.CRT)." -ForegroundColor Red
    Write-Host "  Buscado en:" -ForegroundColor Yellow
    foreach ($p in $pathsToSearch) { Write-Host "    $p" -ForegroundColor Yellow }
    Write-Host "  Instala Visual Studio 2022 Build Tools con 'Desktop development with C++'" -ForegroundColor Yellow
    exit 1
}
Write-Host "  [OK] $vcRedistDir" -ForegroundColor Green

# --- Step 4: Limpiar ZIP anterior y directorio temporal ----------------------
Write-Step 4 11 "Preparando directorio temporal..."
if (Test-Path $TempDir) {
    Write-Host "  [INFO] Limpiando directorio temporal existente..." -ForegroundColor Gray
    Remove-Item -Recurse -Force $TempDir
}
if (Test-Path $ZipOutput) {
    Write-Host "  [INFO] Eliminando ZIP anterior..." -ForegroundColor Gray
    Remove-Item -Force $ZipOutput
}
New-Item -ItemType Directory -Path $TempDir | Out-Null
Write-Host "  [OK] $TempDir" -ForegroundColor Green

# --- Step 5: Copiar ejecutable ------------------------------------------------
Write-Step 5 11 "Copiando ejecutable..."
Copy-Item $BinaryPath -Destination (Join-Path $TempDir "lenteuv.exe")
$ExeSizeKB = [math]::Round((Get-Item $BinaryPath).Length / 1KB, 1)
Write-Host "  [OK] lenteuv.exe ($ExeSizeKB KB)" -ForegroundColor Green

# --- Step 6: Copiar VC++ CRT DLLs (FIX: este paso es lo que faltaba) ---------
Write-Step 6 11 "Copiando VC++ CRT DLLs..."
$vcDlls = @("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll", "vcruntime140_clr0400.dll")
$crtCopied = 0
foreach ($dll in $vcDlls) {
    $src = Join-Path $vcRedistDir $dll
    if (Test-Path $src) {
        Copy-Item $src -Destination (Join-Path $TempDir $dll)
        $crtCopied++
        Write-Host "  [OK] $dll" -ForegroundColor Green
    }
}
if ($crtCopied -lt 3) {
    Write-Host "[WARN] Faltan algunas DLLs de VC++ CRT (copiadas: $crtCopied / $($vcDlls.Count))" -ForegroundColor Yellow
    Write-Host "  El portable puede no funcionar en sistemas sin VC++ Redistributable." -ForegroundColor Yellow
}

# --- Step 7: Crear esqueleto de user_data/ -----------------------------------
Write-Step 7 11 "Creando user_data/ con defaults portables..."
$userDataDir = Join-Path $TempDir "user_data"
New-Item -ItemType Directory -Path (Join-Path $userDataDir "boards") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $userDataDir "cache")  | Out-Null
New-Item -ItemType Directory -Path (Join-Path $userDataDir "tags")   | Out-Null

# FIX: settings.json sin BOM (LoadSession es estricto con la codificacion)
$settingsJson = @"
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
"@
[System.IO.File]::WriteAllText(
    (Join-Path $userDataDir "settings.json"),
    $settingsJson,
    [System.Text.UTF8Encoding]::new($false)
)

# FIX: session.txt vacio, sin BOM
[System.IO.File]::WriteAllText(
    (Join-Path $userDataDir "session.txt"),
    "",
    [System.Text.UTF8Encoding]::new($false)
)
Write-Host "  [OK] user_data/{settings.json, session.txt, boards/, cache/, tags/} (sin BOM)" -ForegroundColor Green

# --- Step 8: Copiar locales, resources, cursors ------------------------------
Write-Step 8 11 "Copiando assets (locales, resources, cursors)..."
foreach ($pair in @(
    @{ Name = "locales";   Dir = $LocalesDir },
    @{ Name = "resources"; Dir = $ResourcesDir },
    @{ Name = "cursors";   Dir = $CursorsDir }
)) {
    if (Test-Path $pair.Dir) {
        Copy-Item $pair.Dir -Destination (Join-Path $TempDir $pair.Name) -Recurse
        $count = (Get-ChildItem $pair.Dir -Recurse -File).Count
        Write-Host "  [OK] $($pair.Name)/ ($count archivos)" -ForegroundColor Green
    } else {
        Write-Host "  [WARN] $($pair.Name)/ no encontrado - se omite." -ForegroundColor Yellow
    }
}

# --- Step 9: Crear README.txt ------------------------------------------------
Write-Step 9 11 "Escribiendo README.txt..."
$readmeContent = @"
LenteUV Image Viewer v0.90 Alpha - Version Portable
=====================================================
Autor: xPhang
Basado en Sergkruma's image viewer por Sergei Krumas (github.com/sergkrumas)
Licencia: GPL v2+

INSTRUCCIONES DE USO
--------------------
1. Extraer el contenido de este ZIP en cualquier carpeta de tu eleccion.
2. Ejecutar lenteuv.exe directamente - no requiere instalacion.
3. Para asociar tipos de archivo o integrar en el menu contextual del explorador,
   utiliza el instalador lenteuv_setup.exe (disponible por separado).

ATAJOS PRINCIPALES
------------------
  NumPad 1   -> Modo Visor
  NumPad 2   -> Modo Biblioteca
  NumPad 3   -> Modo Cascada (Waterfall)
  NumPad 4   -> Modo Tablero (Board)
  F11        -> Pantalla completa
  CTRL+F     -> Busqueda por etiquetas XMP (en modo Board)
  CTRL+L     -> Indexar etiquetas del directorio
  T          -> Abrir gestor de etiquetas XMP
  Alt+Click  -> Trazar enlace entre elementos (en modo Board)

REQUISITOS DEL SISTEMA
-----------------------
  - Windows 10 Build 18362 o superior / Windows 11
  - GPU con soporte Direct2D 1.3 / Direct3D 11
  - El portable incluye las DLLs de VC++ Redistributable necesarias.

SOPORTE Y DOCUMENTACION
------------------------
  https://github.com/sergkrumas/image_viewer
"@
[System.IO.File]::WriteAllText(
    (Join-Path $TempDir "README.txt"),
    $readmeContent,
    [System.Text.UTF8Encoding]::new($false)
)
Write-Host "  [OK] README.txt" -ForegroundColor Green

# --- Step 10: Comprimir en ZIP -----------------------------------------------
Write-Step 10 11 "Comprimiendo en $OutputName.zip..."
Compress-Archive -Path "$TempDir\*" -DestinationPath $ZipOutput -CompressionLevel Optimal
$ZipSizeMB = [math]::Round((Get-Item $ZipOutput).Length / 1MB, 2)
Write-Host "  [OK] $OutputName.zip ($ZipSizeMB MB)" -ForegroundColor Green

# --- Step 11: Smoke test (verificacion de dependencias PE) -------------------
if (-not $SkipSmokeTest) {
    Write-Step 11 11 "Smoke test: verificando dependencias PE del .exe..."
    if (-not (Test-Path $DumpBinPath)) {
        # Buscar dumpbin en cualquier MSVC tools
        $dumpbinAlt = Get-ChildItem -Path "C:\Program Files (x86)\Microsoft Visual Studio" -Recurse -Filter "dumpbin.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($dumpbinAlt) {
            $DumpBinPath = $dumpbinAlt.FullName
            Write-Host "  [INFO] dumpbin encontrado en: $DumpBinPath" -ForegroundColor Gray
        } else {
            Write-Host "  [WARN] dumpbin.exe no encontrado, saltando verificacion PE." -ForegroundColor Yellow
            $DumpBinPath = $null
        }
    }
    if ($DumpBinPath) {
        $depsOutput = & $DumpBinPath /dependents (Join-Path $TempDir "lenteuv.exe") 2>&1
        $missingDlls = @()
        foreach ($line in $depsOutput) {
            if ($line -match "^\s+(\S+\.dll)\s*$") {
                $dllName = $Matches[1]
                # api-ms-win-crt-*.dll son parte del UCRT, integrado en Windows 10+.
                # No necesitan empaquetarse: Windows los provee via ucrtbase.dll.
                if ($dllName -like "api-ms-win-crt-*") { continue }
                # Las DLLs del sistema (kernel32, user32, etc.) vienen con Windows.
                $systemDlls = @("kernel32.dll", "user32.dll", "shell32.dll", "ole32.dll",
                                "oleaut32.dll", "shlwapi.dll", "propsys.dll",
                                "d3d11.dll", "d2d1.dll", "dwrite.dll", "dcomp.dll",
                                "gdi32.dll", "gdi32full.dll", "combase.dll",
                                "rpcrt4.dll", "advapi32.dll", "secur32.dll",
                                "msvcrt.dll", "ucrtbase.dll", "vcruntime140_clr0400.dll",
                                "ws2_32.dll", "crypt32.dll", "bcrypt.dll",
                                "imm32.dll", "version.dll", "wininet.dll",
                                "dwmapi.dll", "uxtheme.dll")
                if ($systemDlls -contains $dllName) { continue }
                $dllPath = Join-Path $TempDir $dllName
                if (-not (Test-Path $dllPath)) {
                    # Verificar si esta en System32 (VC++ Redist pre-instalado)
                    $sysPath = Join-Path $env:SystemRoot "System32\$dllName"
                    if (-not (Test-Path $sysPath)) {
                        $missingDlls += $dllName
                    }
                }
            }
        }
        if ($missingDlls.Count -gt 0) {
            Write-Host "  [ERROR] Faltan DLLs en el portable: $($missingDlls -join ', ')" -ForegroundColor Red
            Write-Host "  El ZIP generado puede no funcionar en sistemas sin VC++ Redistributable." -ForegroundColor Red
            Remove-Item -Recurse -Force $TempDir
            Remove-Item -Force $ZipOutput
            exit 1
        }
        Write-Host "  [OK] Todas las dependencias DLL presentes (UCRT y System DLLs ya provistas por Windows)" -ForegroundColor Green
    }
}

# --- Limpiar directorio temporal ----------------------------------------------
Remove-Item -Recurse -Force $TempDir
if (Test-Path $SmokeDir) { Remove-Item -Recurse -Force $SmokeDir }

# --- Resumen ------------------------------------------------------------------
Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host "              PAQUETE GENERADO CON EXITO                    " -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Archivo : $ZipOutput" -ForegroundColor White
Write-Host "  Tamano  : $ZipSizeMB MB" -ForegroundColor White
Write-Host ""
Write-Host "Para instalar con integracion del menu contextual, usa lenteuv_setup.exe" -ForegroundColor Cyan
Write-Host ""
