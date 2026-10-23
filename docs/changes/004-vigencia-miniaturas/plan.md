# Plan: 004 Vigencia de miniaturas

Fecha: 2026-10-22. Estado: completado; contrato concretado antes de editar producción.

- [x] Revisar Git, documentos 001 y caminos activos de listas/callbacks. Conservar R1/R2.
- [x] Añadir identidad mínima de solicitud (`common/image_request.h`) al modelo
  PreviewItem y ThumbnailInfo; no persistirla. Sin contadores globales ni nuevos hilos.
- [x] Tira: SetFileList traslada entradas retenidas sin duplicar tokens; callback resuelve
  token/ruta actual mediante CompleteThumbnail. No capturar índice ni referencia a vector.
- [x] Gestor compartido: CompletePreview busca token/ruta entre carpetas actuales,
  actualiza textura/fallo/isLoading y consume token. El token viaja con PreviewItem en
  SortFolder y desaparece en erase/clear. No cambiar sort, borrado ni selección.
- [x] Waterfall/Library: emitir token y usar CompletePreview; recalcular layout/redraw
  solo si se aplicó el resultado. Preservar tamaños 200/180 y éxito nonnull previo.
- [x] Harness `tests/async_preview_tests.cpp`: fixture amigo de App/loader/páginas/tira,
  sin exponer API pública de pruebas. Target CMake con fuentes productivas salvo main,
  PCH/opciones/libs correspondientes. Instanciar App sin Initialize/ventana visible, loader WIC
  con cero workers; ejecutar tareas y entregar completados reales en orden controlado.
  Resultados nulos o ImageData sintéticos inválidos para omitir upload del renderer de App;
  bitmap D2D 1x1 con backend WARP software para comprobar éxito de la tira. Ningún stub.
  Cubrir AC-01/02/03 con mutaciones en memoria y callback duplicado. Sin borrado de archivos.
- [x] Registrar y ejecutar `ui.async_previews`, build Release completo y CTest 3/3.
  Revisar diff con lenteuv-architecture-review, git diff --check y enlaces.
- [x] Completar result/review, trazabilidad de 001, guía técnica/tests y roadmap.

Archivos: los anteriores y headers .h de consumidores para contratos y friend del
harness. Loader solo añade friend de test; no cambia su lógica. Los efectos UI con
App no inicializado se inspeccionan; una ventana solo de mensajes evita InvalidateRect(NULL).
Los callbacks se entregan en el hilo principal como en ProcessPendingTextures.

Siguiente recomendación a registrar: diagnóstico 003 empezando por matriz de entrega
IPC (AR-003), antes de corregir protocolo; 002 conserva decisiones pendientes sobre
valor antiguo 4 y placeholder. No se implementan esos trabajos en 004.
