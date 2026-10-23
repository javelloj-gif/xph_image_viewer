# AR-008: reproducción aislada y contraste de captura

Fecha: 2026-10-22. Estado: diagnóstico acotado completado, sin corrección de fuentes.
Se reprodujo la captura negra; no un fallo persistente de --lite que requiera F11.
La observación nativa del usuario y capturas ocluidas confirman una limitación de
esta ruta de captura. No se identificó el mecanismo interno ni se descarta cualquier
transitorio de presentación en otros escenarios. [Plan](plan.md), [revisión](review.md).

## Revisión y preparación

Git comprobado: codex/repository-foundation, HEAD ac4a5da, cambios R1/R2/003/004/005
sin commit preservados. Leídos AGENTS, workflow, project-context, roadmap, documentos
003/005 y real-validation.md; habilidad lenteuv-architecture-review y computer-use.
Mismo EXE Release de 005, SHA-256
`400045F207A8775598368A5B5430BED2C33C117C27DC3BB97A722934C544A5C4`.
JPEG seed.jpg copiado de noname.jpg, SHA-256
`DDBDB9CDB5F109C567D2AAFD076288CD57187E7A97D94898C72591DBFE28235C`.

Seis directorios nuevos en build/ar008-<id>, cada uno con EXE, locales, images y
user_data propios. Packaging defaults con fullscreen=false, opacidad=1, logging y
debug_ipc=true, sesión vacía. Ningún dato personal, asociación o preferencia global.
Mismo entorno de la validación real anterior; no nueva ficha de rendimiento.

Un lanzador PowerShell fuera del sandbox verifica ausencia de LenteUV y pipe antes
de cada caso, ejecuta Start-Process Normal con --windowed --opaque y ruta JPEG;
añade --lite o --multiple según caso. Espera salida antes del siguiente, con límite
180 s y limpieza únicamente del hijo propio. Registra argumentos, PID y salida en
runs.json. La inspección de procesos se hace fuera del sandbox por su limitación de
visibilidad constatada previamente. No son arranques simultáneos ni primera real.

## Observación secuencial

sky.list_windows selecciona la única ventana del caso; get_window_state registra
captura inicial, activate_window más captura, y otra captura posterior sin resize.
«Inicial» es la primera captura del ensayo, no medición del primer frame del proceso.
Las separaciones dependen de la interacción de herramientas; no son plazos de UX.
Escape cierra cada ventana propia. No F11, resize, cambio de página ni nueva apertura.

| Caso / PID | Captura inicial | Tras activar | Captura posterior | Salida |
| --- | --- | --- | --- | --- |
| 01-lite / 19752 | JPEG | Negra | JPEG | 0 |
| 02-normal / 36352 | Negra | JPEG | JPEG | 0 |
| 03-multiple / 18712 | Negra | Negra | JPEG | 0 |
| 04-lite / 20612 | Negra | Negra | JPEG | 0 |
| 05-normal / 40496 | Negra | JPEG | JPEG | 0 |
| 06-multiple / 26880 | Texto de Codex ocluyente; inválida para LenteUV | JPEG | Negra; luego JPEG sin intervención | 0 |

En 01-lite una segunda activación también produjo JPEG sin resize. En el último
ensayo, después de la captura negra y manteniendo la misma ventana sin acciones,
el usuario respondió «Paisaje estable» al contrastar directamente LenteUV. La captura
siguiente volvió a mostrar JPEG. La primera respuesta del usuario («Veo el paisaje»)
llegó después de cambiar de ensayo: se conserva como reporte, no como contraste
sincronizado de 01-lite. No hubo observación nativa de negro o parpadeo reportada.

Resultado: los seis procesos produjeron alguna captura con JPEG sin F11 ni resize.
Las capturas negras aparecen en los tres modos de invocación, incluso después de
una captura con JPEG. La captura con texto ajeno demuestra contaminación por oclusión;
el contraste nativo del último caso evidencia discrepancia de captura/observación.
No afirmar que activar siempre corrige, que F11 era necesario, ni que --lite tiene
un camino de render distinto. Tampoco clasificar toda captura negra como oclusión:
el origen concreto de esos frames no quedó instrumentado.

## Contraste de código y revisión

App::ParseCommandLine (app.cpp:46–47) asigna el mismo config.multiple para ambos
flags; Initialize (:116) solo lo usa para omitir IPC. App::Run/RenderFrame (:253/:296),
Window::HandleMessage (WM_PAINT/WM_SIZE) y Renderer::EndFrame (:250) se inspeccionaron
en el camino relevante. La búsqueda no encontró handler explícito de activación/foco
en Window/App. DefWindowProc y compositor conservan sus efectos: esto no demuestra
que foco/oclusión carezcan de efecto. EndDraw/Present no tienen registro exhaustivo de
todos sus resultados; ausencia de log ERROR no prueba que todo frame se presentara.

No defecto nuevo confirmado ni corrección necesaria sustentada en este alcance.
AR-008 deja de justificar un cambio de renderer: discrepancia de captura constatada,
fallo persistente de producto no reproducido. Reabrir si hay negro/parpadeo observado
nativamente; antes de corregir, definir reproducción y aceptación en entrega propia.

## Validación y límites

Seis salidas 0 y logs de carga/inicialización/cierre completos; ambos casos normales
arrancaron servidor IPC y ambos flags lo omitieron. Comprobación final fuera del
sandbox: cero procesos LenteUV y pipe ausente. Solo capturas/logs/JSON locales bajo
build, sin versionarlos. Hash de producto conservado; sin cambios de fuentes, build
ni nueva ejecución CTest. Se mantiene evidencia previa CTest 4/4. git diff --check.

Muestra pequeña, solo JPEG estático/ventana opaca y diagnóstico activo, sin vídeo
del compositor ni observación nativa de todos los instantes. No certifica todos los
primeros frames ni todos los formatos/configuraciones. Sin mediciones ni mejoras de
rendimiento. 003 sigue abierto: Explorer/asociaciones, primera real/posteriores,
caso habitual, GPU/VRAM y reposo sin logging. Sin ACK, elección, cierre ni Library.
