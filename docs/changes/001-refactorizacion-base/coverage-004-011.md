> Disposición vigente 2026-10-23: [cierre de la base](closure.md). La cobertura restante se conserva como revisión futura por componente; no son tareas obligatorias de 001. Aceptación funcional R2/010/012 reportada por el propietario; límites AR-005 explícitos y AR-006 corregido en 013.

# Inventario acumulado tras 004–011

Fecha: 2026-10-22. Complementa el inventario histórico de [review](review.md), sin
reescribir su evidencia ni convertir revisiones de diffs en auditorías globales.
Fuentes: spec/result/review de cada entrega. No nueva validación visual en este inventario.

| Entrega | Cobertura añadida y contratos revisados | Evidencia y cobertura aún pendiente |
| --- | --- | --- |
| [004](../004-vigencia-miniaturas/review.md) | Tira SetFileList/LoadThumbnail/CompleteThumbnail; previews compartidos, CompletePreview y mutadores de lista/orden; identidad token/ruta y entrega UI | 98 checks iniciales. AR-002 resuelto; no hover, Board async ni cancelación/cierre |
| [005](../005-entrega-ipc/review.md) | IPC completo para transporte, estados Initialize, reintentos, payload y traducción App/main/Window; harness y script | AR-003 corregido en alcance; 5/5 entregas y concurrencia. Presentación real posterior en 003; ACK, elección y cierre excluidos |
| [006](../006-optimizaciones-locales/review.md) | Reconstrucción/ocurrencias y rango visible de tira; guard de overlay invisible, ownership de índice temporal | Equivalencia diferencial, 12 frames WARP; sin revisión general de gestos, scheduler o memoria |
| [007](../007-layouts-por-cambios/review.md) | Layouts Library/Waterfall, claves y revisión; mutadores de gestor, DeleteImage/AddFolder y acceso mutable a previews | 140 modelos y 24 frames; acceso mutable exige notificación. No auditoría completa de borrado, eventos/modal ni persistencia |
| [008](../008-formatos-texto/review.md) | Renderer DrawText/MeasureText/CreateTextFormat, caché privada, factory y descarte de recursos relacionados | Propiedades/aislamiento y comparación WARP controlada; variación inicial documentada. No pérdida de dispositivo/compositor global |
| [009](../009-decoder-compartido/review.md) | Decoder estático/animado, composición existente, cargas sync/async y App::LoadImage; proxies WIC y aislamiento | 294 checks; fallback/bytes/metadata. No certificación universal de códecs, FrameSequencer ni cargas grandes |
| [010](../010-inicializacion-secundaria/review.md) | App::Initialize, prerequisitos Window/IPC/servicios, callbacks tempranos y fallos parciales; main y harness nativo | Diez escenarios nativos; aceptación visual pendiente. No auditoría global del cierre ni mejora de latencia demostrada |
| [011](../011-vigencia-antes-upload/review.md) | LoadAsync/ProcessPendingTextures, predicado UI, identidad de tira/previews, callbacks y revisión 007 | 321 checks nuevos, 162022 async totales y CTest 5/5. No cancelación CPU, cierre, hover/Board ni mejora global |

## Cobertura restante por componente

- App/Window: rutas revisadas de arranque/carga/IPC y mutaciones de previews; falta
  revisión integral de routing, borrado, transformaciones y ciclo de vida.
- Gestor compartido: R2 y mutadores 004/007/011 revisados; no separar navegación y
  persistencia ni unificar AddFolder de Library sin contrastar efectos propios.
- Páginas: tira y geometría revisadas por porciones; falta interacción completa,
  hover/animaciones, modal Waterfall y Viewer. Library sigue operativa.
- Renderer: texto/caché privados revisados; pendientes dispositivo/pérdida, presentación,
  animaciones y scheduler. OPT-007 sigue separado.
- Loader: decode y entrega revisados; AR-005 requiere contrato y pruebas de aceptación
  de trabajo, cancelación y cierre. No inferir que 011 cancela decodificación.
- Board y auxiliares, metadatos/escritores, Settings/overlays, shell/i18n: conservan
  cobertura parcial/inventariada de 001 salvo llamadores expresamente revisados arriba.

## Hallazgos y seguimiento actual

AR-001/004 resueltos por R2/R1; AR-002 por 004 y ampliación 011; AR-003 por 005,
con ejecución real posterior de 003. AR-005 comienza en [012](../012-cierre-colas-ipc/spec.md)
por petición del usuario. AR-006 sigue pendiente; AR-007 se cumple en símbolos
modificados y mantiene deuda progresiva. Recorrido visual R2 y aceptación 010 pendientes.
Retirada 002 exige decisiones de valor antiguo 4 y placeholder; no autorizada en 012.
