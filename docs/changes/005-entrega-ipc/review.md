# Revisión de 005: AR-003

Fecha: 2026-10-22. Habilidad lenteuv-architecture-review, sin delegación.
Git: ac4a5da / codex/repository-foundation, diff previo preservado y sin commits.

Seguimiento posterior: [003/real-validation.md](../003-diagnostico-rendimiento/real-validation.md)
amplía ejecución del producto real y [003/review.md](../003-diagnostico-rendimiento/review.md)
registra AR-008 como indicio visual, sin atribución causal ni cambios de fuentes.
El inventario siguiente es el de la revisión de implementación de 005.

## Cobertura

Leídos AGENTS, workflow, project-context, roadmap, spec/plan/result/review de 003,
criterios/formato de la habilidad y documentos 005. Inspeccionados completos
single_instance_ipc.h/.cpp, harness IPC y script; llamadores App::Initialize,
GetIpcSendError, main, handler Window y configuración CMake de tests. Se revisaron
el cambio del contrato, consumidores, propiedad y etapas de la matriz.
No es auditoría completa de App, loader, páginas o persistencia.

## AR-003 — Corrección y aceptación acotada

- Ubicación verificada: single_instance_ipc.cpp:36 Initialize, :138 reintento,
  :234 payload; app.cpp:118 traducción del resultado; app.h:86 error;
  main.cpp:81 diálogo/retorno. Tests: ipc_diagnostic_tests.cpp:192 ambas rutas
  concurrentes, :204 conteos finales, :213 FailureCases y :272 App real.
- Evidencia: código y ejecución. Frente a los cinco fallos error 2 de 003, ahora
  cinco clientes aplican exactamente una imagen/bitmap; par concurrente entrega
  ambas rutas. Error ocupado retorna ForwardFailed hasta App, sin primario nuevo.
  Fallo de post inducido con HWND destruido incrementa contador sin transferir payload.
- Clasificación/prioridad/confianza del defecto original: defecto alto, confirmado;
  corregido en este alcance. No se generaliza la muestra a todo uso de IPC.
- Consecuencia corregida: se reabre durante recreación de pipe y un fallo de envío
  ya no acaba silenciosamente en AlreadyRunning exitoso. La ausencia inicial sigue
  sin elección atómica; el éxito de transporte no prueba aplicación.
- Propuesta aplicada: reintento solo al abrir, tres resultados y RAII local al post;
  coste medio por migración de llamadores/regresiones. Reutilizar conexión habría
  requerido otro camino de saturación; no se necesitó ACK ni nueva abstracción.
- Compatibilidad: nombre de pipe, UTF-8, mensaje Window, centinela y ruta vacía
  conservados. No cambios persistidos/gestos. App/main cambian el fallo conforme
  a spec previa. El helper compartido añade retry missing también al centinela.
- Validación: Release/CTest 4/4; controles, entrega diferida, rechazo de ruta,
  aplicación exacta, ausencia temporal y errores. Relación: necesario para 005.

Comentarios de métodos/clases modificados contrastados con implementación: Initialize,
SendMessage, ServerThreadProc, App::Initialize y GetIpcSendError; tests tienen comentarios
de responsabilidad. Conversión UTF-8 documentada por familia. No se promete ACK ni
plazo de IO/cierre, y el comentario de main ya no afirma que la imagen fue aplicada.

Sin hallazgos nuevos bloqueantes en el alcance. El payload de post fallido tiene
destrucción RAII comprobable y la prueba une el servidor antes de salir; no se afirmó
una medición de fugas. La propiedad tras post exitoso/cierre queda como limitación
previa, al igual que AR-005. No hubo cambios incidentales a Library o AR-006.

## No revisado ni certificado

Modal visual/exit real de main, Explorer, swapchain/presentación, arranque completo
con flags, elección entre primarios, cancelación/cierre bloqueante, rendimiento,
primera ejecución y consumo en reposo. Fallos de escritura y permisos no inducidos.
No hay garantía de entrega/aplicación general sin ACK. [result.md](result.md) registra
hashes, conteos, comandos y límites. Siguiente componente: validación real IPC/003,
con coordinación de primera ejecución y sin mezclar rediseños no autorizados.
