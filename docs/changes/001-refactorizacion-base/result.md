> Cierre final 2026-10-23: base completada en alcance; aceptación funcional reportada por el propietario y AR-006 resuelto en 013. Véase [matriz de cierre](closure.md). Los pendientes de abajo son históricos.

# Resultado: 001 Refactorización base

Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Estado: auditoría inicial, R1 y R2 completadas; 001 permanece abierto para otras porciones.
Fecha de validación de R1: 2026-10-22.

Actualización 2026-10-23: [inventario acumulado 004–011](coverage-004-011.md) enlaza
las revisiones posteriores y sus límites. [012](../012-cierre-colas-ipc/result.md)
completa primera porción de AR-005 con reproducción previa, Release/CTest 6/6 y
smoke nativo. AR-005 sigue abierto para cancelación por consumidor y límites restantes;
AR-006/007 y validación visual R2 conservan sus pendientes. Evidencia R1/R2 intacta.

## Cambios realizados

La auditoría inicial produjo siete hallazgos con cobertura estática parcial, conservados
como evidencia histórica en [review.md](review.md). No demostró fallos reproducidos.
El propietario autorizó después únicamente R1, resolución común de `user_data` (AR-004).

Se añadió `src/common/user_data_paths.h/.cpp`: una función comprobable con directorio
EXE explícito y un adaptador Win32 sin estado. Los cinco caminos de Settings (incluido
el diagnóstico temprano), FolderDataManager y MetadataCache usan la misma resolución.
Cada consumidor mantiene su nombre final. Se documentaron las funciones modificadas
y el harness; se registraron fuentes y prueba nueva en CMake.

Se conservan los seis candidatos (EXE y cinco padres), prioridad del más cercano,
`std::filesystem::exists` incluso para archivo, excepciones y fallback directamente
junto al EXE. Sin candidato, **no** se añade un directorio user_data al fallback.
No se migran datos ni se cambian formatos, MAX_PATH, permisos, i18n, UI o Library.
No se añade caché de rutas ni se afirma mejora de rendimiento.

## Caracterización anterior a la sustitución

Antes de editar producción se extrajeron mecánicamente los cuerpos de los cinco
resolutores actuales a `build/r1-characterization-source/legacy_paths.h`. Solo se sustituyó
la adquisición Win32 por `(exeDirectory / L"lenteuv.exe").wstring()`; para PreInit se
retuvo la selección y se devolvió la ruta antes de leer JSON. Los adaptadores de prueba
tomaron el padre de cada archivo seleccionado. No se reescribieron los bucles históricos.
Se contrastó el header generado con los originales antes de sustituirlos.

Un snapshot del test permanente, con selección de los cinco resolutores históricos,
se compiló como harness independiente, sin la aplicación. Los árboles temporales
son profundos para que los seis candidatos queden dentro de la prueba, con nombres
Unicode/espacios y expectativas explícitas independientes del bucle productivo.

| Escenario por resolutor | Checks |
| --- | --- |
| Ausencia, fallback EXE y sin creación de user_data | 1 |
| Solo séptimo candidato, excluido | 1 |
| Candidatos simultáneos agregados de nivel 5 a 0; gana el más próximo | 6 |
| Eliminación de candidatos observada en siguiente llamada | 1 |
| Candidato único en cada nivel 0..5 | 6 |
| user_data vacío preferido aunque existan archivos finales junto al EXE | 1 |
| Archivo user_data adyacente preferido a directorio ascendente | 1 |
| Consulta desde raíz de volumen, sin escrituras allí | 1 |

Resultado previo: **90 checks pasaron, 18 por cada una de las cinco copias**.
Después se retiró del test versionado la selección histórica y se ejecutaron los mismos
18 casos sobre el helper, más una comprobación de su adaptador Win32: **19 checks pasaron**.
Los checks usan excepciones, permanecen activos en Release y limpian su árbol propio.

Para reproducir la caracterización histórica, obtener con `git show ac4a5da:<archivo>`
los cinco cuerpos identificados en el plan, aplicar las sustituciones de entrada descritas
arriba y conectar sus raíces a los mismos casos de `tests/user_data_path_tests.cpp`.
El snapshot y harness de esta ejecución permanecen locales bajo `build/`, ignorado.
Comandos usados antes de sustituir producción:

```powershell
cmake -S build/r1-characterization-source -B build/r1-characterization -G 'Visual Studio 17 2022' -A x64
cmake --build build/r1-characterization --config Release
./build/r1-characterization/Release/r1_characterization.exe
```

## Evidencia de aceptación

Entorno R1: Windows x64, MSVC 19.43.34809, Visual Studio Build Tools 2022,
Windows SDK 10.0.22621.0, CMake 4.4.2. Base `ac4a5da`, rama
`codex/repository-foundation`, Git inicialmente limpio; revisión evaluada: diff R1 local
sobre esa base. Se preservan fixtures y datos personales; logs/builds no se versionan.

