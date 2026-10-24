# Revisión de la porción diagnóstica IPC de 003

## Revisión de implementación asíncrona y caché — 2026-10-23

Habilidad local lenteuv-architecture-review, sin delegación. Base funcional 9b1d7e8;
instrumentación guardada en commits posteriores y diff de implementación separado.
Leídos AGENTS/workflow/contexto/desarrollo y los documentos 003. Inspeccionados
App/LoadImage/Initialize/RenderFrame, ImageLoader/colas/entrega/cierre, servicio de
escaneo, FolderDataManager/sesión/orden, tira/Waterfall y caché SQLite/Compression.
Revisados CMake, pruebas nuevas y ejecutor de rendimiento. No es una auditoría nueva
de Board, codecs, escritores de metadatos, renderer completo ni repositorio.

- **R003-01, riesgo alto, atendido:** una apertura vieja o un snapshot anterior
  podía sustituir selección/navegación tras una nueva interacción. Generaciones,
  leases, comprobación antes de upload y entrega UI; cancelación también retira
  resultados CPU grandes ya pendientes. La ordenación cambiada durante un scan
  se repite en el trabajador. Los borrados conocidos se filtran por época de mutación.
  Evidencia de código y regresiones de latest-wins, cambio de orden y borrado.
- **R003-02, riesgo alto, atendido:** guardar durante restauración podría perder
  carpetas no publicadas. Descriptores pendientes y orden de sesión preservados
  al guardar; el worker no modifica modelos, UI ni archivos personales. Tests
  verifican guardado temprano. Cancelación/joins conservan dependencias hasta retornar
  la llamada WIC/E/S activa, sin plazo universal ni hilos detached.
- **R003-03, riesgo alto, atendido:** una caché solo por ruta o una descompresión
  confiada podría devolver contenido viejo o tamaños no válidos. Clave con tamaño,
  mtime, identidad/ChangeTime y variante/pipeline, firma antes/después, dimensiones
  y longitud limitadas y checksum de píxeles. Cache miss/fallo usa WIC. Pruebas de
  igualdad exacta, variantes, Unicode, modificación, reemplazo y corrupción; un
  proceso propio adicional lee la misma caché. No se usa hash completo del original
  para lookup ni se añade un observador universal de cambios externos a texturas ya listas.
- **R003-04, tradeoff explícito:** tres workers de miniaturas dejan capacidad a la
  imagen principal; pueden reducir throughput frío frente a cuatro. Lectura SQLite
  usa presupuesto de 500 ms para admisión inicial y 10 ms con la base preparada;
  no espera locks de SQLite de otro proceso (el scheduler puede demorar el despertar).
  si no accede, decodifica desde el original. Writer separado, cola de hasta 128
  entradas/8 MiB de píxeles, compresión sin pérdidas y escrituras prescindibles.
  SQLite sin WAL, auto-vacuum, reserva de journal, cap de páginas y expulsión por
  transacciones pequeñas. El mantenimiento de una reducción grande de cuota es
  incremental, hasta 32 entradas por escritura. No se promete reducir CPU/E/S/RAM
  y latencia simultáneamente. Modelo y texturas activas conservan su retención previa;
  el presupuesto de disco no es un presupuesto global RAM/VRAM.

Los comentarios de contratos se concentran en declaraciones. GPU/callbacks siguen
en UI; servicios CPU no toman direcciones de previews. La preparación del destino
no cambia la animación ni deriva la variante 100 desde la 200. Waterfall invalida
geometría por lote y mide readiness tras dos frames con la misma revisión.

Evidencia automatizada: Release/CTest 8/8. El smoke nativo anterior pasó diez
escenarios; se repite sobre la versión final, incluido IPC con receptor ya arrancado.
El harness de cierre ahora drena mensajes como App::Run: no interpreta un Peek filtrado
inmediato como prueba suficiente de WM_QUIT. Es adaptación de validación, sin cambio
del protocolo IPC ni investigación del reporte histórico cerrado.

Medición/entrega en curso. No certificar UX completa, primera sesión nueva ni
cumplimiento p95 <=100 ms por compilar o por el retorno de Present. DXGI correlaciona
un ID de frame; la observación produce una cota superior, no una medición óptica.
La evidencia anterior y sus límites permanecen abajo como historial.

**Límite adicional de ejecución:** apareció un fallo intermitente en el par de
clientes de ipc.delivery_diagnostics; una repetición aislada pasó. Se conserva
como IPC-003-CONCURRENT-01 en known-issues, sin causa ni atribución al diff nuevo.
No se modificó el transporte y no se investiga el antiguo reporte manual cerrado.
Se añadió salida de códigos/recuentos antes del assert para no perder evidencia
si vuelve a ocurrir. Los checks siguen siendo estrictos; no se oculta el fallo.

