# 018: Tira de miniaturas y barra de información

Estado: implementado y comprobado el 2026-10-26. Evidencia en [result.md](result.md).
Origen: tareas 4 y 6 del [lote acordado](../product-batch-20261025.md).
Diseño elegido en [016](../016-propuestas-visuales/proposals.md#miniaturas-y-metadatos).
Predecesor funcional: [017](../017-simplificacion-funcional/result.md), commit 2d19d3d.

## Problema y evidencia previa a 018

Código inspeccionado: Settings y packaging usan thumbnail_width=50. ThumbnailStrip
calcula rectángulos cuadrados uniformes; el estado m_selected agranda solo el alto
24 unidades y también determina borde/opacidad. SyncSelection mezcla inicialmente
imagen actual y selección, pero los gestos de grupo pueden separar ambos estados.
Por ello el tratamiento visual actual no identifica siempre la imagen abierta.

GetVisibleRange, límites de scroll, centrar selección e índices por división usan
el mismo ancho uniforme. IsMouseOver, autohide y clips dependen de HEIGHT=120.
Cambiar solo el dibujo o el default dejaría inconsistencias de entradas y recorte.

El camino activo de información es ViewerPage::DrawOverlayInfo, llamado al dibujar
imagen: centra la barra según ancho medido sin acotarlo al cliente y desplaza su Y
con opacidad de tira y HEIGHT constante. ThumbnailStrip dibuja además un nombre
hover/actual encima. App::DrawOverlayInfo tiene implementación, pero no se encontró
llamador en App: no usarla como si fuera la barra activa ni refactorizarla por inercia.
Estas son observaciones de código, no defectos visuales nuevos reproducidos.

## Resultado esperado

- Tamaño base inicial 44 DIP en código, fallback de tira y defaults del paquete,
  solo sin preferencia previa. Respetar thumbnail_width guardado y control actual.
- La imagen abierta tiene miniatura cuadrada al 130 % de la base: 57,2 DIP con base
  44. Borde cian de 2 DIP y opacidad plena, sin rótulo permanente. El tamaño guardado
  también escala al 130 %; no forzar 44/57,2 a configuraciones existentes.
- Hover y multiselección no sustituyen ni multiplican la marca de imagen abierta.
  La selección múltiple conserva una marca check independiente y sus acciones.
  Las otras miniaturas mantienen el tratamiento atenuado existente.
- La geometría compartida permite dibujar, detectar clics/hover, seleccionar,
  reordenar, limitar visibles y desplazar sin recortar la activa ni solaparla con
  vecinas. Centrar y límites de scroll usan esa misma geometría. Mantener reflejos
  y autohide; no restaurar el backplate retirado en 017.
- Barra de nombre/dimensiones/zoom centrada sobre la tira, alineada con la activa
  cuando está centrada. Separación mínima de 12 DIP respecto a la envolvente de
  miniaturas. En desplazamiento manual acotar la barra al cliente; no perseguir
  hover con la información de otra imagen. Conservar índice/frame e información
  que ya ofrece cada tipo y show_metadata/M. Conservar el desvanecimiento fuera de
  controles; mantener barra/tira estables al consultar la tira, barra o tooltip.
- Nombre largo truncado con acceso al texto completo al pasar el puntero; conservar
  legibles dimensiones/zoom. Acotar ancho/posición para clientes estrechos y evitar
  duplicar el nombre actual entre tira y barra. El editor T no se mueve ni rediseña.
- Usar DIP coherentes con renderer y eventos Win32 a DPI real. Si el cliente no
  permite mostrar toda la superficie, acotar el clip sin tamaños negativos ni
  hit targets invisibles; no reducir arbitrariamente calidad o tamaño preferido.

## Alcance y restricciones

Aplicar la tira donde ya se compone: Viewer. El modal Waterfall conserva su
composición actual; compartir allí tira/entradas completas corresponde a 019.
Si usa el mismo DrawOverlayInfo, debe evitar reservar una tira inexistente. No
introducir otra ViewerPage ni añadir loader o jerarquía de páginas nuevos.

Fuera de alcance: dock/iconos/radial (020), compactación por clic/hover Waterfall
(021), marca (022), nueva composición Waterfall (019), alterar zoom/rotación,
formatos o semántica de metadatos, campañas generales de rendimiento y limpiezas.

App sigue coordinando y poseyendo imagen/índice; ThumbnailStrip conserva selección,
scroll y previews; ViewerPage compone la barra. Mantener composición/friend y UI/
D2D en principal, callbacks en ProcessPendingTextures, identidad/leases/cancelación,
colas/prioridades y virtualización. No cambiar resolución de decode, variantes,
calidad ni presupuesto de caché (tira 100, previews 200) por el tamaño visual 117.
Conservar preferencias true/false de 017, enums, KTAG/KBRD, rutas y atribuciones.

## Criterios de aceptación

| ID | Resultado observable | Comprobación prevista |
| --- | --- | --- |
| AC-01 | Base 44 sin preferencia; tamaños guardados sobreviven y activa mide 130 % | Settings real aislado; defaults packaging y geometría en Release |
| AC-02 | Una única activa, cian 2 DIP, mayor y plena; check múltiple y hover independientes | Regresión de estado y snapshots; recorrido de selección/navegación |
| AC-03 | Dibujo, entradas, clips y visibles usan geometría coherente; centrado/scroll/reordenación conservados | Casos borde y diferencial contra recorrido completo; recorrido nativo |
| AC-04 | Barra centrada sin solapamientos, margen 12 DIP y ancho acotado; nombre completo accesible | Checks geométricos, nombres Unicode/largos, compacto/fullscreen y DPI |
| AC-05 | PNG/JPEG/GIF, zoom/rotación/M/T/controles y cancelación siguen operativos; modal sin regresión | Release/CTest y recorridos nativos sobre copias |
| AC-06 | Sin empeoramiento reproducible fuera de variación conservada en comprobaciones comparables del candidato | Series acotadas contra informes/hashes identificados; límites explícitos |

No quedan elecciones visuales de producto pendientes: se aplican las de 016.
Los detalles de geometría/medición son decisiones técnicas del plan, no otros temas.
Un preview o compilación no acredita aceptación nativa ni rendimiento.

## Ajuste de parametrización del propietario — 2026-10-26

Tras comprobar la evidencia, el propietario considera desproporcionado el tamaño
90/117 elegido en 016 y solicita aproximar los defaults a su captura de Settings.
La pista observada va aproximadamente de x=289 a x=949 y el centro del control
está en x=421: (421−289)/(949−289) ≈ 0,20. El slider real es lineal, rango 30–100;
30 + 0,20 × 70 = 44 DIP. Activa: 44 × 1,30 = 57,2 DIP. Esta decisión sustituye
solo los tamaños iniciales anteriores; no reescribe la elección histórica de 016.

Se conservan proporción, borde, separación, barra, gestos, preferencias guardadas
y rango del slider. No se cambian los demás checks visibles en la captura.
El propietario excluye expresamente nuevas pruebas de rendimiento por tratarse
de parametrización. Las mediciones anteriores permanecen históricas, no se
atribuyen al nuevo binario ni se repiten.


## Ajuste visual posterior del propietario

Fuente flotante recuperada a las 13 unidades cliente originales, sin escalado
adicional por DPI; separación horizontal entre miniaturas **3 DIP**, mitad del
valor anterior 6. No cambia el tamaño guardado ni 44/57,2 inicial. Evidencia de
esta corrección conjunta en [020](../020-navegacion-superior/result.md).
