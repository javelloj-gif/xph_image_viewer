# Documentos por cambio

Para un cambio funcional, copiar `_template/` a `<id>-<nombre>/` dentro de esta
carpeta y registrar el cambio en la [hoja de ruta](../roadmap.md).
Ejemplo de nombre: `001-descripcion-del-cambio`.

`_template/` contiene plantillas, no un cambio propuesto ni trabajo aprobado.
Sustituir los marcadores entre corchetes y eliminar filas de ejemplo al utilizarlas.
Si una sección no aplica, explicarlo brevemente; si falta información, marcarla como
pendiente. No rellenar apartados con afirmaciones sin evidencia.

- [Especificación](_template/spec.md): qué debe cambiar y cómo aceptarlo.
- [Plan](_template/plan.md): cómo implementarlo y continuar entre sesiones.
- [Resultado](_template/result.md): qué ocurrió y qué se pudo verificar.

Seguir el [flujo de trabajo](../workflow.md). Mantener los documentos breves y
ampliarlos solo cuando el cambio lo requiera.

## Trabajos registrados

- [001: Refactorización base y auditoría](001-refactorizacion-base/spec.md).
- [002: Retirada de Library](002-retirar-library/spec.md).
- [003: Diagnóstico de rendimiento](003-diagnostico-rendimiento/spec.md).
- [004: Vigencia de miniaturas asíncronas (AR-002)](004-vigencia-miniaturas/spec.md).
- [005: Entrega IPC y fallo explícito (AR-003)](005-entrega-ipc/spec.md).
- [006: Optimizaciones locales de tira y overlay](006-optimizaciones-locales/spec.md).
- [007: Layouts Library/Waterfall por cambios](007-layouts-por-cambios/spec.md).
- [008: Formatos DirectWrite internos acotados](008-formatos-texto/spec.md).
- [009: Decoder compartido para carga del visor](009-decoder-compartido/spec.md).
- [010: Inicialización secundaria mínima (OPT-005)](010-inicializacion-secundaria/spec.md).
- [011: Vigencia antes del upload (OPT-006)](011-vigencia-antes-upload/spec.md).
- [012: Cierre de colas y servidor IPC (primera porción AR-005)](012-cierre-colas-ipc/spec.md).

Consultar la hoja de ruta para prioridad y estado. Un requisito registrado no significa
que esté implementado ni que sus decisiones pendientes se hayan resuelto.

## Lote de producto 2026-10-25

- [Acuerdos y mapa 015–022](product-batch-20261025.md).
- [015: Preparación y limpieza](015-preparacion-limpieza/spec.md).
- [016: Propuestas visuales](016-propuestas-visuales/spec.md).
- [017: Simplificación funcional](017-simplificacion-funcional/result.md), implementada y comprobada; commit 2d19d3d y límites de comparación documentados.
- [018: Tira y metadatos](018-tira-metadatos/result.md), implementado y comprobado; Release/CTest 9/9, tamaños iniciales ajustados a 44/57,2, barra y límites documentados.

- [019: Composición compartida](019-composicion-compartida/result.md).
- [020: Navegación superior](020-navegacion-superior/result.md).
- [021: Gestos](021-gestos/result.md).
- [022: Neutralización de marca](022-neutralizacion-marca/result.md).

019–022 se implementan secuencialmente por autorización expresa del propietario;
los resultados distinguen cada candidato y la validación conjunta final.

- [013: Cierre de la refactorización base](013-cierre-base/spec.md).

- [014: Completar cancelación AR-005](014-cancelacion-ar005/spec.md).
