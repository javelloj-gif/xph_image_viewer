# Compilación, pruebas y distribución

## Entorno y comandos

Windows x64, Visual Studio 2022 Desktop development with C++/Windows SDK, CMake 3.21+
y PowerShell (5.1 o posterior). La versión mínima de CMake se elevó de 3.20 a 3.21
porque el preset utiliza el generador Visual Studio 17 2022. No hay paquetes C++
externos que descargar para compilar.

Desde la raíz:

```powershell
cmake --preset windows-x64
cmake --build --preset debug
ctest --preset debug
cmake --build --preset release
ctest --preset release
```

Alternativa con descubrimiento de herramientas:

```powershell
./scripts/build.ps1 -Configuration Release -Test
./scripts/build.ps1 -Configuration Debug -CMakePath 'C:/ruta/cmake.exe'
```

`build.ps1` configura antes de compilar. `-BuildDirectory` permite una carpeta de build
alternativa para desarrollo. Los scripts de empaquetado consumen el Release estándar
de `build/windows-x64/bin/Release/`. Los presets son multi-configuración y comparten
el árbol de build; no ejecutes dos builds simultáneos en él.

La aplicación enlaza bibliotecas Windows SDK y usa PCH. Al compilar copia `locales/`
y crea `user_data/` junto al ejecutable sin sobrescribir datos existentes. El ICO
versionado permite compilar sin Python. Para regenerarlo:

```powershell
python -m pip install Pillow
python scripts/generate_icons.py
```

El generador prefiere `assets/branding/lenteuv.png`; CairoSVG es opcional para la fuente
SVG si falta el PNG. Revisa el diff del ICO antes de confirmarlo.

## Tests aislados

CTest registra `metadata.tagging`, que llama a `run_tagging_tests.ps1`. Cada ejecución
copia el EXE del harness y dos fixtures a `build/windows-x64/test-runs/<id>/`, crea
`user_data/` local y ejecuta allí. Los artefactos se conservan para diagnóstico.
Las muestras versionadas y los datos personales no se alteran. El harness incluye
tags JPEG/PNG, cache, búsqueda, cancelación, propiedades ampliadas y palabras clave
de Windows; no cubre renderizado ni comportamiento visual.

El stub de Renderer del harness no es el backend de producción. El timeout de CTest
es 90 segundos. Una restricción del sandbox puede impedir escritura/Property Store:
registrar el error y repetir con permiso del entorno, sin desactivar assertions.

## Paquetes

```powershell
# Usa el ICO versionado; compila, prueba y genera ambos formatos.
./scripts/build_release.ps1 -SkipIcons
# Solo portable desde el binario Release ya compilado.
./scripts/create_portable_zip.ps1 -SkipBuild
# Solo instalador; falla si no está Inno Setup.
./scripts/build_release.ps1 -SkipIcons -SkipBuild -Format Installer
```

`build_release.ps1` conserva `-SkipBuild`, `-SkipIcons` y `-Version` del flujo anterior.
`-Format` admite Portable, Installer o All; el generador ZIP conserva `-OutputName`
como nombre simple. `-CMakePath`, `-ISCCPath`, `-DumpBinPath` y `-RuntimeDirectory`
permiten rutas explícitas. `-PythonPath` pertenece al script de release.

La versión predeterminada se lee de `project(... VERSION ...)` en CMake. El recurso
Windows todavía conserva su versión histórica: no confundir la etiqueta del paquete
con una migración de versión de datos.

Las herramientas se buscan por PATH, `vswhere` y ubicaciones de instalación estándar.
El runtime VC++ x64 se copia desde `VC/Redist/MSVC/.../x64/Microsoft.VC143.CRT` de Visual
Studio; si se suministra `-RuntimeDirectory`, debe contener el conjunto correspondiente
de DLLs x64. El script comprueba imports directos del EXE y de las DLLs del staging
mediante dumpbin. Esa comprobación estática no certifica funcionamiento de GPU/códecs.

Cada paquete usa `build/staging/<id>/` nuevo: EXE, ICO, LICENSE, README, runtime VC++,
catálogos `.po` y defaults de distribución. Nunca copia `user_data/` del checkout.
Los defaults de `packaging/default-settings.json` conservan las decisiones del ZIP
anterior, sin BOM; `session.txt` nace vacío. No se incluyen muestras ni caches/logs.
ZIP e instalador usan el mismo staging. Los resultados van a:

- `dist/lenteuv_portable_<versión>.zip`
- `dist/lenteuv_setup_<versión>.exe`

Inno Setup 6 mantiene AppId/tareas/asociaciones del instalador anterior. Compilar el
instalador no lo instala. Probar instalación/desinstalación en una ubicación aislada
y sin registrar asociaciones globales cuando solo se valida la distribución.
Para QA se puede definir `MyAppId` al invocar ISCC directamente; usar una identidad
distinta de producción, `/CURRENTUSER /NOICONS /TASKS=""` y un destino temporal.
El instalador no sobrescribe datos existentes ni los elimina al desinstalar.
La app sigue necesitando escritura junto al EXE; Program Files con usuario sin
privilegios es una limitación previa, no una migración resuelta por esta reforma.

## Datos, Git y limpieza

Conservar datos locales antes de probar UI o compatibilidad. `user_data/` de la raíz
está ignorado; al lanzar el build nuevo no se migra automáticamente al directorio
adyacente al EXE. Copiar datos explícitamente a una prueba aislada si se desea comparar.
Los tableros `!board.kbrd` y catálogos `xmpTags.json` también pueden escribirse en las
carpetas externas de imágenes. Nunca probar allí sobre originales.

`build/` y `dist/` contienen todo lo generado. El estado `.kilo/` se mantiene local:
no borrar sus worktrees. Los builds/paquetes previos se conservaron como artefactos
locales de legado; sus caches CMake contienen rutas antiguas y no deben reutilizarse.
No usar `git add -f` para incluir datos ignorados. Consultar `git diff --check` y revisar
el diff antes de publicar.

El historial publicable retiró `native/build/` para cumplir el límite de archivos de
GitHub; los hashes de los últimos commits históricos cambiaron. La historia original
se conserva localmente en un bundle y una rama de respaldo, según [validación](validation.md).
Publicar ramas concretas; no usar `git push --all` sobre ese respaldo ni enviar referencias
históricas sin comprobar sus artefactos.

## Preparación para Spec Kit

Se reservan `.specify/` y `specs/`, sin carpetas simuladas ni instalación. El contexto
observado vive en `docs/`; futuras especificaciones describirán cambios deseados y
criterios de aceptación sin sobrescribir la documentación del comportamiento actual.
Cuando se decida instalar, consultar las instrucciones vigentes del
[repositorio oficial de GitHub Spec Kit](https://github.com/github/spec-kit), elegir
versión e integración y revisar los archivos generados antes de confirmar. Esta base
no contiene constitución ni comandos de agente generados.
