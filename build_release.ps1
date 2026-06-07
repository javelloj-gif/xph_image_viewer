# ==============================================================================
# build_release.ps1 - LenteUV Image Viewer Release Builder
# ==============================================================================
# Automatiza el pipeline completo de release:
#   1. Genera el icono .ico desde el SVG (Python + Pillow)
#   2. Compila el ejecutable en Release (CMake + MSVC)
#   3. Genera el paquete portable (ZIP)
#   4. Compila el instalador interactivo (Inno Setup)
#
# Uso:
#   .\build_release.ps1
#   .\build_release.ps1 -SkipBuild   (saltar compilacion C++, usar exe existente)
#   .\build_release.ps1 -SkipIcons   (saltar generacion de iconos)
# ==============================================================================

param(
    [switch]$SkipBuild  = $false,
    [switch]$SkipIcons  = $false,
    [string]$Version    = "0.90.0"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$CMakePath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$ISCCPath  = "C:\Users\xxx_h\AppData\Local\Programs\Inno Setup 6\ISCC.exe"

function Step { param($n, $total, $msg)
    Write-Host ""
    Write-Host "[$n/$total] $msg" -ForegroundColor Cyan
    Write-Host ("=" * 60) -ForegroundColor DarkCyan
}

$TotalSteps = 4

Write-Host ""
Write-Host "============================================================" -ForegroundColor Magenta
Write-Host "   LenteUV Image Viewer v$Version - Release Builder         " -ForegroundColor Magenta
Write-Host "============================================================" -ForegroundColor Magenta

# ─── PASO 1: Generar iconos ─────────────────────────────────────────────────
if (-not $SkipIcons) {
    Step 1 $TotalSteps "Generando iconos (.ico) desde SVG vectorial..."
    python "$ScriptDir\native\generate_icons.py"
    if (-not (Test-Path "$ScriptDir\native\src\resources\lenteuv.ico")) {
        Write-Host "[ERROR] El archivo lenteuv.ico no se genero correctamente." -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK] lenteuv.ico generado." -ForegroundColor Green
} else {
    Write-Host "[SKIP] Generacion de iconos omitida." -ForegroundColor Gray
}

# ─── PASO 2: Compilar ejecutable ────────────────────────────────────────────
if (-not $SkipBuild) {
    Step 2 $TotalSteps "Compilando lenteuv.exe en Release (CMake + MSVC)..."
    if (-not (Test-Path $CMakePath)) {
        Write-Host "[ERROR] cmake.exe no encontrado en: $CMakePath" -ForegroundColor Red
        Write-Host "  Instala Visual Studio 2022 Build Tools con 'Desktop development with C++'" -ForegroundColor Yellow
        exit 1
    }
    & $CMakePath --build "$ScriptDir\native\build" --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] La compilacion fallo con codigo $LASTEXITCODE" -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK] lenteuv.exe compilado." -ForegroundColor Green
} else {
    Write-Host "[SKIP] Compilacion C++ omitida." -ForegroundColor Gray
}

# ─── PASO 3: Paquete portable ZIP ───────────────────────────────────────────
Step 3 $TotalSteps "Generando paquete portable lenteuv_portable.zip..."
powershell -ExecutionPolicy Bypass -File "$ScriptDir\create_portable_zip.ps1" -OutputName "lenteuv_portable"
if (-not (Test-Path "$ScriptDir\lenteuv_portable.zip")) {
    Write-Host "[ERROR] No se genero lenteuv_portable.zip" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] lenteuv_portable.zip generado." -ForegroundColor Green

# ─── PASO 4: Compilar instalador Inno Setup ─────────────────────────────────
Step 4 $TotalSteps "Compilando instalador lenteuv_setup.exe (Inno Setup)..."
if (-not (Test-Path $ISCCPath)) {
    Write-Host "[WARN] ISCC.exe no encontrado en: $ISCCPath" -ForegroundColor Yellow
    Write-Host "  Instala Inno Setup 6 desde: https://jrsoftware.org/isinfo.php" -ForegroundColor Yellow
    Write-Host "  O con: winget install JRSoftware.InnoSetup --source winget" -ForegroundColor Yellow
    Write-Host "  El instalador .exe NO fue generado (los otros paquetes si estan listos)." -ForegroundColor Yellow
} else {
    & $ISCCPath "$ScriptDir\lenteuv_setup.iss" 2>&1 | Where-Object { $_ -match "Warning|Error|Successful" }
    if (-not (Test-Path "$ScriptDir\Output\lenteuv_setup.exe")) {
        Write-Host "[ERROR] No se genero Output\lenteuv_setup.exe" -ForegroundColor Red
        exit 1
    }
    $SetupSize = [math]::Round((Get-Item "$ScriptDir\Output\lenteuv_setup.exe").Length / 1MB, 2)
    Write-Host "[OK] lenteuv_setup.exe generado ($SetupSize MB)." -ForegroundColor Green
}

# ─── Resumen final ──────────────────────────────────────────────────────────
Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host "           RELEASE BUILD COMPLETADO CON EXITO               " -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Artefactos generados:" -ForegroundColor White
if (Test-Path "$ScriptDir\native\src\resources\lenteuv.ico") {
    $s = [math]::Round((Get-Item "$ScriptDir\native\src\resources\lenteuv.ico").Length/1KB,1)
    Write-Host "    [OK] native\src\resources\lenteuv.ico  ($s KB, 4 bandas)" -ForegroundColor Green
}
if (Test-Path "$ScriptDir\lenteuv_portable.zip") {
    $s = [math]::Round((Get-Item "$ScriptDir\lenteuv_portable.zip").Length/1MB,2)
    Write-Host "    [OK] lenteuv_portable.zip              ($s MB)" -ForegroundColor Green
}
if (Test-Path "$ScriptDir\Output\lenteuv_setup.exe") {
    $s = [math]::Round((Get-Item "$ScriptDir\Output\lenteuv_setup.exe").Length/1MB,2)
    Write-Host "    [OK] Output\lenteuv_setup.exe          ($s MB)" -ForegroundColor Green
}
Write-Host ""
