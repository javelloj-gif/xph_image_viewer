# Revisión de 003: arranque asíncrono y caché

**Revisión de cierre, 2026-10-24:** revisión local del último diff de App::Initialize,
FolderDataManager::LoadSession, declaraciones/contratos, fases de Renderer y tests
de arranque/sesión. CPU worker sin publicación de modelos; callbacks/UI/GPU siguen
en principal tras renderer listo; partial failure y teardown cubiertos por el harness.
Sin defecto bloqueante confirmado en ese alcance. Portable con imports comprobados
y smoke correcto. No equivale a revisión independiente ni auditoría universal.
Rendimiento e IPC intermitente conservan sus límites aceptados en [closure.md](closure.md).

## Registro anterior al cierre (histórico)

## Resultado de la captura ETW — 2026-10-24

Continuación de la habilidad lenteuv-architecture-review, sin delegación ni cambios
de fuentes. Alcance: evidencia del arranque y propuestas ya revisadas, no nueva
auditoría de producto. Revisados ETL/QPC, procesos/hilos/imágenes, CSwitch/ReadyThread,
stacks con símbolos públicos, faults/disco y reporte oficial de Defender. Auditoría
FileIO del ETL completo para el visor sobre la carpeta de solo lectura indicada.
[Resultado y procedimiento reproducible](etw-result.md).

Una sola apertura del candidato/hash previsto en otro boot: original enviado
2215,70 ms, DXGI observado 2237,34 ms; salida 0, cero pérdidas QPC/ETW y hashes de
settings/sesión iguales. La auditoría no observa FileIoWrite/Delete/Rename/SetInfo/
SetEA/SetSecurity del visor sobre la carpeta multimedia. No se tocaron sus archivos
para comparar hashes ni se inició otra instancia para el análisis.

- **R003-09, diagnóstico de intervalo previo al marcador: atribución resuelta
  para esta captura, sin extrapolar al historial.** El hilo creador 13196 del padre
  permanece fuera de CPU 1426,244 ms en Waiting/Executive. Stack del comienzo en
  CreateProcessInternalW/NtCreateUserProcess/PspCallProcessNotifyRoutines/WdFilter/
  FltSendMessage. MsMpEng (4688/12740) lo reactiva mediante ReadyThread, antes de
  crear el hilo principal. Get-MpPerformanceReport mide FileScan del EXE durante
  1407,5852 ms. No se atribuye la espera a App::Initialize ni al trabajo JIT de
  otro hilo de PowerShell observado en paralelo. No se usa hit-percent de un
  butterfly combinado como porcentaje de latencia o CPU del visor.
- **R003-12, cuello de botella externo de prioridad alta, confirmado en esta
  ejecución:** análisis síncrono de Defender en creación del proceso. EXE sin
  firma y eventos de comprobación de confianza/firma; hay un disparador de nube,
  sin duración de red aislada. Propuesta: evaluar distribución firmada con identidad
  estable o preparar un reporte técnico si persiste, sin garantizar ahorro por
  firma y sin confundir Defender Antivirus con SmartScreen. Coste de certificado,
  proceso de publicación y nuevas mediciones en sesiones disponibles. No se firma,
  envía el EXE ni cambia seguridad por esta porción; esas decisiones siguen pendientes.

R003-10/11 siguen como oportunidades internas sin implementar. En esta captura
hay 260 ms de renderer, unos 44 ms de scan de restauración cancelado y 204 ms de
scan necesario antes de solicitar el original. Conservar generaciones/orden/datos
al proponer solapamiento; no prometer 100 ms tras retirar una espera externa.
La preparación ETW y el análisis pedidos están completados; el rendimiento inicial
continúa rechazado y 003 permanece abierta. No hubo build/CTest/candidato nuevos.

## Preparación de captura ETW — 2026-10-24

Alcance autorizado posterior: preparar opción 1, entrada explícita a la carpeta
indicada por el propietario, sin modificar multimedia. Revisados los scripts
run_startup_etw.ps1 y startup_etw_collector.ps1, perfiles WPR, token/UAC,
handshake, exclusión de reintentos por boot, rutas de salida, cierre y evidencia.
WPT oficial instalado; fuentes del producto/EXE permanecen iguales. Estado y
comandos en [etw-capture-protocol.md](etw-capture-protocol.md).

