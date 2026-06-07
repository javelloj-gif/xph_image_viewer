# ==============================================================================
# create_portable_zip.ps1 - LenteUV Image Viewer Portable Package Builder
# ==============================================================================
# Recopila el binario compilado y sus dependencias para la version portable.
#
# Uso:
#   .\create_portable_zip.ps1
#   .\create_portable_zip.ps1 -OutputName "lenteuv_portable_v0.90"
#
# Resultado:
#   lenteuv_portable.zip  (o el nombre especificado) en el directorio raiz.
# ==============================================================================

param(
    [string]$OutputName = "lenteuv_portable"
)

$ErrorActionPreference = "Stop"

# --- Rutas -------------------------------------------------------------------
$ScriptDir    = Split-Path -Parent $MyInvocation.MyCommand.Path
$BinaryPath   = Join-Path $ScriptDir "native\build\bin\Release\lenteuv.exe"
$LocalesDir   = Join-Path $ScriptDir "locales"
$ResourcesDir = Join-Path $ScriptDir "resources"
$CursorsDir   = Join-Path $ScriptDir "cursors"
$TempDir      = Join-Path $ScriptDir $OutputName
$ZipOutput    = Join-Path $ScriptDir "$OutputName.zip"

# --- Verificaciones previas --------------------------------------------------
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "     LenteUV Image Viewer - Portable Package Builder        " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path $BinaryPath)) {
    Write-Host "[ERROR] No se encontro el ejecutable compilado:" -ForegroundColor Red
    Write-Host "        $BinaryPath" -ForegroundColor Red
    Write-Host ""
    Write-Host "  Compila primero el proyecto con:" -ForegroundColor Yellow
    Write-Host '  $cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"' -ForegroundColor Yellow
    Write-Host '  & $cmake --build native/build --config Release' -ForegroundColor Yellow
    exit 1
}

# --- Limpiar directorio temporal si existe ------------------------------------
if (Test-Path $TempDir) {
    Write-Host "[INFO] Limpiando directorio temporal existente..." -ForegroundColor Gray
    Remove-Item -Recurse -Force $TempDir
}
if (Test-Path $ZipOutput) {
    Write-Host "[INFO] Eliminando ZIP anterior..." -ForegroundColor Gray
    Remove-Item -Force $ZipOutput
}

# --- Crear estructura del paquete portable ------------------------------------
Write-Host "[STEP 1/5] Creando estructura del paquete portable..." -ForegroundColor White
New-Item -ItemType Directory -Path $TempDir | Out-Null

# --- Copiar ejecutable --------------------------------------------------------
Write-Host "[STEP 2/5] Copiando ejecutable..." -ForegroundColor White
Copy-Item $BinaryPath -Destination $TempDir
$ExeSize = (Get-Item $BinaryPath).Length
$ExeSizeKB = [math]::Round($ExeSize / 1KB, 1)
Write-Host "  [OK] lenteuv.exe ($ExeSizeKB KB)" -ForegroundColor Green

# --- Crear user_data/session.txt vacio para evitar heredar sesiones malformed ---
Write-Host "[STEP 2/5] Creando user_data/session.txt vacio..." -ForegroundColor White
$UserDataDir = Join-Path $TempDir "user_data"
New-Item -ItemType Directory -Path $UserDataDir | Out-Null
$SessionTxtPath = Join-Path $UserDataDir "session.txt"
"" | Out-File -FilePath $SessionTxtPath -Encoding UTF8
Write-Host "  [OK] user_data/session.txt (vacio)" -ForegroundColor Green

# --- Copiar locales (traducciones) --------------------------------------------
Write-Host "[STEP 3/5] Copiando traducciones (locales/)..." -ForegroundColor White
if (Test-Path $LocalesDir) {
    Copy-Item $LocalesDir -Destination (Join-Path $TempDir "locales") -Recurse
    $LocaleCount = (Get-ChildItem $LocalesDir -Recurse -File).Count
    Write-Host "  [OK] locales/ ($LocaleCount archivos)" -ForegroundColor Green
} else {
    Write-Host "  [WARN] Directorio locales/ no encontrado - se omite." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path (Join-Path $TempDir "locales") | Out-Null
}

# --- Copiar resources (assets graficos) ---------------------------------------
Write-Host "[STEP 4/5] Copiando recursos graficos (resources/)..." -ForegroundColor White
if (Test-Path $ResourcesDir) {
    Copy-Item $ResourcesDir -Destination (Join-Path $TempDir "resources") -Recurse
    $ResCount = (Get-ChildItem $ResourcesDir -Recurse -File).Count
    Write-Host "  [OK] resources/ ($ResCount archivos)" -ForegroundColor Green
} else {
    Write-Host "  [WARN] Directorio resources/ no encontrado - se omite." -ForegroundColor Yellow
}

# --- Copiar cursors (cursores personalizados) ---------------------------------
if (Test-Path $CursorsDir) {
    Copy-Item $CursorsDir -Destination (Join-Path $TempDir "cursors") -Recurse
    $CursorCount = (Get-ChildItem $CursorsDir -Recurse -File).Count
    Write-Host "  [OK] cursors/ ($CursorCount archivos)" -ForegroundColor Green
}

# --- Crear README portable ----------------------------------------------------
$ReadmeContent = @"
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

SOPORTE Y DOCUMENTACION
------------------------
  https://github.com/sergkrumas/image_viewer
"@
$ReadmeContent | Out-File -FilePath (Join-Path $TempDir "README.txt") -Encoding UTF8

# --- Comprimir en ZIP ---------------------------------------------------------
Write-Host "[STEP 5/5] Comprimiendo en $OutputName.zip..." -ForegroundColor White
Compress-Archive -Path "$TempDir\*" -DestinationPath $ZipOutput -CompressionLevel Optimal
$ZipSize = (Get-Item $ZipOutput).Length
$ZipSizeMB = [math]::Round($ZipSize / 1MB, 2)
Write-Host "  [OK] $OutputName.zip ($ZipSizeMB MB)" -ForegroundColor Green

# --- Limpieza del directorio temporal -----------------------------------------
Remove-Item -Recurse -Force $TempDir

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
