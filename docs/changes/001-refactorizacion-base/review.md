> Disposición final 2026-10-23: [cierre](closure.md) y [revisión 013](../013-cierre-base/review.md). Este informe conserva evidencia histórica; AR-005 global sigue diferido, AR-006 resuelto y AR-007 progresivo.

# Auditoría inicial de arquitectura y mantenibilidad

Fecha: 2026-10-22. Habilidad aplicada: [lenteuv-architecture-review](../../../.agents/skills/lenteuv-architecture-review/SKILL.md).
Base: `632bcdae0a8385ce123db6502421c04a2a1643db`, con documentación y habilidad locales sin commit.
`git diff --stat -- src` no mostró cambios de fuentes en la auditoría inicial. Las secciones
iniciales conservan esa evidencia histórica; las revisiones de los diffs R1/R2 se añaden al final.

## Objetivo y método

Orientar refactorizaciones pequeñas que preserven comportamiento y preparar la retirada de Library.
Se consultaron `AGENTS.md`, contexto técnico, flujo de trabajo, problemas conocidos, validación histórica,
la habilidad y sus dos referencias. Se inventarió `src/` mediante `rg --files` y se contrastó con las listas
activas de CMake. No se ejecutó la aplicación, compilación, tests, profiling ni escritura sobre datos.

Las ubicaciones siguientes son relativas a la raíz y sus líneas corresponden a la base indicada.
Una evidencia estática confirma la estructura o un camino posible; no confirma frecuencia, impacto
medido ni reproducción del fallo. La evidencia histórica de IPC se cita como antecedente documental,
sin presentarla como una reproducción realizada por esta auditoría.

## Inventario y cobertura

El siguiente inventario es histórico. La [cobertura acumulada 004–011](coverage-004-011.md)
actualiza las porciones revisadas y pendientes sin borrar sus límites. Primera
implementación posterior de AR-005 en [012](../012-cierre-colas-ipc/result.md).

**Completa** significa lectura del archivo indicado, no certificación funcional. **Parcial** indica lectura
de secciones concretas y búsquedas de consumidores. **Inventariado** significa localizado en `src/`
y CMake, sin revisión de implementación. El inventario cubre los componentes; la auditoría de sus
implementaciones es parcial.

