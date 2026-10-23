# 007: Layouts Library/Waterfall por cambios

Fecha: 2026-10-22. Estado: completado en alcance; [evidencia](result.md). Autorizado continuar optimizaciones
conservando UX/presentación. Base ac4a5da más R1/R2/003/004/005/006 sin commit.
Origen: porción de layouts de [OPT-002](../003-diagnostico-rendimiento/source-optimization-review.md).

## Alcance y contrato previo

Reutilizar los rectángulos existentes mientras sus entradas no cambien. Mantener
exactamente el cálculo greedy, operaciones float, fallback cuadrado, desempate por
primera columna, scroll, hit testing, hover, clipping, sombras y orden de dibujo.
Sin índice de visibles para masonry, algoritmos nuevos, caché adicional de rectángulos,
reducción de calidad/FPS/efectos, retirada de Library o cambios de datos persistidos/IPC/cierre.

FolderDataManager publica una revisión transitoria UI que cambia al limpiar/agregar/
quitar/reordenar previews o aplicar una textura válida. Las mutaciones directas de App
y Library notifican al mismo servicio. Su acceso mutable prestado requiere notificar
tras cambiar estructura/orden o dimensiones efectivas de previews. Tokens/fallos de
004 permanecen intactos; descartes y fallos sin textura no requieren nuevo layout.
La revisión es global al modelo: un cambio en carpeta inactiva puede causar una
reconstrucción extra, pero ninguna pasada por todos los previews en un frame estable.

Cada página guarda revisión, índice seleccionado, ancho y cantidad; Waterfall también
columnas y espaciado. Inicializar o pedir cálculo sin carpeta invalida su clave. El cálculo
publica clave válida solo al terminar. No usar punteros a elementos reubicables como
identidad. Scroll, hover, alto de ventana y corner radius no alteran rects base.
Sin nueva retención proporcional a N: se reutiliza PreviewItem::rect y m_previewRects.
El código que modifique esas salidas directamente debe invalidar el modelo.

## Aceptación

- AC-01: mismos bits de rectángulos que algoritmo previo para ambas páginas, listas
  vacías/mixtas, distintos anchos/columnas/gaps y proporciones. Repetir sin cambios no
  reconstruye. Mutaciones de orden/lista/carpeta, resize, parámetros y textura entregada
  por cualquiera de las páginas invalidan antes del próximo uso existente del cálculo.
- AC-02: baseline antes de editar producción y mismos píxeles en frames WARP offscreen
  Library/Waterfall con scroll, hover, resize/DPI y entrega progresiva. Sin certificar
  compositor/interacción nativa exhaustiva. Si difiere, corregir o retirar la porción.
- AC-03: Release/CTest completo pasan, conservando regresiones 004/005/006. Revisar
  consumidores y mutadores, comentarios y diff contra snapshot local preservando lo previo.
- AC-04: medir exclusivamente cálculo repetido estable/invalidadas con datos iguales
  frente al cálculo productivo forzado sin reutilización, conservando el algoritmo previo.
  Mediana sin umbral temporal frágil; no atribuir
  ahorro de arranque, reposo, GPU ni RAM global. Registro del coste fijo de claves.

Sin decisiones de producto pendientes. Cualquier cambio funcional de hit testing,
política de precarga o animación queda fuera de esta entrega.
