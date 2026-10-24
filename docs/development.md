# Compilación, pruebas y distribución

## Entorno y comandos

Windows x64, Visual Studio 2022 Desktop development with C++/Windows SDK, CMake 3.21+
y PowerShell (5.1 o posterior). La versión mínima de CMake se elevó de 3.20 a 3.21
porque el preset utiliza el generador Visual Studio 17 2022. No hay paquetes C++
externos que descargar para compilar.

003 utiliza SQLite de Windows (`winsqlite/winsqlite3.h`, `winsqlite3.lib`) y
Windows Compression API (`compressapi.h`, `cabinet.lib`) para la caché prescindible.
El SDK 10.0.22621.0 usado en la validación los incluye; no se incorpora otra copia
de SQLite ni se descarga una dependencia.

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

La configuración actual contiene ocho suites. `loader.startup_cache` usa
`run_startup_cache_tests.ps1`, otro EXE aislado y copias propias de fixtures/datos.
Sus 94 comprobaciones cubren píxeles exactos entre generación y persistencia,
variantes, firmas, Unicode, corrupción, cuotas/LRU, lectura entre procesos,
prioridades/cancelación, snapshots y sesión pendiente. WIC/WARP comprueban entrega
en UI; no certifican presentación nativa ni un plazo de 100 ms.

Para medir arranque usa `scripts/run_startup_benchmark.ps1` o la matriz secuencial
`scripts/run_startup_comparison.ps1`. Ambos requieren un RunRoot nuevo bajo build,
leen los originales y crean datos/cachés propios junto a una copia del EXE.
La instrumentación QPC se activa con `LENTEUV_PERF_OUTPUT` absoluto y exporta al
salir; por defecto está desactivada. Las variables LENTEUV_PERF_* de escenario,
duración y salida son del ejecutor, sin preferencias ni opciones visuales nuevas.
Cada ejecución guarda CSV, manifest, hashes/configuración y trazas localmente.
No ejecutar builds u otras pruebas intensivas simultáneamente con la medición.
No confundir E/S lógica del proceso con lecturas físicas de disco ni caché de
aplicación vacía con una primera sesión de Windows. Véase la evidencia de 003.

`ipc.delivery_diagnostics` usa `run_ipc_diagnostics.ps1`: copia el harness de 004,
fixtures y user_data a un run nuevo. El mismo EXE ejecuta roles sender en procesos
propios; el receptor usa pipe exclusiva, Window/App/WIC reales y D2D software WARP,
sin ventana visible ni swapchain. Comprueba imagen/bitmap aplicado, no presentación.
Se ejecuta con `ctest --preset release -R ipc.delivery_diagnostics --verbose`.
Conserva el nombre histórico de CTest/script/CLI de 003; desde 005 es una regresión
que exige cinco aperturas y dos concurrentes aplicadas exactamente una vez, conteos
finales sin duplicados y errores explícitos. Añade pipe temporalmente ausente, plazo
agotado por ausencia/saturación (también App::Initialize) y PostMessage con HWND destruido.
Véanse los resultados de 003 (histórico) y [005](changes/005-entrega-ipc/result.md).
El harness no automatiza el diálogo/salida 2 de main; esa parte se revisó en código.
La [validación real posterior de 003](changes/003-diagnostico-rendimiento/real-validation.md)
sí observó ese diálogo y salida 2 en una copia del producto, además de presentación
PNG y ventanas de ambos flags. Es evidencia separada del harness, con límites de
captura inicial --lite; no convierte CTest en certificación visual ni de rendimiento.
No contacta con la pipe global ni arranca App completo con --multiple/--lite; esos flags
solo se verifican en parser/bypass. El timeout CTest es 60 s, cada hijo tiene plazo de 5 s.

`ui.async_previews` enlaza las implementaciones productivas de App/páginas/loader sin
ejecutar App::Initialize. Usa WIC con cero workers, colas reales y entrega diferida
mediante ProcessPendingTextures; una ventana solo de mensajes recibe redraws y un
bitmap D2D 1x1 con WARP software comprueba asociación exitosa en la tira. No hay
ventana visible, swapchain ni datos personales. Cubre reemplazo/lista vacía/menor,
reordenación, borrado en memoria, recreación de rutas, éxito/fallo y entrega repetida.
Los checks permanecen activos en Release. Ejecutar por separado con
`ctest --preset release -R ui.async_previews --output-on-failure`.
Necesita WIC y backend WARP/Direct2D del sistema; no certifica UX ni rendimiento.

