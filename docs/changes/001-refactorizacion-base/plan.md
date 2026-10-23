> Estado vigente 2026-10-23: todas las tareas de la base cerradas/dispuestas. [Cierre](closure.md) y [continuidad vigente](../013-cierre-base/resume.md). No ejecutar las recomendaciones históricas de abajo por inercia.

# Plan: 001 Refactorización base

Especificación: [spec.md](spec.md). Última actualización: 2026-10-22.
Estado: auditoría inicial, R1 y R2 completadas; recorrido visual R2 pendiente por entorno.

Actualización 2026-10-23: [inventario acumulado 004–011](coverage-004-011.md) revisado.
[012](../012-cierre-colas-ipc/result.md) implementa primera porción AR-005, cierre de
loader/servidor y payloads; no cierra cancelación por consumidor ni límites restantes.
Los siguientes apartados de ejecución R1/R2 conservan su evidencia histórica.

## Componentes y diseño

Auditoría estática por componentes, con prioridad a `App`, carpetas/Library/Waterfall,
loader, IPC y persistencia. Revisar otros consumidores según riesgo y registrar
explícitamente la cobertura restante. Usar el diseño actual como contexto, no imponer
una arquitectura nueva. Informe persistente: [review.md](review.md).

## Tareas y comprobaciones

- [x] T-01: Inventariar componentes y cobertura de la auditoría. Criterio AC-01.
- [x] T-02: Ejecutar el agente con la habilidad y verificar sus hallazgos. Criterio AC-02.
- [x] T-03: Proponer la primera porción y mapear hallazgos a trabajo posterior. Criterio AC-03.
- [x] T-04: Concretar alcance, archivos e invariantes de esa porción y sus pruebas.
  Resolver decisiones materiales antes de edición dependiente. Criterios AC-04/AC-05.
- [x] T-05: Implementar R1, revisar y validar. Criterios AC-04/AC-05.
- [x] T-06: Registrar resultado de R1, guías afectadas y pendientes. Criterio AC-06.
- [x] T-07: Concretar R2, dependencias, invariantes y comprobaciones. AC-04/AC-05.
- [x] T-08: Trasladar gestor/modelo, renombrar miembro consumidor y documentar API. AC-04/AC-05.
- [x] T-09: Validar R2, revisar diff y registrar evidencia/roadmap. AC-04/AC-05/AC-06.

## Trazabilidad de hallazgos

IDs locales al [informe](review.md). La auditoría no los corrigió; R1 resuelve
la duplicación AR-004 y R2 la ubicación/nombre de AR-001 con implementación y validación posteriores.

| Hallazgo | Tratamiento propuesto | Destino y estado |
| --- | --- | --- |
| AR-001 | Separar ubicación/nombres del gestor compartido | R2 completada; core/FolderDataManager y App::m_folderData; recorrido visual limitado por entorno |
| AR-002 | Identidad/vigencia de resultados async y regresiones | Resuelto en [004](../004-vigencia-miniaturas/result.md): tira y previews compartidos; 98 checks diferidos |
| AR-003 | Diagnóstico de detección/envío/recepción IPC | [005](../005-entrega-ipc/result.md) corrigió transporte/error/payload; presentación real posterior registrada en 003; sin ACK/elección atómica |
| AR-004 | Búsqueda común de datos preservando algoritmo | R1 completada; evidencia en result.md y revisión final en review.md |
| AR-005 | Caracterizar colas/cierre; distinguir descarte de cancelación | [012](../012-cierre-colas-ipc/result.md) completa primera porción: stop de admisión/cola y servidor cancelable. Pendientes consumidor, WIC activo, scanners y cliente síncrono |
| AR-006 | Especificar validación de caché con datos sintéticos | Endurecimiento posterior propuesto; en 003 solo investigar si explica tiempos |
| AR-007 | Comentarios exactos y contratos de componentes modificados | Criterio transversal AC-05; cobertura restante progresiva |

El orden R1/R2 es una propuesta de refactorizaciones puras. AR-002/AR-003 tienen
riesgos altos; una reproducción de estabilidad puede justificar adelantar su corrección
sin presentarla como limpieza mecánica.

## Ejecución de R1

### Contrato y archivos acordados antes de editar producción (2026-10-22)

- Base: `ac4a5da`, rama `codex/repository-foundation`, Git inicialmente limpio.
  El propietario autoriza únicamente R1; no hay decisiones materiales abiertas.
- `src/common/user_data_paths.h/.cpp`: función sin estado con directorio EXE explícito
  que devuelve la raíz elegida; adaptador Win32 que conserva buffer `MAX_PATH`,
  `GetModuleFileNameW` y sus actuales condiciones de error. Sin caché ni escrituras.