| Componente | Archivos y símbolos inspeccionados | Cobertura y pendiente |
| --- | --- | --- |
| Entrada | `main.cpp`: entrada, diagnóstico, COM, resultados de Initialize | Completa del archivo; sin ejecución |
| Coordinación | `core/app.h`, `core/app.cpp`: propiedad, referencias al gestor, Initialize, Run, LoadImage, LoadFolder, ApplySortMode y comienzo de DeleteImage; referencias de SetPage/atajos/drop | Parcial. Revisar a continuación el resto de borrado, transformaciones, routing y render completo |
| Ventana | `core/window.cpp`: receptor del mensaje IPC, líneas 427–435 | Parcial. `window.h` y resto de ventana inventariados |
| IPC | `core/single_instance_ipc.h/.cpp`: ciclo de vida completo, cliente, servidor, envío | Completa de ambos archivos; sin reproducción |
| Configuración | `core/settings.cpp`: helpers JSON y resolución de rutas, PreInitDebugIpc, comienzo de Load | Parcial. Resto de Load/Save y `settings.h` pendientes |
| Carpetas | `library/folder_data_manager.h/.cpp`: modelo, escaneo, selección, ordenación, sesión, favoritos, historial | Completa de ambos archivos; sin prueba de persistencia |
| Library | `library/library_page.cpp`: LoadPreviewTexture, AddFolder y selección; búsquedas de consumidores de previews y cambios de modo | Parcial. `library_page.h` y resto de página pendientes; evitar invertir en su rediseño porque se retirará |
| Waterfall | `waterfall/waterfall_page.cpp`: LoadPreviewTexture y consumidores de carpetas/previews localizados | Parcial. Lectura completa de layout/modal/eventos y `waterfall_page.h` pendiente |
| Carga | `loader/image_loader.h`: API, estructuras, propiedad y colas; `.cpp`: Initialize/Shutdown/WorkerLoop/EnqueueTask, cargas async y ProcessPendingTextures | Parcial. Decodificadores y subida GPU completa pendientes. `frame_sequencer.h/.cpp` inventariados |
| Tira inferior | `ui/thumbnail_strip.cpp`: SetFileList, LoadThumbnail y llamadores desde App | Parcial. Resto de interacción y `thumbnail_strip.h` pendientes |
| Otros overlays | `ui/options_menu.h/.cpp`, `ui/tagging_modal.h/.cpp` | Inventariados. Búsquedas auxiliares no equivalen a lectura |
| Caché | `metadata/metadata_cache.h`; `.cpp`: resolución de ruta, LoadCache, comienzo de SaveCache | Parcial. Fin de serialización y normalización pendientes |
| Escáner | `metadata/async_directory_scanner.h/.cpp`: búsqueda, pool, cancelación y entrega de callbacks | Completa de ambos archivos; contrato diferente del loader; no se concluye auditoría completa de consumidores |
| Fachada XMP | `metadata/xmp_tag_manager.cpp`: Initialize y ReadTags, búsquedas de accesos a caché | Parcial. `xmp_tag_manager.h` y resto de operaciones pendientes |
| Lectores/escritores | `metadata/wic_metadata_reader_writer.h/.cpp`, `jpeg_segment_injector.h/.cpp`, `png_chunk_injector.h/.cpp` | Inventariados; búsqueda de temporales no certifica seguridad de escritura |
| Board | `board/board_page.cpp`: GetActiveDirectory, líneas 789–798 | Parcial. Búsquedas en search manager auxiliares; resto pendiente |
| Auxiliares Board | `board_item.h`, `board_canvas.*`, `board_item_renderer.*`, `board_selection_manager.*`, `board_text_editor.*`, `board_serializer.*`, `board_search_manager.*`, `board_tag_scanner.*`, `board_layout_helper.*`, `board_page.h` | Inventariados. Cada `.*` denota los archivos `.h` y `.cpp` existentes |
| Viewer | `viewer/viewer_page.h/.cpp` | Inventariados; transformaciones y gestos pendientes |
| Render | `rendering/renderer.*`, `animation_controller.*`, `animation_curve.*` | Inventariados; dispositivo, recursos y pérdida de dispositivo pendientes |
| Infraestructura | `common/types.h`, `common/pch.h/.cpp`, `shell/shell_integrator.*`, `i18n/i18n_engine.*` | Inventariados; búsqueda auxiliar de rutas i18n, sin equipararla al contrato de user_data |
| Recursos | `resources/app.rc`, `app.manifest`, `lenteuv.ico` | Inventariados, sin revisión visual/binaria |
| Build | `CMakeLists.txt`: fuentes activas, recursos locales y harness CTest | Lectura de configuración. Scripts y pruebas no auditados en esta etapa |

## Hallazgos

### AR-001 — Servicio compartido alojado y nombrado como parte del modo que se retirará

- **Ubicación:** `src/core/app.h:138` (`m_libraryData`) y `:151` (referencias al estado);
  `src/core/app.cpp:16` (constructor), `:591`, `:608`, `:639`; `src/library/folder_data_manager.h:19`
  (`PreviewItem`), `:27` (`FolderInfo`), `:40` (`FolderDataManager`);
  `src/waterfall/waterfall_page.cpp:357`; `src/board/board_page.cpp:789`.
- **Evidencia:** código. App usa el gestor para historial/carpeta/sesión; Waterfall consume sus previews;
  Board lo consulta para resolver el directorio. El gestor también actualiza selección, tira y carga mediante App.
  `LibraryPage::AddFolder` (`library_page.cpp:361`) duplica parte del escaneo del gestor, con diferencias
  de selección/guardado: no asumir que ambos contratos sean intercambiables.
- **Clasificación/prioridad/confianza:** deuda, media; estructura confirmada. Alta relevancia para retirar Library.
- **Consecuencia:** borrar `src/library/` en bloque eliminaría servicios de modos que deben permanecer.
  El nombre `libraryLoading` en `AddFolder` oculta una diferencia de efectos, no un tipo de decodificación.
- **Propuesta:** mover el gestor/modelo compartido a una ubicación neutral y renombrar su miembro consumidor;
  documentar explícitamente selección, escaneo y efectos de carga/guardado. Conservar inicialmente la estructura
  y referencias existentes. Alternativa: mantener ubicación hasta retirar Library; reduce el diff inmediato,
  pero mantiene una frontera engañosa. No extraer una jerarquía de páginas ni añadir nuevos patrones.
  La duplicación exclusiva de Library se retira con ese modo, sin reconstruirla como un nuevo servicio.