[011](changes/011-vigencia-antes-upload/result.md) añade 321 checks de vigencia antes
del upload: tarea CPU real en hilo COM con entrega controlada, píxeles sintéticos y
renderer WARP para observar bitmap omitido/creado. Comprueba callback/hilo/resultado,
reencolado, reemplazo, recreación, duplicados y revisión 007. Total async: 162022 checks;
no es un benchmark ni certifica presentación nativa. CTest sigue teniendo cinco suites.

Desde [006](changes/006-optimizaciones-locales/result.md), el mismo test conserva los
98 checks originales y añade reconstrucción diferencial frente al algoritmo previo y
visibilidad frente al recorrido completo (duplicados, tokens y límites float).
Dos modos optativos de `build/windows-x64/Release/async_preview_tests.exe`:
`--optimization-snapshot <archivo.bgra>` escribe 12 frames de tira/overlay reales en
un target WARP sin ventana/swapchain; `--optimization-benchmark` compara medianas de
reconstrucción con listas sintéticas. Crear previamente la carpeta de salida bajo
build. No escribe fixtures ni abre datos personales. Snapshot es BGRA crudo y requiere
baseline previo con el mismo harness/entorno para comparar; no certifica compositor
ni toda la UX. Benchmark no tiene umbrales en CTest ni demuestra arranque global.

[007](changes/007-layouts-por-cambios/result.md) añade al mismo harness comparación
bit a bit de layouts, hits/invalidation y callbacks compartidos. Modos optativos:
`--layout-snapshot <archivo.bgra>` escribe 24 frames WARP de grids productivos para
comparación con baseline previo; `--layout-benchmark` compara recálculo productivo
forzado con hits estables y reconstrucción por revisión. Datos sintéticos, sin sesión
ni imágenes personales. Conserva modos/regresiones 006; en 007 había cuatro tests.
Los tiempos describen CalculateLayout, no Draw/Present, GPU o arranque completo.

[008](changes/008-formatos-texto/result.md) añade propiedades/medidas de texto,
aislamiento público/privado, expulsión, límites y reemplazo/descarte de factory al
mismo harness. `--text-snapshot <archivo.bgra>` y `--text-reference-snapshot <archivo.bgra>`
comparan 12 frames WARP cacheados frente a formatos frescos. `--text-benchmark` separa
obtención/configuración y MeasureText completo; datos sintéticos y sin umbral temporal.
La evidencia incluye una captura inicial distinta, controles frescos/original y cero
bytes distintos en el contraste controlado. No descartar una diferencia visual sin
esos controles ni cambiar tipografía/antialiasing para hacer pasar la prueba.
En 008 se mantuvieron cuatro tests; la recuperación real de dispositivo y toda UX nativa
no quedan certificadas por esos casos. Artefactos siempre bajo build ignorado.

[009](changes/009-decoder-compartido/result.md) añade el quinto CTest,
`loader.viewer_decode`. Ejecutar `ctest --preset release -R loader.viewer_decode`.
El wrapper scripts/run_decode_tests.ps1 prepara copia del EXE/user_data/fixtures;
el modo `--decode-tests` solo debe ejecutarse en ese staging aislado. Usa WIC real y
proxies de test para contar aperturas/inyectar fallos; compara bytes/metadata y crea
PNG/GIF/WebP diminutos propios sin dependencia Python. El WebP grande se omite antes
de decode completo por presupuesto de frames estimados; codec ausente o APNG de un
frame no certifican animación de esos formatos. El benchmark CPU es informativo,
no un umbral de CTest ni medida de App/primera ejecución/GPU.

CTest también registra `common.user_data_paths`: prueba la selección de raíz de datos
en árboles temporales propios, con checks activos en Release. Cubre los seis candidatos,
prioridad, exclusión del séptimo, fallback, entradas de tipo archivo y resolución sin caché;
no abre la aplicación ni modifica datos personales. Para ejecutarlo por separado:
`ctest --preset release -R common.user_data_paths --output-on-failure`.

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

Desde [012](changes/012-cierre-colas-ipc/result.md), CTest incluye una sexta suite,
`lifecycle.shutdown`. El harness lanza nueve hijos propios con watchdog de 5 s por
proceso y pipes exclusivas. Comprueba cierre con cola ocupada/cliente silencioso,
payload pendiente, WM_QUIT y reinicio; no accede al endpoint global ni datos personales.
Solo el watchdog de tests termina sus hijos si fallan; producto no fuerza hilos/procesos.
Ejecutar `ctest --preset release -R lifecycle.shutdown --output-on-failure`.
El smoke nativo opcional de 010 añade desde 012 hide-to-tray y WM_CLOSE real tras
post pendiente. El script conserva `-Startup`; usar RunRoot build/shutdown-012 para
aislar evidencia nueva. No sustituye aceptación visual/interactiva del usuario.

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

