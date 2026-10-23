# Plan: 005 Entrega IPC

Especificación: [spec.md](spec.md). Revisión: [review.md](review.md).
Base ac4a5da, rama codex/repository-foundation; diff R1/R2/003/004 preservado.

- [x] Leer Git, habilidad, guías y documentos 003; seguir IPC/App/main/Window/tests.
- [x] Definir antes de editar contrato: reintento solo al abrir, tres resultados,
  error de envío con salida 2 y sin fallback, payload RAII ante fallo de post.
- [x] Implementar IPC y llamadores con comentarios de responsabilidad/propiedad.
- [x] Convertir matriz a regresiones y añadir ausencia temporal, saturación y fallo de post.
- [x] Configure/build Release/CTest, revisión del diff, guías y resultado final.

El servidor posee handles y payload hasta PostMessage; Window destruye lo entregado
en hilo UI. El cliente posee conexión hasta cerrar y no reenvía tras escribir.
Usar pipes privadas, copias y user_data aislado del harness existente. Fallos de
tests se corrigen sin debilitar aplicación exacta y errores explícitos.

Estado final: completado en el alcance especificado. Release y CTest 4/4 correctos;
sin criterios necesarios fallidos. [Resultado](result.md) contiene matriz, hashes y
límites. Se conserva nombre histórico ipc.delivery_diagnostics, ahora con exigencias
de regresión. App::Initialize sí se ejecuta para fallo ocupado antes de crear renderer;
el receptor de imágenes continúa siendo Window/App con WARP sin ventana visible.

Siguiente: coordinar validación real de presentación/Explorer y arranque completo
--multiple/--lite; continuar primera ejecución real/reposo de 003. No ampliar por ello
a elección atómica, ACK o cierre. Library espera valor 4 y placeholder. Sin commits.

Seguimiento de validación: [003/real-validation.md](../003-diagnostico-rendimiento/real-validation.md)
añade PNG presentado tras IPC, flags completos y diálogo/salida 2 reales, con el mismo
EXE. [AR-008](../003-diagnostico-rendimiento/ar008-reproduction.md) se contrastó después:
discrepancia de captura constatada, sin fallo persistente de --lite reproducido ni
cambio de fuentes/contrato de 005. 003 conserva sus otros pendientes.