- **Coste:** pequeño para traslado/nombres/comentarios e includes/CMake; medio si se intenta separar persistencia
  y navegación, lo que debe quedar fuera de la primera porción.
- **Compatibilidad/validación:** preservar formatos, rutas, sorting, previews, referencias de App y controles;
  compilar y recorrer Viewer/Waterfall/Board; probar sesión/favoritos sobre copias si se toca su lógica.
- **Relación:** preparación necesaria del cambio de retirada; propuesta inicial de refactorización, no implementada.

### AR-002 — Resultados asíncronos asociados a posiciones mutables

- **Ubicación:** `src/ui/thumbnail_strip.cpp:25` (`SetFileList`), `:126` (`LoadThumbnail`);
  `src/waterfall/waterfall_page.cpp:357`; `src/library/library_page.cpp:334`;
  `src/library/folder_data_manager.cpp:401`; `src/core/app.cpp:684`.
- **Evidencia:** código. La tira captura `index` y escribe sin comprobar límites ni ruta al entregar el resultado.
  `SetFileList` sustituye el vector. Waterfall y Library comprueban tamaño, pero no identidad: la ordenación
  reorganiza previews y el borrado desplaza elementos. `ProcessPendingTextures` (`image_loader.cpp:438`)
  entrega posteriormente en el hilo principal; estar en el mismo hilo no conserva identidad entre solicitud y entrega.
- **Clasificación/prioridad/confianza:** riesgo alto; mecanismo estático confirmado, fallo no reproducido aquí.
- **Consecuencia:** cambiar a una lista menor antes de la entrega permite acceso fuera de rango en la tira;
  ordenar/borrar puede asignar una textura al archivo equivocado aun con índice válido.
- **Propuesta:** identificar solicitud por generación de lista/carpeta más identidad estable de archivo, comprobar
  vigencia antes de escribir y definir qué solicitudes se descartan. Alternativa mínima de comparar ruta y límites
  evita parte del problema, pero requiere resolver estados `isLoading` conservados por SetFileList y cambios de vuelta
  a la misma lista. Revisar ese contrato antes de implementar; un chequeo de tamaño aislado no basta.
- **Coste:** medio, acotado a consumidores y contrato de invalidación; no exige mover recursos D2D a workers.
- **Compatibilidad/validación:** preservar selección/orden y entrega en hilo principal; regresión con entrega diferida,
  cambio a lista vacía/menor, ordenación y borrado sobre copias; recorrido de imágenes para detectar asociación errónea.
- **Relación:** porción correctiva separada prioritaria; cubrir Waterfall/tira permanentes, no rediseñar Library.

### AR-003 — IPC confunde detectar otro proceso con entregar la petición

- **Ubicación:** `src/core/single_instance_ipc.cpp:79`–`:83` (`Initialize`), `:126` (`SendMessage`),
  `:169` (servidor), `:231`; `src/core/app.cpp:115`; `src/core/window.cpp:427`;
  `src/main.cpp` (tratamiento de `AlreadyRunning`).
- **Evidencia:** código. Initialize ignora el bool de SendMessage y devuelve el mismo resultado de salida aunque
  falle el envío. El servidor se inicia después del sondeo, sin elección atómica del propietario. PostMessageW no
  se comprueba al transferir un puntero heap. El receptor libera el puntero si recibe el mensaje. El timeout del helper
  no representa un plazo de confirmación del receptor; no hay acuse de apertura aplicada.
  `docs/known-issues.md` y `docs/validation.md` registran fallo histórico de entrega, sin probar cuál de estos puntos lo causó.
- **Clasificación/prioridad/confianza:** defecto de contrato de resultado, alto; confirmado en código.
  Carrera entre arranques y fallo de entrega son riesgos sin nueva reproducción.
- **Consecuencia:** el cliente puede salir exitosamente sin demostrar que la imagen cambió; dos arranques pueden
  considerarse primarios antes de crear pipe. Un PostMessage fallido deja el payload sin liberar.
- **Propuesta:** distinguir resultados de descubrimiento, envío y recepción; registrar transiciones y errores antes
  de cambiar protocolo. Después definir elección de instancia y propiedad del payload. Alternativa: conservar protocolo
  con manejo explícito de errores, como primer paso, sin afirmar garantía de apertura. Un mutex por sí solo no prueba entrega.
