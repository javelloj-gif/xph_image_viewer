# Revisión local de 018

Fecha: 2026-10-26. Habilidad aplicada: lenteuv-architecture-review.
Base 2d19d3d, diff local 018 en codex/015-roadmap-design. No revisión independiente.
Leídos spec/plan/result, reglas y criterios locales. Cobertura: Settings/defaults,
ThumbnailStrip completo en los caminos modificados, ViewerPage Update/barra y
entradas modificadas, retorno animado App::ApplyLoadedImage, comentarios, tests
modificados y composición/routing de Viewer/Waterfall. No se reauditan loader,
Board completo, serializadores, seguridad de códecs o todos los escritorios/DPI.

AR-018-01 — defecto del primer candidato, confirmado en recorrido visual, prioridad
media: autohide de tira movía la barra mientras se consultaba el nombre/tooltip.
Ubicación: ThumbnailStrip::Update y ViewerPage::Update/DrawOverlayInfo. Resuelto
manteniendo actividad cuando el puntero está en tira/barra/tooltip. Conserva fade
fuera y hace legible el nombre sin clicks sobre el visor detrás. Verificado en
pasada posterior del candidato; no deriva un menú/modal nuevo ni cambia hilos.

AR-018-02 — riesgo de identidad/duplicados en la intervención, confirmado por código,
prioridad media: índice solicitado puede preceder a la entrega del original y
reordenación por rutas puede eliminar todas las ocurrencias de una ruta repetida.
Resuelto en el alcance modificado mediante identidad aplicada y reordenación por
índices de ocurrencia; contrato anterior de reutilización de SetFileList intacto.
Regresiones pending/applied, duplicados y reordenación reales, sin modificar
latest-wins, tokens/leases ni loader. No se certifican todos los gestores del proyecto.

Revisión final: sin otros hallazgos accionables pendientes en los caminos inspeccionados.
Geometría O(1), búsqueda binaria conservada, UI/D2D/formatos/layout en principal;
solo un layout de tooltip retenido por nombre/ancho/fuente, con COM RAII. No se
mutan formatos internos de Renderer. La resolución 100/200 y presupuesto de caché
no cambian por la ampliación. Los métodos modificados/nuevos tienen contratos en
sus declaraciones o comentario común de familia. No hay nuevos enums/rutas/formatos.

Validación en result: Release/CTest 9/9, 3251 checks nuevos, 12 frames WARP, recorridos
nativos y mediciones exclusivas del candidato. No certificación universal de UI,
DPI físico, recursos o rendimiento; no equivalencia causal por solapamiento de rangos.
