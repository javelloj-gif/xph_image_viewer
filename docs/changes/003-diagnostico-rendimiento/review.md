# Revisión de la porción diagnóstica IPC de 003

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