- Se consulta `exists(dir / "user_data")` para el directorio del EXE y cinco padres,
  en ese orden, sin exigir directorio ni archivo de datos concreto. En ausencia se
  devuelve el directorio del EXE, **no** un `user_data` inexistente. Se conservan
  excepciones de filesystem y repetición del padre en la raíz; no canonicalizar.
- `src/core/settings.cpp/.h`: adaptar `GetSettingsFilePath` y `PreInitDebugIpc`;
  `src/library/folder_data_manager.cpp/.h`: adaptar sesión/favoritos;
  `src/metadata/metadata_cache.cpp/.h`: adaptar caché. Cada consumidor conserva
  su nombre de archivo, lectura/escritura y comentario de responsabilidad.
- `tests/user_data_path_tests.cpp`, `CMakeLists.txt`: harness C++ sin UI con checks
  activos en Release y temporales propios. Primero ejecutar contra las cinco funciones
  actuales extraídas mecánicamente a un header local en `build/`, sustituyendo solo
  adquisición del EXE por entrada explícita. Después ejecutar los mismos casos contra
  el helper: ausencia, candidato adyacente, cada nivel 0..5, varios candidatos,
  nivel 6 excluido, candidato como archivo y selección independiente del archivo final.
  Comprobar también cambios entre llamadas (sin caché), ausencia de escrituras y raíz.
- Registrar el helper en aplicación y harness de metadatos. Configurar CMake, compilar
  Release completo y ejecutar CTest (nuevo test y `metadata.tagging` sobre copias).
  Revisar diff/comentarios con la habilidad y ejecutar `git diff --check`.
- Actualizar `review.md`, `result.md`, este plan, estado de roadmap y guía de desarrollo
  para el nuevo test. Sin recorrido UI: no cambia UI, hilos ni formatos. No se mide ni
  se declara rendimiento, ni se ejecuta la app sobre datos personales.

1. Caracterizar el algoritmo existente en árboles temporales antes de sustituirlo.
2. Introducir helper neutral con raíz de EXE como entrada comprobable y adaptar
   los cinco caminos duplicados, sin migrar datos ni alterar política de existencia.
3. Revisar diff y comentarios; ejecutar los casos de selección y build/CTest relevante.
4. Registrar rutas seleccionadas y límites de prueba; comparar tiempos solo si se mide
   una base equivalente. No presentar reducción de duplicación como mejora demostrada.

