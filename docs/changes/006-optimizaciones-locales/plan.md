# Plan: 006 Optimizaciones locales

[Especificación](spec.md). Fecha: 2026-10-22. Estado: completado en alcance.

- [x] T-01: revisar Git, habilidad, guías, fuentes y contratos 004/005; definir alcance.
- [x] T-02: guardar snapshot local de fuentes previas y obtener baseline offscreen
  mediante métodos de dibujo reales, sin ventana/swapchain ni datos personales. AC-03.
- [x] T-03: implementar índice por ocurrencias, intervalo por bisección y guard de
  opacidad; documentar responsabilidad y propiedad en declaraciones. AC-01/02/03.
- [x] T-04: comparación diferencial de reconstrucción y visibilidad, conservar 004,
  comparar píxeles con baseline y microbenchmark acotado. AC-01 a AC-04.
- [x] T-05: build Release/CTest, revisar diff frente al snapshot, registrar resultados,
  guías y roadmap. AC-04.

Propietario del estado: ThumbnailStrip y ViewerPage en UI; ningún worker nuevo.
El índice conserva copias de claves porque mover las entradas invalida referencias
a sus strings. Memoria temporal proporcional al número de rutas/ocurrencias.
Rangos usan geometría existente, sin nueva política de clipping. Guard solo dentro
del dibujo de metadatos; no suprimir otros overlays ni cambios de Update.

Evidencia local bajo build/optimization-006; no versionar capturas, logs o snapshots.
Medir referencia y producción en el mismo ejecutable Release con listas sintéticas
100/1000/10000, repeticiones acotadas y mediana. La equivalencia es condición dura;
las mediciones de tiempo no se convierten en tests frágiles ni promesas de producto.

Recuperación: retirar únicamente el diff 006 usando snapshot local; no git reset ni
restauración global. Sin cambios a fixtures. Sin prueba nativa/Explorer en esta entrega.

Resultado: [evidencia](result.md) y [revisión](review.md). Doce frames WARP idénticos,
250 reconstrucciones diferenciales, 3402 geometrías y Release/CTest 4/4 correctos.
El microbenchmark alterna orden referencia/producción y reutiliza selección en ambos
caminos; cinco muestras tras warmup, mediana sin umbral temporal de test.

Seguimiento: [007](../007-layouts-por-cambios/result.md) implementa layouts por cambios
Library/Waterfall con invalidación por modelo, ancho y parámetros. La recomendación
inicial se ejecutó como cambio propio; índice de visibles masonry queda pendiente.
Siguiente propuesto: formatos DirectWrite internos acotados. Caché de texto, decoder, inicio secundario, uploads y
presentación siguen separados; no extender 006 ni dar esas oportunidades por realizadas.