- **Coste:** medio/grande según cambios de protocolo y recuperación. No mezclar con simple traslado de archivos.
- **Compatibilidad/validación:** mantener rutas UTF-8 y flags multiple/lite; escenarios simultáneos, receptor ocupado,
  ruta vacía/inválida y cierre. Verificar la imagen del receptor, no solo exit code. Requiere definir resultado esperado ante fallo.
- **Relación:** enlazar al diagnóstico de arranque/IPC y derivar una corrección propia tras delimitar protocolo.

### AR-004 — Resolución de datos duplicada entre servicios

- **Ubicación:** `src/library/folder_data_manager.cpp:23`, `:37`; `src/core/settings.cpp:82`, `:96`;
  `src/metadata/metadata_cache.cpp:36`.
- **Evidencia:** código. Se repite búsqueda ascendente de hasta seis niveles desde EXE y fallback junto al EXE.
  Settings vuelve a realizarla en el diagnóstico temprano. CMake prepara un user_data al lado de cada binario.
  i18n busca otro recurso y no debe incorporarse automáticamente como si fuera el mismo contrato.
- **Clasificación/prioridad/confianza:** deuda media; duplicación confirmada, coste de ejecución sin medir.
- **Consecuencia:** una corrección de resolución puede divergir entre sesión, favoritos, preferencias y caché;
  el arranque temprano puede consultar una ubicación distinta si las copias evolucionan por separado.
- **Propuesta:** helper común para resolver una raíz de datos con el algoritmo actual y política explícita;
  separar resolución de nombres concretos. Preferir entrada de directorio EXE explícita para poder probarlo.
  Alternativa: mantener las copias documentadas hasta disponer de casos de caracterización; no migrar a AppData aquí.
- **Coste:** pequeño/medio, con adaptación de consumidores y pruebas de selección de ruta.
- **Compatibilidad/validación:** casos de primer user_data, varios ascendentes, ausencia y límite de seis niveles;
  usar directorios temporales y comprobar selección idéntica. No cambiar formatos ni rutas personales.
- **Relación:** candidata posterior independiente; opción segura si se prefiere iniciar por persistencia en lugar de AR-001.

### AR-005 — Contratos de cola y cierre carecen de cancelación por consumidor

- **Ubicación:** `src/loader/image_loader.h` (colas privadas); `image_loader.cpp:69`, `:84`, `:102`,
  `:410`, `:438`; `src/core/app.cpp:254`; `src/core/single_instance_ipc.cpp:94`–`:120`, `:192`, `:206`.
- **Evidencia:** código. El loader termina solo cuando shutdown y cola vacía; EnqueueTask no rechaza por shutdown.
  ProcessPendingTextures procesa todos los resultados intercambiados. IPC hace join después de intentar centinela;
  el servidor usa ConnectNamedPipe/ReadFile bloqueantes. Si falla el centinela se registra que se fuerza detención,
  pero el código mostrado no fuerza ni cancela ese IO antes de join.
- **Clasificación/prioridad/confianza:** riesgo medio/alto; caminos confirmados, latencias/bloqueo no reproducidos.
- **Consecuencia:** trabajo obsoleto puede retrasar salida y cargas vigentes; cierre IPC podría quedar esperando
  si no se logra desbloquear IO. No se afirma que esto ocurra en una ejecución normal.
- **Propuesta:** medir cola/tiempos y definir estados de aceptación, cancelación y cierre antes de alterar comportamiento.
  Distinguir descartar resultado (AR-002) de cancelar decodificación. Alternativa: conservar drenaje, pero documentar
  que es intencional y mantener límites medidos. Para IPC diseñar desbloqueo verificable, no confiar en el texto del log.
- **Coste:** medio/grande; separar loader e IPC en entregas propias.
- **Compatibilidad/validación:** sin GPU en workers; probar cierre con cola ocupada, repetido y cliente que conecta sin
  enviar. Medir fin de proceso, no solo desaparición de ventana. No cambiar política de cierre incidentalmente.
- **Relación:** insumo de diagnóstico y correcciones posteriores; no condición para el traslado neutral de AR-001.

### AR-006 — Lectura de caché binaria confía en longitudes y estado de stream

