# Lote 026–030: organización, navegación e idiomas

Acuerdo del propietario, 2026-10-26. Library significa Board, no un modo nuevo.

| Entrega | Resultado y aceptación | Dependencias |
| --- | --- | --- |
| 026-menu-tablero | AC-01: seis acciones en Viewer/modal. AC-02: envío único al tablero en memoria sin navegar ni perder cambios al entrar | Preparación del tablero separada de entrada |
| 027-orden-filtro | AC-03: seis órdenes, filtros combinables persistentes y modal Aplicar/Cancelar. AC-04: snapshots cancelables, conservación de selección/cachés y cierre seguro | Prioridad de implementación |
| 028-docks | AC-05: seis acciones adaptadas al modo; Viewer primero en Waterfall/Board; rotar y ordenar bloqueados en Board; tres propuestas Outline | 027 habilita ordenar |
| 029-metadatos | AC-06: desplazamiento vertical suave con la tira; conservar inactividad, tooltip y efectos | Composición compartida |
| 030-localizacion | AC-07: opciones, mensajes, tooltips y accesibilidad en inglés/ruso/español; botón Español | Integra textos de las demás entregas |

Preferencias: globales Viewer/Waterfall; fecha de modificación; familias JPEG/JFIF,
PNG/APNG, GIF, BMP, TIFF, WebP, AVIF, HEIF/HEIC y Otros. Todos inicial; cero permitido.
Imagen excluida permanece abierta con índice -1; siguiente/previa al primer/último
resultado. Envío al tablero con guardado manual. Icono base: foto individual.

CPU y E/S fuera de UI; publicar solo generaciones vigentes. Sin reasignar enums,
formatos, rutas o gestos. Métodos modificados documentan responsabilidad y contratos.

Validación funcional por entrega con datos aislados. Medir arranque/rendimiento
solo tras completar 027/028, comparando binario base y candidato en el mismo entorno:
30 arranques por escenario (defaults y selección guardada), mediana <= +5%,
p95 <= +10%. Registrar reutilización, primera aplicación y reposo. No confundir
caché de aplicación vacía con boot frío. Resultados pendientes no prueban aceptación.