La captura elevada solo pertenece al helper; el padre rechaza elevación para no
medir LenteUV con token administrativo. Se fija el directorio como argumento y
Viewer por preparación con configuración validada. No hay comandos de modificación
de medios ni accesos preparatorios al contenido multimedia. El helper comprueba
los keywords de CPU/waits/loader/disco/faults/FileIO antes de publicar readiness.
Un nombre único acota start/stop/cancel; no se cancela una sesión ajena.

Se preserva guard exclusivo antes del único Process.Start del producto, incluso si
este falla. Capture rechaza el mismo boot de preparación, ausencia de confirmación,
EXE/settings/sesión modificados, toolchain cambiado o scripts sin validar. Verifica
otra vez inputs y ausencia de procesos tras UAC. Hash EXE después de medir para
evitar lectura anticipada. Watchdog del helper guarda evidencia ante pérdida del
padre; el producto solo recibe cierre normal, sin kill/reintento automático.

Evidencia: validaciones del recolector real sin producto, ETL guardado y pérdidas
cero; Xperf extrajo procesos/imágenes y stacks. Negativas de Capture sin confirmación
y con confirmación en el boot consumido verificadas; no llegaron a UAC ni a abrir
el visor. Parseo de ambos scripts y diff/enlaces comprobados. Ninguna nueva
ejecución del candidato ni test sobre la carpeta multimedia.

Límites: el tramo Capture→producto/QPC todavía no se ejecuta porque requiere otra
primera oportunidad real; la validación sin producto no lo convierte en medición.
No hay PDB Release coincidente disponible; conservar módulos/RVA y símbolos públicos
del SO, sin usar el Debug distinto. Presencia de eventos antimalware no acredita
causalidad por sí sola. La traza será global y permanecerá local bajo build.
No nueva causa ni optimización de producto declarada; R003-09 continúa pendiente
del análisis de la futura captura. Sin cambios de protocolo IPC o base/AR-005.

## Revisión acotada de primera sesión — 2026-10-24

Habilidad lenteuv-architecture-review aplicada sin delegación. Git 76dc06c,
fuentes del EXE b0e98f8 y cambios documentales previos preservados. Leídos
AGENTS/workflow/contexto/desarrollo y documentación 003, criterios/formato de la
habilidad. Revisión por camino de arranque: main/PreInitDebugIpc/PerformanceTrace,
App::Initialize/RenderFrame/BeginViewerLoad, restauración/selección de FolderDataManager,
FolderScanService::Scan, loader CPU/upload, apertura de ThumbnailCache y renderer
D3D/D2D/DirectWrite/swapchain/composición. CMake e imports PE inspeccionados con dumpbin.
Sin nueva auditoría de metadatos, Board, codecs completos ni IPC/protocolo.

Ejecución: una única apertura del candidato en la sesión recién reiniciada
reportada por el propietario. Sin calentamiento/repetición. Original enviado
1932 ms, confirmación DXGI observada 1956 ms; salida 0, settings/sesión intactos,
hash EXE conservado y 0 eventos descartados. Evidencia, límites y opciones en
[first-session-analysis.md](first-session-analysis.md). Sin cambios productivos,
build ni CTest nuevos. El propietario confirma seis áreas funcionales y rechaza
los aproximadamente 3000 ms iniciales; 003 sigue abierta por rendimiento inicial.

- **R003-09, diagnóstico de prioridad alta, pendiente de atribución:** main.cpp:48
  marca main.entry dentro de wWinMainImpl; PreInitDebugIpc está antes (:118) y
  PerformanceTrace::Get/reserve se ejecuta al registrar el primer evento
  (performance_trace.h:17/:76). Medidos 1325 ms hasta ese marcador, de ellos unos
  1298 ms después de creación Win32. Sin ETW de loader/CPU/waits/disco/seguridad,
  no se puede atribuir a DLL, antivirus, driver ni CRT individualmente. Imports
  ordinarios confirmados, sin delay directory. Consecuencia: optimizar solo App
  no atiende el intervalo dominante. Propuesta de coste medio: captura elevada
  previa a otra primera sesión disponible, correlación de image loads/stacks/faults
  y, si corresponde, analizador Defender. No reiniciar/repetir ahora; sin exclusiones
  de seguridad. Diferir imports/servicios es alternativa condicionada a ese perfil,
  con fallos de carga tardíos y demora de primera miniatura como compromisos.