- **Ubicación:** `src/metadata/metadata_cache.cpp:50` (`LoadCache`), especialmente `:76`–`:116`.
- **Evidencia:** código. Verifica firma/versión, pero reserva strings/vectores con longitudes del archivo sin comprobar
  lecturas completas ni límites antes de reservar. Borra el caché vivo antes de terminar y retorna true al concluir el bucle.
- **Clasificación/prioridad/confianza:** riesgo medio; confirmado en código; no se probó archivo corrupto.
- **Consecuencia:** truncado o longitudes inconsistentes pueden provocar asignaciones grandes, datos parciales o éxito
  aparente. No se atribuye a este camino el arranque lento reportado sin medir el archivo y las fases.
- **Propuesta:** leer en estructura temporal con límites coherentes y comprobaciones del stream; publicar solo al completar.
  Alternativa: invalidar/regenerar caché corrupto con política explícita. Mantener KTAG v1 sin nuevo formato por defecto.
- **Coste:** medio; requiere decidir límites de recursos y tratamiento de errores.
- **Compatibilidad/validación:** fixtures sintéticos aislados: válido, truncado, longitud excesiva, versión no soportada;
  comprobar que un fallo no publica estado parcial. No usar caché personal como muestra.
- **Relación:** endurecimiento posterior, independiente de retirar Library; puede investigarse durante arranque.

### AR-007 — Comentarios públicos incompletos y garantías que superan la evidencia

- **Ubicación:** `src/library/folder_data_manager.h:40`–`:78` (Initialize, AddFolder, RemoveFolder,
  SetCurrentFolderIndex, LoadSession, SaveSession, favoritos, historial, SortFolder);
  `src/core/single_instance_ipc.h:31`–`:38`; `src/core/app.cpp:256`;
  encabezados `app.cpp:3`, `folder_data_manager.cpp:3`, `single_instance_ipc.h:3`.
- **Evidencia:** código. Encabezados con `[IMAGE_VIEWER_APP_NAME]`; secciones de FolderDataManager no explican
  los efectos de cada método. IPC declara cierre con garantía absoluta de ausencia de fugas/deadlock que el mecanismo
  observado no demuestra. Esto no significa que toda clase/método del repositorio carezca de comentario.
- **Clasificación/prioridad/confianza:** deuda de documentación, baja general; media en contratos de hilo/cierre;
  confirmada. No se declara infracción retroactiva de la regla para todo el código preexistente.
- **Consecuencia:** agentes y mantenedores pueden asumir selección/carga inexistente o garantías de cierre no verificadas.
- **Propuesta:** documentar cada clase/método modificado en su declaración con responsabilidad, propiedad, hilo y efectos
  pertinentes; corregir garantías al revisar el componente. Conservar atribuciones y evitar duplicación mecánica.
  Alternativa: mantener cobertura restante como deuda por componente. No añadir comentarios especulativos por volumen.
- **Coste:** pequeño por componente, grande si se pretendiera completar todo de una vez.
- **Compatibilidad/validación:** revisión cruzada de comentario, implementación y llamadores. No requiere ejecutar tests
  por comentarios solos; cambios de contrato real sí requieren validación funcional.
- **Relación:** criterio transversal para cada porción; la cobertura completa permanece progresiva.

## Orden recomendado y primera porción

1. **Porción inicial propuesta: AR-004, resolución común de user_data.** Extraer el algoritmo actual de búsqueda
   ascendente y fallback en un helper neutral; utilizarlo desde Settings (incluido diagnóstico temprano),
   FolderDataManager y MetadataCache. Preservar exactamente profundidad, orden, nombres y comportamiento de fallback.
   Caracterizar selección en árboles temporales con varios directorios candidatos antes de sustituir consumidores.
   Excluir i18n, migración a AppData, cachear la ruta, cambios de MAX_PATH y política de permisos: serían otras decisiones.
   Es una refactorización pura y acotada; no corrige por sí sola rendimiento ni errores de IPC.
2. **AR-001, separación neutral del gestor compartido**, como preparación de retirada Library. Mover sus dos
   archivos, ajustar CMake/includes/nombre de miembro y comentar API, conservando modelo y comportamiento.
   No retirar el modo ni cambiar Page en esa porción; si exige rediseño, reducirla.