**R003-05, diagnóstico, corregido:** la serie de tira mostró que el lerp float en
posiciones lejanas puede estabilizarse a una distancia representable >0,1 px del
destino. El antiguo criterio de readiness nunca cerraba esos casos aunque el área
visible ya no se moviera. Se reconoce también una posición idéntica en dos updates
consecutivos, sin alterar lerp, snap, efectos ni cadencia. La base instrumentada
recibe el mismo criterio y una regresión reproduce el plateau a 144 Hz. Se conserva
la serie anterior como exploratoria y se repite la matriz. Release/CTest 8/8
posteriores correctos (89 checks de startup/cache, 17,96 s).

La revisión vigente añade dos correcciones finales (validación y serie final
registradas en result.md):

- **R003-06, rendimiento, atendido:** la serie instrumentada encontró hits
  acompañados de muchas escrituras de recencia y decodificaciones adicionales.
  El UPDATE síncrono por hit mantenía el mutex SQLite durante journal/commit.
  La recencia pasa al writer en lotes acotados de 128 registros con ventana de
  250 ms; un nuevo write drena la recencia antes de expulsar por LRU. Se preserva
  el tiempo de acceso, se descartan pendientes al cerrar y se omite recencia
  bajo 1 GiB libre. La mejora efectiva requiere contraste de la serie final;
  el diseño por sí solo no demuestra su magnitud.
- **R003-07, robustez, atendido:** COMMIT puede fallar ante un lector de otra
  conexión después de un INSERT correcto. Se hace ROLLBACK explícito para no
  conservar una transacción que impida los siguientes BEGIN. La regresión mantiene
  un read lock independiente, comprueba ausencia de bytes no confirmados y
  recuperación posterior. No se relajan los checks de caché ni de IPC.
- **R003-08, rendimiento de cola, atendido:** las trazas de cc5db31 detectaron
  aperturas SQLite de unos 300 ms que hacían expirar las lecturas concurrentes y
  disparaban regeneraciones. El writer prepara la base durante la inicialización
  del renderer; las miniaturas coordinan esa admisión inicial con presupuesto de
  500 ms. La imagen principal conserva su worker y no espera SQLite. La apertura
  del sistema de archivos ya iniciada debe retornar; no se promete interrupción
  forzada. La nueva serie mide ese trabajo y no atribuye al nuevo EXE las cifras
  de cc5db31. La política posterior de contención conserva presupuesto de 10 ms.
- **Reserva de capacidad:** el único llamador productivo restante de LoadAsync
  con tamaño limitado es Board (400px). También participa en el techo de tres
  miniaturas, sin añadir una variante persistente nueva. La prueba de gating usa
  esa admisión real antes de arrancar los cuatro workers y verifica capacidad
  disponible para foreground. No se cambia la calidad, retención ni formato Board.

Última validación de fuentes: Release y CTest 8/8, 18,27 s, 94 checks startup/cache.

## Revisión histórica

Seguimiento: [006](../006-optimizaciones-locales/review.md) implementa la primera
porción local, con revisión/equivalencia/medición propias. La auditoría histórica de
abajo no es una afirmación de que ninguna oportunidad se haya implementado después.

**Prioridad actual por instrucción del propietario:** [revisión de optimización por
fuentes](source-optimization-review.md). Siete oportunidades OPT con evidencia,
coste, restricciones de equivalencia y validación propuesta. No son siete defectos
reproducidos ni cambios implementados. No nuevos tests/runtime en esa auditoría.

La porción anterior [Explorer/reposo sin logging](explorer-validation.md) sí se
ejecutó: elección única con ayuda manual, JPEG→PNG en PID propio, UserChoice/comando
JPG/PNG conservados y base CPU/memoria/GPU por PID. No hubo código de salida recuperable
del receptor; ausencia de proceso no se convirtió en 0. Portable asociado con hash
distinto al build validado: dato de entorno, no defecto ni prueba de revisión antigua.
Cobertura de esa porción: main/ParseCommandLine/Initialize/OnIpcOpenPath y raíz de
datos; no cambios de contratos. Error del helper documentado, sin fallback de UI
nativo alternativo. Primera sesión solo tiene protocolo preparado.

Seguimiento actual: la [validación real posterior a 005](real-validation.md) amplía
evidencia de presentación, flags, diálogo/salida y reposo CPU/memoria. El informe
histórico se conserva; revisión de esta ampliación al final.

