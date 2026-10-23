# Plan: 007 Layouts por cambios

[Spec](spec.md). Estado: completado en alcance. Fecha: 2026-10-22.

- [x] Revisar Git, AGENTS, habilidad, workflow/contexto/roadmap, 006 y caminos activos.
- [x] Guardar snapshot de fuentes previas y baseline WARP con grids productivos. AC-02.
- [x] Añadir revisión transitoria del modelo y notificar mutaciones productivas. AC-01.
- [x] Guardar claves en páginas sin cambiar fórmulas ni consumidores geométricos. AC-01.
- [x] Regresiones diferenciales/invalidation y comparación de píxeles. AC-01/02/03.
- [x] Benchmark acotado, Release/CTest, revisión y documentación. AC-03/04.

Mutadores auditados: FolderDataManager Initialize/AddFolder/RemoveFolder/SortFolder/
CompletePreview; Library AddFolder; App DeleteImage. Selección se detecta en clave.
LoadSession agrega mediante AddFolder; cargas llegan en UI vía CompletePreview;
App ApplySortMode usa SortFolder. Dimensiones de ImageData se fijan al decodificar,
antes de entregar; upload posterior no cambia ancho/alto. Revisión no se persiste.

Pruebas en harness existente, sin fixtures ni sesión personal. WARP sin ventana ni
swapchain; artefactos ignorados bajo build/optimization-007. Oráculo preserva algoritmo
previo. Pruebas de hit verifican que no se reescriba una salida marcada; toda mutación
del input de pruebas notifica como exige el contrato. Sin temporizadores nuevos.
Benchmark estable y forzado: 100/1000/10000 entradas, orden de medición alternado,
warmup y mediana; no prueba de rendimiento de aplicación completa. La base temporal
fuerza CalculateLayout productivo sin hit; las fórmulas coinciden con el algoritmo
previo. El oráculo independiente queda solo para equivalencia, pues sus escrituras
contiguas/código unificado no son una referencia justa de coste para Library.

Recuperación: retirar solo diff 007 mediante snapshot, sin reset/restauración global.
Resto de candidatos OPT-003..007 y pendientes 003/AR-005/006/007 permanecen separados.
Resultado: [evidencia](result.md), [revisión](review.md). 140 modelos con varias
transiciones y comprobaciones dirigidas; 24 frames de grids idénticos y los 12 de 006
conservados. Release/CTest 4/4; 160440 checks totales en harness ampliado.

Seguimiento: [008](../008-formatos-texto/result.md) implementa formatos DirectWrite
privados acotados, clave efectiva, lifecycle y fábrica pública mutable independiente.
Siguiente recomendado: especificar decoder único OPT-004. Índice de visibles masonry
y última medida del overlay siguen sin implementar. No añadir
scheduler, cancellation, ACK ni cambios de Library como efecto secundario.