3. **AR-002, corrección de invalidación asíncrona.** Prioridad alta de riesgo; entrega propia y criterios de regresión
   antes de modificar el loader. No construir una nueva infraestructura de miniaturas para la página que se retirará.
4. **Retirada funcional de Library.** Reutilizar la frontera neutral; eliminar exclusividades y resolver antes los casos
   pendientes de configuración antigua, entrada por carpeta y placeholder visual. No preservar por iniciativa un modo obsoleto.
5. **AR-003/AR-005/AR-006**, entregas independientes informadas por diagnóstico y su impacto confirmado.
   Un defecto de estabilidad reproducido puede cambiar este orden. No relegarlo por ser transversal.

AR-002 y las correcciones de AR-003/AR-005/AR-006 cambian resultados en escenarios de fallo o carrera: no presentarlas
como refactorizaciones puras. Su alcance necesita especificación y aceptación propia. El orden de refactorización pura
no afirma menor gravedad de estos riesgos.

AR-007 se aplica a los componentes modificados en cada entrega. No se propone una limpieza global de nombres,
eliminar `friend`, reescribir App ni incorporar patrones por catálogo. Los hallazgos pendientes no son automáticamente
bloqueos para entregas que no los tocan. La selección de porción es una recomendación pendiente de incorporar al plan.

## Enlace con el diagnóstico de rendimiento

No hay mediciones nuevas. La primera ejecución durante una sesión de Windows más lenta es un **reporte del usuario**;
no demuestra proceso residente, fuga ni decodificación GPU. Priorizar inicialmente arranque, IPC y reposo.

| Observación estática | Experimento pendiente |
| --- | --- |
| `App::Initialize:228` restaura sesión incluso con argumento explícito; `AddFolder` enumera y ordena sincrónicamente | Tiempos por fase con sesión vacía/controlada y mismo archivo explícito; identificar revisión y directorios |
| `App::LoadImage:573/:598` decodifica sincrónicamente y sube recursos en UI | Separar tiempo de decodificación, upload y primera presentación utilizable |
| `ThumbnailStrip::SetFileList:30` busca linealmente en cache por cada ruta | Medir tamaños y duración; complejidad potencial cuadrática no cuantifica impacto de arranque |
| `App::Run:268` renderiza y solo después espera hasta 16 ms | Medir CPU/GPU en reposo con imagen estática, oculto y visible; no llamar fuga al consumo |
| IPC detecta y envía con resultados ambiguos | Trazar cliente, servidor y recepción UI; validar imagen final en receptor |
| Cola se drena al cerrar y sube todos los completados por frame | Instrumentar profundidad, edad y duración por lote, sin declarar mejora antes de comparar |

Registrar si no queda ningún proceso tras cerrar; comparar primera ejecución de la sesión y posteriores usando la misma
revisión Release, entrada y datos controlados. Repetir sesiones para distinguir variabilidad y cachés del sistema. Mantener
logs y datos fuera de Git; versionar procedimiento y resumen. La base previa a refactorizar debe identificar su commit.

## Cierre y continuidad

- Comprobado: estado Git, inventario de fuentes, listas CMake, símbolos y líneas citadas, contratos del loader y escáner.
- No realizado: ejecución, compilación, pruebas, análisis dinámico, profiling, auditoría integral de UI/GPU/metadatos/Board.
- Decisiones pendientes: política general para valores de página inválidos (incluido el antiguo valor de Library),
  destino al abrir carpeta, etiqueta exacta del placeholder y comportamiento ante fallo IPC. La auditoría no los decide.
- Siguiente revisión: completar mutaciones de lista/selección en App, ThumbnailStrip y Waterfall para especificar AR-002;
  después revisar ciclo de vida completo de Board/scanners y persistencia de Board sobre su propio alcance.
- Una entrega de auditoría puede cerrarse con esta cobertura explícita; las refactorizaciones siguen sin implementar y
  sus criterios funcionales no están validados. Enlazar este informe desde plan/result y promover solo hallazgos aceptados
  a entregas verificables en la hoja de ruta.

## Revisión del diff R1 — 2026-10-22

Base de implementación: `ac4a5da`, Git inicialmente limpio en `codex/repository-foundation`.
Revisión en la misma sesión con la habilidad y sus criterios; sin agente adicional.
Alcance autorizado: únicamente AR-004. Las ubicaciones de esta sección corresponden
al árbol de trabajo R1, no a las líneas de la auditoría histórica.