Fecha: 2026-10-22. Base ac4a5da más diff R1/R2/004 preservado, misma rama, sin commit.
Seguimiento posterior: AR-003 se corrige en el alcance de
[005](../005-entrega-ipc/review.md); este informe conserva el diagnóstico histórico.
Se aplicó lenteuv-architecture-review en esta sesión, sin delegación. Alcance autorizado:
observabilidad y diagnóstico; no cambio del protocolo ni corrección funcional.

## Cobertura

Lectura completa de single_instance_ipc.h/.cpp y tests/ipc_diagnostic_tests.cpp/script;
entrada main.cpp, tratamiento AlreadyRunning, cuerpo de Initialize y callback IPC de App,
receptor Window::HandleMessage, LoadImage, requisitos D2D y CMake inspeccionados.
Se comprobó la vida de HWND, procesos hijos y servidor en el harness y la propiedad de
los mensajes al entregarlos por la cola Win32 real. Los contadores atómicos no contienen
rutas; snapshot no transaccional y no garantiza ACK. GetLastError de ConnectNamedPipe
se captura antes de publicar la etapa diagnóstica siguiente.

Cuerpo extraído de OnIpcOpenPath y endpoint predeterminado comparados con HEAD: iguales
omitiendo indentación. Reintentos y decisiones de Initialize/SendMessage/Shutdown no
se cambiaron. Los nuevos/modificados métodos y estructuras tienen responsabilidad
comentada. Las garantías falsas de cierre se corrigieron sin alterar su mecanismo.

No revisado de nuevo: UI completa, todos los consumidores/render/loader, costes de
arranque y reposo, mutex/elección simultánea, cancelación de IO ni formatos/persistencia.

## AR-003 — Actualización con reproducción

- Ubicación: SingleInstanceIpc::Initialize (core/single_instance_ipc.cpp:36),
  SendMessage (:128), ServerThreadProc (:180); App: tratamiento del bool en Initialize;
  main.cpp: AlreadyRunning. Líneas del árbol de esta porción, no de auditoría original.
- Evidencia: código y ejecución. Cinco procesos clientes secuenciales y dos simultáneos
  detectan receptor pero fallan en envío con ERROR_FILE_NOT_FOUND. Controles directos
  aplican JPEG/PNG en el handler real, con bitmap disponible. Hay conexiones vacías en
  los logs del servidor. El bool Initialize no distingue detección de envío fallido.
- Clasificación/prioridad/confianza: defecto de contrato alto; reproducción confirmada
  en entorno aislado instrumentado. No se cuantifica su frecuencia en uso real.
- Consecuencia: el llamador puede tratar la petición como derivada aunque no hubo envío;
  no llega al handler ni cambia la imagen. La escritura/posteo tampoco confirman aplicación.
- Propuesta mínima: especificar corrección del intervalo entre sondeo y reenvío y del
  resultado de error. Comparar reutilizar conexión con reintento acotado tras detección;
  mantener fallo observable si no hay entrega. Resolver propiedad ante post fallido.
  ACK y elección atómica tienen alcance/aceptación adicionales, no se presuponen.
- Coste: medio, acotado inicialmente a cliente/resultados/propiedad y regresiones;
  mayor si se redefine elección, ACK o cierre.
- Validación: transformar esta matriz en regresión que exija aplicación válida, error
  explícito e inexistencia de mensajes duplicados; completar presentación/arranque real.
- Relación: siguiente corrección propia sugerida en roadmap, no implementada por 003.

Sin hallazgos nuevos bloqueantes de la instrumentación. Persiste el riesgo previo
AR-003 del payload si PostMessage falla y AR-005 de cierre, expresamente fuera de la
porción; no se provocaron ni se consideran corregidos. El endpoint global no se contacta
por las pruebas. Hijos propios tienen timeout/recolección, incluidos caminos de excepción.

## Evidencia y límites

Release/CTest 4/4 y comprobación IPC final pasaron. Un diagnóstico que reproduce fallos
puede pasar sus invariantes sin que el producto esté corregido. Véase result.md para
matriz, hashes y comandos. No hay imagen presentada en swapchain, primera ejecución,
perfil de reposo ni mediciones de coste de la instrumentación. 003 permanece abierto.

## Ampliación: validación real posterior a 005 — 2026-10-22