Resultado: caracterización previa de las cinco copias, 90 checks (18 por copia);
helper final, mismos 18 casos y adaptador Win32 (19 checks). Release completo y
CTest 2/2 pasaron. El snapshot temporal y harness previo quedan en `build/` ignorado;
la prueba permanente usa solo el helper productivo. Véanse [resultado](result.md)
y [revisión final](review.md#revisión-del-diff-r1--2026-10-22).

## Ejecución de R2

Alcance autorizado por el propietario el 2026-10-22: ejecutar R2 recomendada al cerrar
R1. Base `ac4a5da` más diff R1 sin commit, preservado; misma rama. El estado inicial
coincide con el cierre anterior. No se trasladan cambios a otra rama ni se hace commit.

- Mover `src/library/folder_data_manager.h/.cpp` a `src/core/`, manteniendo clase,
  modelo `FolderInfo`/`PreviewItem`, disposición de miembros, firmas y enums.
  Renombrar solo `App::m_libraryData` a `m_folderData`. Conservar `friend` y referencias
  `m_folders`/`m_currentFolderIdx`, su orden de inicialización y propiedad en App.
- Actualizar `CMakeLists.txt`, includes de `core/app.h` y `library/library_page.h`,
  y consumidores en `core/app.cpp`, `library/library_page.cpp`, `board/board_page.cpp`.
  Waterfall y Viewer siguen consumiendo las mismas referencias; no requieren cambios.
- Documentar toda la API del gestor trasladado y modelos, el App* no propietario y
  uso en hilo UI. Describir escaneo síncrono, `libraryLoading` que suprime selección y
  guardado en AddFolder, `loadImage`, carga parcial de sesión, ordenación de previews,
  referencias mutables, historial en memoria y efectos de favoritos/sesión.
  Comentar también declaraciones de métodos consumidores modificados por el renombre.
- Excluir cambios de algoritmos, flags, comportamiento async, rutas R1, formatos,
  política de selección, gestos, logs, retirada de Library y nuevos servicios.
  La duplicación de `LibraryPage::AddFolder` se conserva con sus diferencias actuales.
- Guardar snapshot local previo de los archivos afectados en `build/r2-baseline/`.
  Comparar tokens C++ antes/después ignorando comentarios/espacios y normalizando
  exclusivamente rutas de include y nombre del miembro; exigir igualdad en todos
  los archivos de producción afectados. Confirmar que ya no hay referencias activas
  a ruta antigua/miembro antiguo y que CMake compila solo el nuevo archivo.
- Ejecutar configure, Release completo y CTest existente (rutas y metadatos sobre
  copias). No añadir tests que repliquen un traslado mecánico. Intentar recorrido
  Viewer/Library/Waterfall/Board con EXE/locales, fixtures y user_data propios bajo
  `build/`, sin datos personales, sin modificar asociaciones ni autorun. Verificar
  navegación/selección y restauración de sesión/favoritos; si el entorno limita la
  observación, registrar por separado qué se pudo comprobar y qué queda pendiente.
- Revisar el diff R2 frente al snapshot usando la habilidad y sus referencias;
  actualizar spec, plan, result, review, roadmap y mapa de servicios en contexto
  técnico. Las citas históricas de auditoría/R1 mantienen sus rutas de origen.

La porción es mecánica y no altera fases ni recursos de arranque; no requiere nueva
instrumentación de 003. No se interpreta la compilación como prueba de UX o rendimiento.

Resultado R2: equivalencia de tokens comprobada en los ocho archivos C++ afectados,
normalizando solo includes/miembro y omitiendo comentarios/espacios. CMake solo cambia
las dos rutas del gestor respecto al snapshot R1. Los hashes de los demás archivos
R1 se preservan. Release y CTest 2/2 pasaron.

El intento de recorrido con `computer-use` devolvió `computer-use request budget exhausted`
antes de obtener ventana. No se sustituyó por automatización nativa alternativa. Como
evidencia adicional de ejecución se construyó un harness local enlazado con los objetos
Release reales (todos salvo main.obj, incluido el objeto PCH, sin stubs), que ejecutó
36 checks de escaneo/selección/previews/sesión/favoritos/historial sobre copias con App
nulo, sin ventanas ni GPU. Pasaron. Fuentes y datos del harness quedan en `build/`.
El intento visual era comprobación complementaria de este traslado sin cambios de UI;
se conserva pendiente explícito y no se afirma equivalencia visual por los tests.
Véanse [resultado](result.md#r2--separación-neutral-del-gestor-compartido) y
[revisión](review.md#revisión-del-diff-r2--2026-10-22).

## Validación

Para la auditoría: comprobar ubicaciones, consumidores y límites; verificar enlaces
y `git diff --check`. No hace falta compilar para validar documentos.
Para implementar: seleccionar pruebas según el diff; usar comandos de desarrollo
para build/CTest y recorridos adicionales cuando afecte UI, concurrencia o persistencia.
Si afecta tiempos o recursos, medir una base antes mediante el protocolo de 003.
No usar CTest de metadatos como prueba de renderizado.

## Riesgos y recuperación

Una reorganización amplia puede mezclar cambios funcionales con mecánicos. Mantener
porciones revisables, datos aislados y cambios ajenos intactos. Separar lo exclusivo
de Library de los servicios que usan otros modos. No renumerar enums persistidos.

## Decisiones y ajustes

- 2026-10-22: prioridad de refactorización acordada; auditoría y creación de requisitos autorizadas.
- Agente especializado autorizado para analizar y escribir únicamente su informe;
  no fuentes, builds ni datos de ejecución.
- Informe inicial recibido y contrastado: se prioriza como recomendación R1 (AR-004)
  y después separación compartida R2 (AR-001); correcciones funcionales en trabajos propios.
- 2026-10-22: propietario autoriza implementación y validación rutinaria de R1.
  Se confirma contrato contra cinco resolutores actuales antes de editar producción.
  No se amplía alcance a otros hallazgos, i18n, caché de rutas ni política de errores.
- 2026-10-22: propietario autoriza R2. Se preserva el diff R1 sin commit; R2 queda
  limitado a traslado, nombre del miembro, includes/CMake y comentarios. La restricción
  de Computer Use impide el smoke visual, no las comprobaciones mecánicas ni de modelo.

## Estado para continuar

- Completado: auditoría inicial, R1 (AR-004) y R2 (AR-001), con validación de código/build/tests.
- En curso: ninguna otra implementación; 001 permanece abierto para cobertura posterior.
- Pendiente complementario: recorrer Viewer/Library/Waterfall/Board cuando Computer Use
  permita obtener ventana. No se validaron los efectos UI de App no nulo en el probe.
- AR-002 se implementó posteriormente en [004](../004-vigencia-miniaturas/result.md),
  entrega funcional independiente de R1/R2, con regresiones y validación propias.
- Diagnóstico IPC de 003 ejecutado posteriormente: fallo de envío reproducido, no corregido.
- Seguimiento posterior: [005](../005-entrega-ipc/result.md) corrige AR-003 en alcance;
  siguiente recomendado: validación real IPC/003 según roadmap, sin rediseño incidental.
  R2 ya prepara 002, pero sus decisiones de compatibilidad siguen abiertas.