### Cobertura y contraste

- Lectura completa del helper `common/user_data_paths.h/.cpp`, del nuevo test
  `tests/user_data_path_tests.cpp` y de CMake; lectura de `scripts/run_tagging_tests.ps1`
  para confirmar aislamiento. Se revisaron las declaraciones de los tres consumidores.
- Revisión de los cinco resolutores cambiados: `Settings::GetSettingsFilePath` (`settings.cpp:83`),
  `PreInitDebugIpc` (`:87`), `FolderDataManager::GetSessionFilePath` (`folder_data_manager.cpp:24`),
  `GetFavoritesFilePath` (`:28`) y `MetadataCache::GetCacheFilePath` (`metadata_cache.cpp:37`).
  Se siguieron sus usos Load/Save, entrada temprana `main.cpp:108` e inicialización
  de caché en `XmpTagManager::Initialize`; esos consumidores no se modificaron.
- `ResolveUserDataRoot` (`user_data_paths.cpp:6`) conserva seis consultas `exists`,
  orden desde EXE hasta quinto padre y fallback EXE. No exige tipo de entrada ni
  presencia del archivo final. `GetExecutableUserDataRoot` (`:17`) conserva adquisición
  Win32/MAX_PATH; sin estado compartido, migración, escrituras ni normalización añadida.
- Los nombres finales siguen siendo settings.json, session.txt, fav.txt y xmp_tag_cache.bin.
  CMake incluye el helper en aplicación y tagging_tests y registra el nuevo test sin PCH.
  Hay comentarios de responsabilidad para las funciones modificadas, helper, clase y
  métodos del harness. Las clases consumidoras conservan sus comentarios de responsabilidad.
- Fuera de esta revisión: i18n, UI/GPU, invalidación async, IPC, cierre, otros formatos,
  endurecimiento de caché y cobertura documental preexistente ajena al diff.

### Resultado y evidencia

Sin hallazgos nuevos accionables ni bloqueantes en el diff R1. AR-004 queda resuelto
en su alcance de duplicación; AR-001/002/003/005/006 permanecen pendientes, y AR-007
se cumple solo para los símbolos de esta porción. No se atribuyen fallos reproducidos
ni mejoras de rendimiento a la deduplicación.

Ejecución previa a producción: 90 checks sobre las cinco copias extraídas (18 cada una).
Ejecución posterior: mismos 18 casos del helper más adaptador Win32, 19 checks.
`cmake --preset windows-x64`, `cmake --build --preset release` y
`ctest --preset release --verbose` pasaron (2/2 tests). `metadata.tagging` pasó todos
sus casos sobre copias, incluido caché. Evidencia y reproducción: [result.md](result.md).

Límites: no se forzaron errores de permisos/Win32 ni rutas largas; se preserva su
tratamiento en código. No se ejecutó UX ni se midió rendimiento. La raíz del volumen
solo se consultó, sin crear candidatos allí. La revisión no certifica toda la aplicación.
Se recomienda concretar R2 como siguiente refactorización neutral; AR-002 conserva
su prioridad de riesgo y requiere su especificación correctiva propia.

## Revisión del diff R2 — 2026-10-22

Base: `ac4a5da` más el diff R1 sin commit. Git coincidía con el cierre de R1;
se guardó snapshot previo en `build/r2-baseline/`. Revisión en la misma sesión con
la habilidad y ambas referencias, sin delegación. Alcance: AR-001, traslado neutral.
La evidencia histórica anterior conserva sus ubicaciones originales.

### Cobertura

- Lectura completa del gestor/modelo antes y después: ahora en
  `src/core/folder_data_manager.h/.cpp`. Contratos comprobados contra implementación
  y consumidores: escaneo síncrono, selección, sesión/favoritos, historial, previews
  y ordenación. Los métodos de su API y modelos tienen comentarios de responsabilidad,
  hilo UI, propiedad y efectos. También se documentó FileEntry y las declaraciones
  consumidoras modificadas por el renombre.
- App conserva clase y propiedad del servicio (`app.h:142`, `m_folderData`) y referencias
  al mismo estado (`:156` y `:157`, constructor `app.cpp:16`). Sus rutas de inicialización,
  favoritos/sesión, historial, LoadFolder y ApplySortMode solo cambian el nombre del miembro.
