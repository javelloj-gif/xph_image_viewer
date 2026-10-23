# Revisión: 006 Optimizaciones locales

Fecha: 2026-10-22. Habilidad lenteuv-architecture-review, revisión en sesión actual.
Cobertura: diff frente a snapshot previo de thumbnail_strip.cpp/.h, viewer_page.cpp/.h,
renderer.h y async_preview_tests.cpp; contratos de tokens 004 y build/harness existentes.
Consultados Git/AGENTS/workflow/contexto/roadmap/desarrollo y documentos 003/005.
No revisión global de Board, codecs, scheduler, cierre o cachés de metadatos.

## Hallazgos y resolución

- Equivalencia de ocurrencias: las claves del mapa son copias independientes, por lo
  que mover strings antiguas no invalida el índice. Cadena creada en orden inverso
  consume en orden original. Primera ocurrencia nueva replica el find_if previo,
  incluso si está pendiente; no copia isLoading/token a duplicados adicionales.
- Bordes float: bisección llama GetThumbnailRect, no aproxima índices con división.
  Ambos filtros originales siguen activos y el orden ascendente se mantiene.
  Guard de base no finita evita predicados no monótonos por overflow; ancho inválido
  y count fuera de int conservan camino previo. 3402 geometrías contrastadas.
- Overlay: condición previa es exactamente la que ya impedía sus comandos de dibujo;
  solo elimina preparación. El check interno se conserva para minimizar diff.
  No cambios a Update ni estado de fade/zoom ni otros overlays.
- Memoria/propiedad: índice temporal proporcional al total de entradas, sin tokens
  compartidos nuevos, retención persistente ni recursos GPU en worker. Coste temporal
  de memoria documentado; no se declara una reducción de RAM global.
- Pruebas: baseline WARP se generó antes de editar producción; hash idéntico después.
  Oráculo de reconstrucción conserva código anterior y compara estado, no solo tamaño.
  Microbenchmark alterna orden y mantiene ambas selecciones reutilizables. Tiempos
  no se convierten en aserciones frágiles ni en promesas de arranque.

No hallazgos accionables pendientes en el alcance. Comentarios de responsabilidad
en métodos modificados/nuevo helper y structs de soporte; accesos friend solo para
harness offscreen. Release /W4 /WX y CTest 4/4 correctos; git diff --check pasó.

## Límites

Equivalencia en casos ejecutados, no prueba absoluta de toda UX. WARP offscreen no
certifica Present/compositor/driver físico; no se abre ventana real en esta entrega.
Los futuros layouts/cachés/scheduler necesitan alcance y validación propios. El diff
previo R1/R2/004/005 continúa sin commit y no se restaura ni normaliza por esta revisión.