## Trabajo por cambios

El [flujo de trabajo](workflow.md) organiza especificación, plan y resultado en
`docs/changes/`. La [hoja de ruta](roadmap.md) registra prioridades y dependencias.
El contexto observado permanece en las guías existentes; las especificaciones
describen cambios deseados sin sustituir la documentación del comportamiento actual.
El procedimiento utiliza documentos versionados, las herramientas de Codex y los
comandos de compilación y validación de este repositorio, sin instalación adicional.

## Arranque secundario: validación 010

El harness IPC incluye secundarios por App reales y comprueba factories omitidas,
bitmap aplicado y errores 005. CTest conserva cinco suites y WARP/ventana oculta.
El smoke opcional requiere dispositivo gráfico nativo y puede mostrar brevemente
ventanas; usa EXE/user_data/fixtures copiados y endpoint privado:

```powershell
./scripts/run_ipc_diagnostics.ps1 -Executable build/windows-x64/Release/async_preview_tests.exe -Fixtures tests/fixtures -RunRoot build/optimization-010 -Startup
```

Cubre diez escenarios: reentrada inducida antes de WIC, flags, bypass, archivo,
carpeta, sesión/directorio actual y fallos HWND/WIC/renderer. Imports temporales solo
en el EXE de tests; no hooks productivos. Desde 002 esos arranques dibujan los tres modos restantes;
no se comprueban visualmente sus píxeles ni gestos. `-ReferenceExecutable` permite
comparar con un harness previo que admita `--ipc-app-reference`; no pasar el EXE de
producto. [Resultado 010](changes/010-inicializacion-secundaria/result.md) conserva
medición, hashes y candidato portable; revisión visual de usuario pendiente.

## Cierre base 013

La base está cerrada; el [resultado 013](changes/013-cierre-base/result.md) registra
aceptación funcional reportada y correcciones finales. CTest conserva seis suites.
metadata.tagging añade 164 prefijos KTAG truncados con ambos estados dirty, firma,
versión, cantidades imposibles y compatibilidad; solo sobre user_data del EXE copiado.
ui.async_previews añade trabajo de Board/búsqueda retenido durante destrucción de
App para comprobar join antes del cierre del loader. 162025 checks async finales.
No sustituye UX, agotamiento de memoria ni un plazo universal de cierre.

## AR-005 completado: cancelación 014

CTest tiene ahora siete suites; lifecycle.cancellation copia su EXE y PNG a un
directorio único con user_data propio mediante scripts/run_cancellation_tests.ps1.
Cubre los consumidores reales, hover/fallback, sort/delete, Board duplicado,
cancelación de catálogo y reinicio (42 checks). lifecycle.shutdown conserva los
casos anteriores y suma cuatro receptores IPC controlados: trece hijos con watchdog.
loader.viewer_decode suma cancelación en WIC/stream, callback de búsqueda y una
imagen sintética BMP de 64 MiB; 314 checks. No modificar fixtures ni usar datos personales.

Pruebas y portable final en [014/result](changes/014-cancelacion-ar005/result.md).
Para comprobar el producto empaquetado en una copia aislada, el harness admite
--ipc-package <lenteuv.exe> <test/seed.png>; exige user_data y fixture junto al EXE.
Lanza solo sus hijos con --multiple/--lite, recorre los tres modos restantes y envía NUM2 reservado por mensajes a
su HWND/PID y solicita cierre normal. El watchdog solo limpia sus propios hijos.
Conservar otra extracción limpia para el usuario. No certifica UX visual universal.

El contrato de loader distingue predicado 011 (rechaza upload pero entrega CPU)
de cancelación 014 por lease débil (omite trabajo/entrega). No capturar el lease
fuerte en callbacks. Los checks WIC son cooperativos; join espera la llamada ya
iniciada. La base/AR-005 quedan cerrados para implementar el siguiente cambio funcional.

## Retirada de Library: 002

La retirada elimina la página, su enum, routing, opciones y fuentes CMake exclusivas;
conserva FolderDataManager en core y las regresiones de Waterfall/tira/Board.
No incorpora diálogo, opción nueva ni migración de preferencias. Los casos de tests
exclusivos de Library se retiraron; snapshots/layout-benchmark actuales cubren solo
Waterfall, sin reinterpretar los resultados históricos de 007. El snapshot de layout
actual escribe 12 frames, no los 24 históricos con las dos páginas.
Validación final y límites en [resultado 002](changes/002-retirar-library/result.md).
