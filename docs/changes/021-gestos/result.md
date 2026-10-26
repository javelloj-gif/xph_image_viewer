# Resultado 021

Implementado sobre 020 local. Exterior transparente de Viewer usa inversa de rotación/zoom/pan, no alfa del bitmap; fullscreen sale a ventana por SetFullscreen(false) y conserva el ajuste existente. El primer press/release se consume; no se inyectan eventos en otra app. En compacto el fondo no restaura fullscreen por doble clic. Controles/overlays se consumen en App antes del gesto; modificadores y gestos activos excluidos. Waterfall conserva cierre del modal.

Hover compartido pasa de +40 a +200/3 píxeles cliente de ancho, centro y proporción constantes. Dibujo y hit testing siguen el mismo helper.

Release y dos suites afectadas: 2/2, 2,42 s. build/session-021/build.log y ctest.log. Hash 31DE2E5FACDEDF29D9378625BCD47E9DCCF6BE0978D74000CA0FFD247199D2C5. Regresiones: inversa 90°, pan/zoom, interior transparente, exterior, press/release, doble clic compacto, metadatos, gesto activo, ventana opaca y exclusión Waterfall; hover centro/aspecto/incremento. Recorrido nativo final pendiente de registrar al cerrar lote; no declarar evidencia física por compilar.


### Continuidad: comprobación de la franja compacta

La validación nativa interrumpida por Escape físico se retomó en el candidato de
corrección visual de 020 (A975803A…). Settings funciona desde la franja superior
compacta; doble clic exterior sobre JPEG estrecho conserva el tamaño compacto.
La corrección de HitTest está presente y su regresión nativa oculta pasa en la
suite completa 9/9. No equivale a una matriz universal de gestos/DPI; ver límites
y recorrido en [020](../020-navegacion-superior/result.md).