Sin fuentes modificadas, misma base y hash de producto 005. Leídos Git, AGENTS,
workflow, project-context, roadmap, documentos 005, habilidad/criterios y guías
computer-use. Inspeccionados App::Initialize/OnIpcOpenPath/LoadImage/OnResize,
AdjustWindowToImage, Renderer::Resize/BeginFrame/EndFrame, entrada main y settings
de prueba. Inventario acotado; no nueva auditoría global ni cierre/persistencia.

Ejecución: producto real con renderer/swapchain, PNG presentado tras IPC, ventanas
independientes --multiple/--lite, diálogo Win32 231 y salida 2 de wWinMain. Reposo:
tres ventanas de 10 s CPU/memoria con diagnóstico activo. Datos/hashes/conteos,
serie descartada y límites en real-validation.md. No se recompiló ni repitió CTest,
pues no cambiaron fuentes. Capturas antes de activar que mostraban oclusión no se
aceptan como evidencia de la imagen de LenteUV. Limpieza propia confirmada fuera
del sandbox; su listado de procesos no tenía visibilidad de los procesos externos.

### AR-008 — Capturas iniciales negras de --lite

**Seguimiento actual:** [reproducción aislada](ar008-reproduction.md). La ficha de
indicio que sigue se conserva como histórica. Diagnóstico acotado completado:
capturas negras en los tres tipos de arranque, JPEG sin resize/F11 en los seis;
una captura recogió Codex ocluyente y el usuario informó paisaje estable durante
el último contraste, tras una captura negra. Otra captura volvió a mostrar JPEG.
Hay evidencia de limitación de captura; no de fallo persistente exclusivo de --lite.
No se ha localizado causa en el renderer ni corresponde corregirlo por esta evidencia.

Se comprobó Git y se preservó el diff. Cobertura nueva: parser de flags (:46–47),
condición IPC (:116), Run/RenderFrame (:253/:296), mensajes Window WM_PAINT/WM_SIZE,
Renderer::EndFrame (:250). Los dos flags comparten config.multiple; no otro modo de
dibujo dedicado. Seis salidas 0, logs de inicialización/cierre y limpieza final sin
procesos/pipe. Hash de producto igual a 005. Sin build/CTest nuevos ni fuentes editadas.
La ausencia de ERROR en logs no certifica HRESULT de todos los frames; tampoco la
respuesta del usuario constituye vídeo sincronizado. Límite: no se descartan todos
los transitorios o configuraciones. Sin hallazgos nuevos confirmados de código.
AR-008 queda como discrepancia de captura constatada / defecto persistente no
reproducido en esta campaña; reabrir con negro/parpadeo nativo. 003 sigue abierto.

- Ubicación: escenario de arranque con --lite. Caminos inspeccionados, sin causa
  localizada: App::Initialize (app.cpp:177 mostrar ventana, :201 carga inicial),
  LoadImage (:571), OnResize (:882); Renderer::Resize (renderer.cpp:212) y
  EndFrame (:250). Estas referencias delimitan investigación, no prueban un defecto.
- Evidencia: ejecución/capturas. Con JPEG y renderer inicializados, dos capturas
  tras activar lite muestran cliente negro; F11 muestra JPEG y otro F11 lo conserva
  en ventana. Multiple y receptor PNG sí produjeron capturas con imagen.
- Clasificación: riesgo de presentación por investigar; prioridad media porque
  una imagen inicial invisible afecta apertura, pero no bloquea corrección IPC 005.
- Confianza: indicio; captura/oclusión y producto aún no diferenciados. No se afirma
  defecto confirmado de render, de --lite ni regresión introducida por 005.
- Consecuencia posible: usuario podría necesitar un cambio de tamaño para ver JPEG;
  falta reproducción controlada y observación nativa para confirmar ese efecto.
- Propuesta mínima: repetir arranque secuencial y aislado con ambos flags y sin flag,
  comparar captura activada con observación del usuario y probar redraw/resize.
  Investigar errores D2D/DXGI solo después; no refactorizar renderer por el indicio.
- Coste estimado: pequeño para reproducir, corrección indeterminada según causa.
- Compatibilidad/validación: copias y settings propios; preservar gestos, modos y
  datos. No usar título/log como prueba de presentación; no cambiar IPC/ACK/cierre.
- Relación: seguimiento de AC-03 de 003, fuera del contrato de transporte de 005.

No hallazgos nuevos confirmados de código en este alcance. Quedan sin certificar
Explorer/asociaciones, presentación inicial lite, primera real/posteriores medidos,
GPU/VRAM, reposo sin logging y rendimiento. El usuario confirmó uso previo de la
aplicación en esta sesión; no fabricar primera ejecución reiniciando/cerrando sesión.