- Library conserva su propio AddFolder y su comportamiento diferente. Solo cambian
  include y acceso al miembro en SetCurrentFolder (`library_page.cpp:398`) y OnMouseDown
  (`:610`). No se retira Library ni se unifica su escaneo con el gestor.
- Board conserva la resolución de directorio (`board_page.cpp:789`): lista actual,
  carpeta compartida seleccionada y current_path. Viewer y Waterfall no se modifican;
  se siguen sus accesos al modelo a través de App y se compilan ambos consumidores.
- CMake: solo se sustituyen las rutas del header/source del gestor respecto a R1.
  El helper de rutas R1 permanece y sus consumidores no cambian de política.
- No revisado de nuevo: UI completa de páginas, rendering/GPU, scanner/loader completos,
  IPC, formatos binarios ni cobertura documental preexistente ajena a los símbolos afectados.

### Comprobaciones y dictamen

Ocho comparaciones de tokens C++ pasaron frente al snapshot: gestor .h/.cpp,
App .h/.cpp, LibraryPage .h/.cpp y BoardPage .h/.cpp. Se ignoran comentarios/espacios
fuera de literales y se normalizan solo nombre del miembro y rutas de include conocidas;
se conservan strings, firmas, enums, expresiones, orden de inicialización y cuerpos.
Los archivos R1 ajenos al traslado conservan hashes; CMake coincide con R1 salvo dos
sustituciones de ruta. No quedan referencias activas al miembro/ruta anterior.

Release completo y CTest 2/2 pasaron. Un probe local enlazado con objetos reales de
producción, sin stubs ni App/ventana inicializada, pasó 36 checks de contratos y
roundtrips UTF-8 sobre copias. El intento visual mediante la habilidad Computer Use
falló con `computer-use request budget exhausted` antes de obtener ventana; no se
observó UX ni se prueban los efectos UI de App no nulo con el probe. No es un fallo
reproducido de LenteUV. Procedimiento y límites: [result.md](result.md#r2--separación-neutral-del-gestor-compartido).

Sin nuevos hallazgos accionables ni bloqueantes en el diff mecánico. AR-001 resuelto
en su alcance de ubicación/nombre/responsabilidad documentada; no se separan navegación
y persistencia ni se corrigen los riesgos de callbacks. AR-004 sigue resuelto por R1;
AR-007 se cumple en la porción, no globalmente. AR-002/003/005/006 permanecen pendientes.
No se midió rendimiento ni se declara mejora. El smoke visual queda pendiente como
comprobación complementaria; no se usa compilación/CTest como evidencia visual.

Siguiente entrega recomendada: especificación acotada de AR-002 con generación/identidad,
entrega diferida y cambios de lista/orden/borrado antes de modificar consumidores.
Las decisiones de compatibilidad de 002 siguen abiertas; R2 no autoriza retirar Library.

## Seguimiento de AR-002 — 2026-10-22

El propietario autorizó su corrección después de R2. [004](../004-vigencia-miniaturas/spec.md)
registra contrato, implementación y validación independientes: tokens por solicitud/ruta
en tira y previews compartidos. Una prueba previa reprodujo asociación errónea A/B sin
crash; tras corregir pasaron 98 checks diferidos y CTest 3/3. Véanse
[resultado](../004-vigencia-miniaturas/result.md) y [revisión](../004-vigencia-miniaturas/review.md).
AR-002 resuelto en ese alcance; las secciones anteriores conservan evidencia histórica.

## Seguimiento de AR-003 — 2026-10-22

La porción [IPC de 003](../003-diagnostico-rendimiento/result.md) añade una reproducción
controlada con procesos y handler Window/App reales: cinco detecciones seguidas de
error 2 al enviar, mientras controles directos aplican imagen con bitmap. El contrato
booleano y protocolo se conservaron; no se declara AR-003 corregido. Revisión y límites
en [003/review.md](../003-diagnostico-rendimiento/review.md). Arranque/reposo siguen pendientes.

Seguimiento posterior: [005](../005-entrega-ipc/review.md) corrige AR-003 en el alcance
de reintento al abrir, resultado explícito y payload de post fallido. La matriz exige
5/5 aplicaciones, dos concurrentes, errores inducidos y conteos exactos; Release/CTest
4/4 pasaron. Presentación real y elección/cierre/ACK no se consideran certificados.