- **R003-10, oportunidad de rendimiento de prioridad alta, confirmada en código
  y en esta ejecución, sin corrección aplicada:** app.cpp:212/:218 inicializa
  renderer antes de LoadSession (:240); folder_data_manager.cpp:80/:205 solicita
  original tras aplicar catálogo. Renderer 208 ms y primer catálogo 224 ms se
  ejecutan en serie al restaurar sin argumentos. Propuesta de coste medio:
  anticipar el scan CPU y diferir publicación a UI lista. Solapamiento podría
  evitar del orden de 200 ms si no introduce contención; no es ahorro medido.
  Preservar prioridad de ruta explícita, generaciones, selección, sesión pendiente,
  reentrada y cierre; validar todos esos casos antes de otro candidato.
- **R003-11, oportunidad de rendimiento de prioridad media, medida sin causa
  interna completa:** folder_scan_service.cpp:44/:47/:52 incluye enumeración y
  orden. De 224 ms, solo 3 ms son el span de orden final/keys. Hay además unos
  35 ms desde snapshot hasta image.request, sin desglose de UI/planificación.
  Propuesta de coste medio: perfilar E/S/atributos y publicación antes de cambiar
  enumerador/construcción de listas. Conservar sort/ties, Unicode, enlaces,
  cancelación y cambios del directorio. No presentar otra optimización del
  comparator como solución al segundo dominante previo al marcador.

Sin defecto nuevo confirmado en thumbnails o driver. Apertura SQLite 13 ms
solapada; demandas de tira posteriores al original. Renderer 208 ms sin desglose
por llamada (renderer.cpp:49/:103/:150/:167); instrumentar esas llamadas es
diagnóstico posterior, no justificación para sustituir backend/transparencia.
Las recomendaciones son propuestas; las tres quedan pendientes, sin autorización
inferida para implementarlas ni cerrar el rendimiento por aceptación histórica.

## Actualización manual anterior — 2026-10-24 (sustituida en rendimiento inicial)

El [reporte del propietario](user-validation.md) acepta el rendimiento percibido:
carga inicial de unos 3 s tras iniciar el SO y mejora considerable posterior,
también en Waterfall. No confirma la matriz funcional completa, una serie comparable
de primera sesión ni la disposición de IPC-003-CONCURRENT-01. 003 sigue abierta.
La meta de 100 ms permanece sin acreditar. Actualización documental sin fuentes,
nueva auditoría, compilación ni repetición de pruebas; la revisión inferior conserva
su fecha y alcance histórico.

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

Evidencia automatizada final: Release/CTest 8/8 (18,27 s, 94 checks startup/cache)
y diez escenarios nativos sobre b0e98f8, incluido IPC con receptor ya arrancado.
El harness de cierre ahora drena mensajes como App::Run: no interpreta un Peek filtrado
inmediato como prueba suficiente de WM_QUIT. Es adaptación de validación, sin cambio
del protocolo IPC ni investigación del reporte histórico cerrado.

Medición/portable entregados: [resultado](result.md) y [series](performance-result.md).
La aceptación funcional permanece pendiente. No certificar UX completa, primera sesión nueva ni
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
  bajo 1 GiB libre. La serie final confirma el beneficio preparado; la
  generación inicial conserva un coste mayor que la base, documentado en las tablas.
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

## Cierre de esta revisión — 2026-10-24

No hubo cambios productivos después de b0e98f8. Se contrastaron hashes de EXE,
configuración y originales entre versiones/condiciones. El portable contiene el
mismo EXE medido; imports/runtime/locales/defaults comprobados. Un smoke normal sin
diagnóstico en otra copia respondió a entradas de modos y cerró con salida 0.

Las series tienen n=30 por posición y escenario/condición, y 120 observaciones
comparables de reutilización. El intento no preparado de base C queda registrado;
se repitió con 60 s y no se mezcló con el resto como caché caliente.

**Tradeoff que requiere disposición del propietario:** el original no acredita
100 ms; Waterfall vacío tiene p95 1,22–1,95 s frente a unos 0,68 s de base, aunque
preparado baje a 67–72 ms. Las tablas registran el coste CPU/E/S/memoria/GPU inicial.
No se declara optimización uniforme ni cierre funcional sin la aceptación pendiente.
La revisión no certifica todos los gestos, formatos opcionales ni escritores de
metadatos/Board; esos límites no se convierten en una campaña nueva por iniciativa
propia. La base/AR-005 y el reporte manual IPC-014-01 siguen cerrados.
