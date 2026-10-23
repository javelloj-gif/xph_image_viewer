# Revisión: 007 Layouts por cambios

Fecha: 2026-10-22. Habilidad lenteuv-architecture-review, revisión en sesión actual.
Base ac4a5da y diff previo preservados; comparación específica con snapshot 006 local.
Leídos Git, AGENTS, workflow, contexto/roadmap, 006 y referencias de la habilidad.

## Cobertura

CalculateLayout/Initialize y consumidores de geometría de Library/Waterfall;
LoadPreviewTexture/CompletePreview, mutadores FolderDataManager, App::DeleteImage,
Library::AddFolder, rutas de sort/session/selección y uso mutable de GetFolders.
Cambios C++: esas páginas, gestor, App y harness; nueve fuentes frente al snapshot.
Búsquedas globales de escritura previews/texture/rect para localizar mutadores.
No auditoría global de Board, codecs, cachés, IPC, cierre o renderer/scheduler.

## Hallazgos y resolución

- LC-001, riesgo confirmado de invalidación cruzada: cada callback antes solo pedía
  cálculo de su página. Epoch en FolderDataManager::CompletePreview (línea 319) y clave
  por página solucionan reutilización obsoleta en el otro consumidor. Mutadores del
  gestor y rutas directas App::DeleteImage/Library::AddFolder notifican la misma revisión.
  Época UI-only, sin locks ni persistencia; tokens y rechazo de resultados intactos.
- LC-002, riesgo de claves ligadas a referencias reubicables: no se guarda puntero a
  FolderInfo/PreviewItem. Índice + revisión/width/count cubren reemplazo y movimientos
  mediante contrato de notificación. No hay N entradas de claves ni snapshots por frame.
- Geometría: cuerpos greedy originales intactos; solo guard previo/publicación final.
  Library conserva rect en modelo y Waterfall su vector. Scroll/hover/altura/radius no
  entran en clave porque no afectan el rect base. Columnas y gap sí; tests separados.
- Medición: oráculo independiente se usa para equivalencia, no como coste comparable
  de producción por su almacenamiento distinto. Benchmark fuerza el mismo cálculo
  productivo, y separa retorno estable de reconstrucción por revisión.

No hallazgos accionables pendientes en alcance. Responsabilidad/contratos añadidos
a métodos modificados y acceso mutable; revisión transitoria nunca se reinicializa a
cero al limpiar modelo. API pública mutable sigue exigiendo disciplina de notificación;
es riesgo de mantenimiento documentado, no prueba de error actual.

## Comprobaciones y límites

140 modelos con transiciones, callbacks reales, 24 frames WARP idénticos y 12 frames
006 conservados; Release/CTest 4/4 y git diff --check pasaron. Sin ventana visible ni
prueba de interacción/compositor completa. No afirmar menor RAM/GPU/arranque global.
No nueva abstracción de layout ni retirada de Library, renumeración o cambio de datos.
Actualización de guías y roadmap consistente; fuentes previas conservadas sin commit.
