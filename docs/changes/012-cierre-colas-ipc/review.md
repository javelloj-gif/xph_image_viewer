# Revisión 012 — primera porción AR-005

Fecha: 2026-10-23. lenteuv-architecture-review, sin delegación. Rama
codex/repository-foundation / ac4a5da más todo el trabajo local conservado.
Diff específico contra build/shutdown-012/before; HEAD no representa producto 011.

## Cobertura

ImageLoader Initialize (:48), Shutdown (:70), WorkerLoop (:96), EnqueueTask (:114),
LoadAsync/LoadAnimatedAsync y entrega pendiente; declaraciones y ownership.
SingleInstanceIpc Initialize (:36), Shutdown (:93), CompleteServerIo (:174),
ServerThreadProc (:187), SendMessage y diagnósticos; Window WM_CLOSE/WM_DESTROY y
payload. App destructor (:22), callback onCloseRequest (:158), Run (:269), orden
de miembros y fallos de Initialize. CMake, shutdown_tests, dispatch async y ampliación
IPC/startup tests. Inventario de revisiones 004–011 actualizado por documento propio.

Sin revisión global de scanners, Board, metadatos, cancelación por consumidor,
compositor, elecciones IPC ni sincronía del cliente. No certificar cierre universal.

## Hallazgos tratados

- CL-001 — Defecto/riesgo alto, código y ejecución confirmados: cola aceptaba trabajo
  durante stop y se drenaba. Baseline con worker retenido ejecutó 1001 tareas en cola;
  candidata 0, activo 1. Parada y admisión comparten mutex, workers ya no toman tareas
  tras stop. Resultados de activos se liberan después de join, nunca callbacks en cierre.
  Cambio necesario para AC-02; coste pequeño. WIC en curso sigue sin interrupción.
- CL-002 — Defecto alto reproducido en proceso aislado: cliente conectado sin datos
  mantiene ReadFile bloqueante y centinela no puede entrar. Dos escenarios baseline
  agotaron watchdog de 5 s; candidata termina. Evento stop y OVERLAPPED permiten
  cancelar conexión/lectura sin depender del cliente. Se espera GetOverlappedResult
  antes de liberar estructura/buffer/handle; no detach/TerminateThread en producto.
  Necesario AC-03; coste medio. No equivale a plazo absoluto de todos los drivers.
- CL-003 — Riesgo alto de propiedad, código y prueba: payload ya posteado permanecía
  sin consumir al cerrar; baseline falla check. Shutdown drena solo mensajes IPC de
  su HWND tras unir servidor. WM_CLOSE destruye ventana antes de WM_QUIT: App detiene
  IPC en callback de cierre terminal antes de DestroyWindow, y en destructor antes
  de destruir consumidores ante fallo parcial. Ocultar a bandeja no para servicios.
  Necesario AC-03/04, coste medio. Contrato HWND vivo/hilo dueño documentado.
- CL-004 — Riesgo alto identificado al revisar el nuevo drenaje, corregido antes de
  entrega: PeekMessage puede devolver WM_QUIT pese al filtro. No interpretar wParam
  como puntero; conservar y repostear código de salida. Test con código 42 y payload
  simultáneo pasa. El HWND se toma y limpia antes de drenar para no retenerlo en cierres
  posteriores. Necesario AC-03; no se afirma fallo previo del producto por este camino nuevo.

## Contratos y límites

Métodos/clases nuevos o modificados documentan hilo, propiedad y efectos. Lifecycle
serializado en UI, sin Shutdown concurrente/reentrante ni llamadas desde workers.
Enqueue puede competir con stop, siempre decide bajo el mismo mutex. Capturas y
pendientes se destruyen fuera de mutex; GPU pendiente se libera en hilo dueño.
Factory WIC se libera tras joins. Reinicio del loader exige reconstruir modelos cuyos
tokens quedaron sin completion; App solo usa cierre terminal. Callbacks normales y
predicado UI 011 no cambian; no se evalúa estado de página en workers.

IPC mantiene protocolo/UTF-8/read único/tamaño, sondeo, retries y resultados 005.
El centinela histórico aún se acepta, pero Shutdown no lo envía. Dos nuevos eventos
son privados, creados solo en primario y cerrados tras join. Fallo de cualquiera
retorna ForwardFailed/error Win32, igual que otros fallos de preparación IPC; no
inicia servidor y libera el primer evento si falla el segundo. Pruebas inducidas
en IAT del EXE de tests, sin alterar DLL ni agregar hooks productivos.

Referencias técnicas utilizadas: [servidor overlapped de Microsoft](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-server-using-overlapped-i-o)
y [CancelIoEx](https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex).
La solicitud de cancelación no basta para liberar su almacenamiento: se observa
la finalización, incluso si la operación gana la carrera y acaba normalmente.

## Dictamen

Sin hallazgos accionables pendientes en la primera porción especificada. Release y
CTest 6/6; nueve escenarios de cierre en procesos propios (uno hace doce ciclos),
162022 checks async, 294 decoder y diez escenarios nativos. Smoke ampliado confirma
WM_CLOSE y hide-to-tray, no aceptación visual exhaustiva. No medición global de fugas,
VRAM, latencia de apertura ni cierre bajo cualquier codec. AR-005 sigue abierto para
cancelación por consumidor y límites WIC/scanners/cliente síncrono; AR-006/007 siguen
con su alcance propio. No ACK, elección atómica o retirada de Library.