| Criterio | Comprobación | Resultado |
| --- | --- | --- |
| AC-01 | Inventario original de auditoría sobre `632bcda` y cobertura adicional R1 en review.md | Pasó, cobertura parcial explícita |
| AC-02 | Siete hallazgos originales y contraste de los cinco resolutores contra código actual | Pasó como auditoría estática; sin reproducción de los riesgos restantes |
| AC-03 | R1 elegida y relación de AR-001 a AR-007 con trabajo futuro | Pasó |
| AC-04 (R1) | Caracterización previa 90 checks; helper 19 checks; Release y CTest 2/2 | Pasó para contratos de resolución de R1 |
| AC-05 (R1) | Revisión de declaraciones, responsabilidades, propiedad del temporal y ausencia de estado compartido | Pasó en símbolos modificados/nuevos |
| AC-06 | Plan, resultado, revisión final, guía de tests y roadmap actualizados | Pasó para cierre R1; otras porciones siguen abiertas |

Validación final reproducible:

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release --verbose
git diff --check
```

Configure y compilación completa de aplicación, tagging_tests y user_data_path_tests:
correctos. CTest: **2/2 pasaron**, `common.user_data_paths` y `metadata.tagging`.
El harness de metadatos pasó JPEG/PNG, caché, búsqueda/cancelación, propiedades ampliadas
y Windows Keywords sobre las copias preparadas por `run_tagging_tests.ps1`.
`git diff --check` sin errores. No se desactivaron assertions ni se necesitó escalación.

## Revisión y limitaciones

La [revisión final del diff](review.md#revisión-del-diff-r1--2026-10-22), realizada con
la habilidad en esta sesión, no encontró hallazgos nuevos accionables ni bloqueantes.
AR-004 se considera resuelto por la unificación y evidencia; el resto no se implementó.

No hubo recorrido UI, profiling ni pruebas de errores inducidos de permisos,
GetModuleFileNameW o rutas largas. La preservación de esos tratamientos se revisó en código.
La matriz histórica prueba selección con entrada explícita, sin trasladar el EXE real por
cada árbol. El adaptador Win32 se ejecutó desde el harness y la integración productiva
se comprobó por diff y compilación; no se hicieron roundtrips de settings/sesión/favoritos.
El test de caché y metadatos no certifica renderizado, IPC ni UX.

## Cierre de R1 y recomendación entonces

**R1 completada.** La recomendación siguiente es concretar R2 (AR-001): separación
neutral de ubicación/nombre del gestor compartido, includes/CMake y comentarios,
conservando servicios, modelo y Library. R2 no está implementada ni autorizada por
este cierre. AR-002 conserva prioridad de riesgo y requiere una entrega correctiva
con identidad/vigencia y regresiones propias. No se declara completado todo el cambio 001.

## R2 — Separación neutral del gestor compartido

Autorizada posteriormente por el propietario el 2026-10-22. **R2 completada en su
alcance mecánico**, con recorrido visual pendiente por limitación del entorno.
Base: `ac4a5da` más R1 sin commit, en la misma rama; se preservaron los cambios R1.
Entorno: mismo Windows x64/MSVC 19.43.34809/SDK 10.0.22621.0/CMake 4.4.2 de R1.

### Cambio y preservación

`FolderDataManager` y los modelos `FolderInfo`/`PreviewItem` residen ahora en
`src/core/folder_data_manager.h/.cpp`. App posee `m_folderData` y mantiene las mismas
referencias a carpetas/índice. Se ajustaron CMake, los includes de App/Library y los
accesos del miembro en App, Library y Board. Viewer/Waterfall no necesitan cambios.

Se documentaron la API/modelos del gestor y los métodos consumidores modificados:
propiedad, hilo UI, escaneo síncrono, referencias prestadas, selección/carga,
`libraryLoading`, lectura/guardado e historial. No hay cambios en los cuerpos del gestor
respecto a R1, salvo comentarios y ubicación; su API conserva nombres/firmas.
Library mantiene su modo y su camino AddFolder propio, con sus diferencias actuales.

### Evidencia de aceptación R2

| Criterio | Comprobación | Resultado |
| --- | --- | --- |
| AC-04 | Ocho comparaciones de tokens sobre snapshot R1; normalizar solo includes y miembro, ignorar comentarios/espacios fuera de literales | Pasó: cuerpos, firmas, modelo, enums y orden de inicialización equivalentes |
| AC-04 | CMake coincide con snapshot R1 salvo las dos rutas; ausencia de referencias activas a ruta/miembro antiguos | Pasó |
| AC-04 | Hashes de archivos R1 ajenos al traslado, incluidos helper/tests de rutas y settings/caché | Conservados |
| AC-04 | Configure y compilación Release completa; CTest de rutas/metadatos sobre copias | Pasó, 2/2 |
| AC-04 | Probe local enlazado con objetos reales Release de producción, sin stubs, App nulo y datos propios | Pasó, 36 checks |
| AC-05 | Revisión de responsabilidades/efectos/hilo/propiedad de símbolos trasladados y declaraciones consumidoras modificadas | Pasó |
| AC-06 | Plan, especificación, revisión final, mapa técnico y roadmap | Actualizados; no se declara validación visual |

Comandos del build y tests permanentes:

```powershell
cmake --preset windows-x64
cmake --build --preset release
ctest --preset release --output-on-failure
git diff --check
```

La comparación local está en `build/r2-baseline/verify.ps1`, con snapshot anterior al
traslado. Tokeniza conservando literales; se ejecutó antes del cierre sin diferencias
inesperadas. CMake conserva las entradas R1; solo sustituye `src/library/folder_data_manager.*`
por `src/core/folder_data_manager.*`. Es evidencia de código, no de UX.

El probe local usa `build/r2-contract-source/main.cpp` y CMake independiente. Enlaza
los `.obj` del target lenteuv Release salvo main.obj, incluyendo PCH y todas las
implementaciones reales. No sustituye métodos de App, ImageLoader, Window o Renderer.
Solo invoca el gestor con App nulo, sin ejecutar inicialización de UI/GPU/IPC.

```powershell
cmake -S build/r2-contract-source -B build/r2-contract-probe -G 'Visual Studio 17 2022' -A x64
cmake --build build/r2-contract-probe --config Release
```

Se copia su EXE a un nuevo `build/r2-contract-run-<id>/`, junto a user_data vacío y
carpetas alpha/beta con copias de linux.png y noname.jpg, renombradas 01-linux.png y
02-photo.jpg. Se ejecuta allí; el probe exige un nombre/directorio de ejecución propio
y root local antes de escribir. Comprueba: selección diferida/normal y duplicados,
ordenación descendente con asociación de previews, índices válidos/-1/fuera de rango,
sesión de dos carpetas con bytes de cuatro líneas y restauración sin duplicar,
favoritos con bytes UTF-8/restauración/toggle, historial de máximo 100/deduplicación,
retirada de carpetas y reinicialización. **36 checks pasaron**, activos en Release.
Las reparaciones iniciales de build del harness fueron incorporar /wd4100 como el
target productivo y enlazar el objeto PCH; no requirieron cambios de producto.
Fuentes/datos del probe y snapshot permanecen locales e ignorados, no como nueva
infraestructura de pruebas versionada para un traslado mecánico.

### Revisión y límites R2

La [revisión final](review.md#revisión-del-diff-r2--2026-10-22), con la habilidad de
arquitectura y sus criterios, no encontró hallazgos nuevos accionables ni bloqueantes.
AR-001 queda resuelto en el alcance neutral acordado; no se implementan otros hallazgos.
La guía técnica refleja ahora la ubicación compartida y la resolución de rutas R1.
Las secciones de evidencia histórica R1 conservan sus rutas y recomendaciones de entonces.

Se preparó una copia aislada del visor con locales, dos carpetas de fixtures, settings,
sesión y favoritos propios. El intento de lanzar/controlar mediante la habilidad `computer-use`
devolvió `computer-use request budget exhausted`; no se obtuvo ventana de LenteUV ni
se observó un proceso de esa copia, y los datos preparados conservaron hashes.
El recorrido Viewer/Library/Waterfall/Board no se realizó. Es una limitación de la
herramienta, no evidencia de fallo del visor. Queda pendiente repetirlo cuando haya
acceso: restaurar las dos carpetas, navegar en Viewer, seleccionar carpeta/preview en
Library, abrir/cerrar el modal Waterfall y entrar en Board sobre esas mismas copias.

El probe no valida carga de imagen/redraw/strip con App no nulo ni efectos gráficos;
la preservación de esos caminos se apoya en equivalencia mecánica y compilación.
No se midió rendimiento ni se validaron carpetas grandes, async o formatos binarios.
No se cambió ninguna decisión de producto de 002 ni se declararon corregidos AR-002/003/005/006.

Siguiente paso recomendado: especificar AR-002 con identidad/vigencia y regresiones
con entrega diferida antes de implementar la corrección. La retirada de Library ya
puede apoyarse en core, pero conserva sus decisiones de compatibilidad pendientes.

## Continuación posterior a R2

AR-002 se implementó y validó con autorización posterior en el cambio funcional
[004](../004-vigencia-miniaturas/result.md). Las recomendaciones anteriores corresponden
a los cierres históricos de R1/R2. El siguiente paso propuesto es 003 empezando por IPC,
según la hoja de ruta; 002 conserva sus decisiones pendientes.

## Seguimiento posterior: 003 y 005

003 instrumentó y reprodujo cinco fallos de envío con error 2 tras sondeo.
[005](../005-entrega-ipc/result.md) corrigió después AR-003 en transporte, resultado
de error y propiedad ante post fallido: cinco aperturas y par concurrente aplicados,
errores inducidos y conteos exactos; Release/CTest 4/4. Se conservan pendientes de
presentación real, arranque/reposo de 003 y decisiones de Library. No hay ACK,
elección atómica ni rediseño de cierre; no se declara mejora de rendimiento.
