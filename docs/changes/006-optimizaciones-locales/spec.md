# 006: Trabajo redundante en tira y overlay

Fecha: 2026-10-22. Estado: completado en alcance; [evidencia](result.md). El propietario autoriza
comenzar optimizaciones conservando experiencia y presentación. Base ac4a5da más
trabajo previo sin commit, preservado en codex/repository-foundation.
Origen: [OPT-001, OPT-002-tira y parte mínima OPT-003](../003-diagnostico-rendimiento/source-optimization-review.md).

## Objetivo y alcance

Sustituir búsquedas/erase cuadráticos de SetFileList por un índice temporal de
ocurrencias; recorrer en Update/Draw solo el intervalo candidato visible de la tira;
no construir ni medir metadatos si la opacidad actual impide dibujarlos.
Se mantienen rectángulos, orden y comandos de dibujo de cada elemento visible.

Las ocurrencias antiguas se consumen en orden, una vez cada una, conservando textura,
error, token y estado pendiente. Las ocurrencias adicionales solo copian textura y
error de la primera ocurrencia nueva de esa ruta, exactamente como antes; nunca
copian un token pendiente. La selección se reinicia como antes. El índice tiene vida
local O(N); no añade caché persistente ni limita imágenes. El hashing da coste
promedio O(N), no una garantía de peor caso lineal.

El intervalo usa las mismas operaciones float y límites inclusivos que el filtro
existente. Buscar sus extremos por bisección evita diferencias por redondeo de una
división. Ante parámetros no finitos o ancho de miniatura no positivo se conserva
el recorrido completo y los filtros originales. Animaciones y eventos no cambian.

## Exclusiones

Layouts Library/Waterfall, cachés de texto, decoder, orden de inicialización IPC,
cancelación/uploads, cadencia/Present, cierre, Library, enums y persistencia.
No bajar resolución, efectos, FPS ni calidad. No atribuir mejora de arranque/reposo
completo a microbenchmarks. No modificar asociaciones ni datos personales.

## Aceptación

- AC-01: estado de reconstrucción idéntico al algoritmo previo, incluidos duplicados,
  pendientes, texturas, errores, Unicode, cambios de orden y selección reiniciada.
  Comparación diferencial y regresiones 004.
- AC-02: Update/Draw conservan exactamente los conjuntos y orden visibles, bordes
  inclusivos y flotantes. Comparación diferencial de rangos y filtros originales.
- AC-03: píxeles de tira y overlay idénticos antes/después en render offscreen WARP,
  con borde, scroll fraccionario, selección/hover, reflejos/crop, placeholders, drag,
  opacidad cero/intermedia/completa, Unicode y modal Waterfall. No certifica compositor,
  interacción nativa ni toda la aplicación. Mantener Update/fade sin cambios.
- AC-04: Release y CTest completo pasan; comentarios y revisión del diff. Comparación
  temporal acotada con algoritmo previo bajo el mismo proceso/datos, sin umbral de
  velocidad de aplicación ni promesa porcentual; registrar memoria temporal y límites.

Si la equivalencia falla se corrige o retira la porción afectada antes de aceptarla.
La comparación visual nativa exhaustiva queda fuera de esta porción; toda discrepancia
posterior exige revisar equivalencia. Ninguna decisión de producto pendiente.
