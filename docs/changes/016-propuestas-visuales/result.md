# Resultado: 016

Especificación: [spec.md](spec.md). Plan: [plan.md](plan.md).
Estado: diseño completado y aceptado por el propietario el 2026-10-26.

## Evidencia

Revisión base 9a334ad2 más documentación local en codex/015-roadmap-design,
2026-10-25. Las tres propuestas y sus contratos están en [proposals.md](proposals.md).
Preview local de conversación: viewer-proposals.html, en el directorio de
visualizaciones de este chat; no forma parte del producto ni se publica.

| Criterio | Resultado |
| --- | --- |
| AC-01 | Pasó: tres composiciones distintas presentadas con el mismo contenido sintético |
| AC-02 | Pasó: seis acciones e iconos por variante, una deshabilitada, estados hover/activo y rotación local |
| AC-03 | Pasó en preview: dimensiones/adaptación definidas; verificación automatizada a 1024, 736 y 320 px, sin overflow del panel/barra; autoocultamiento definido |
| AC-04 | Pasó en preview: selección de miniatura actual y marca múltiple distintas, barra sin solapamiento con miniatura activa |
| AC-05 | Pasó: propietario eligió dock compacto, iconos Outline y confirmó barra centrada, miniatura activa de 117 DIP (+30 %) y borde cian sin rótulo |

Render con wrapper de visualize; comprobación Playwright/Edge sin ventana en nueve
combinaciones de variante/ancho. Rotación y selección locales operativas, sin errores
JavaScript. Capturas de las tres variantes revisadas; se ajustó el rótulo de estado
para separarlo del panel de dos filas. Evidencia local ignorada en build/session-015:
check-preview.cjs, proposals-preview.html y variant-0/1/2.png.

Chromium de Playwright no estaba disponible; Edge no inició dentro del sandbox.
La comprobación con Edge fuera del sandbox fue autorizada y pasó. No fue necesario
instalar dependencias. No se ejecutó ni modificó el visor nativo.

## Límites

El preview explora decisiones visuales; sus botones no cambian de página ni escriben
etiquetas/settings reales. No certifica renderizado D2D, foco Win32, transparencia,
DPI físico o rendimiento. El cierre es de diseño; la implementación requiere sus planes propios.
No hay cambios funcionales 017–022 ni renombrado aplicado. Las nuevas láminas de
iconos son exploración generada; no validan legibilidad real a DPI físico ni coste GPU.

## Ampliación: familias de iconos

Generadas e inspeccionadas tres láminas con image_gen integrado: Outline, Soft Solid
y Technical Duotone. Cada una contiene las seis acciones, con labels ingleses,
y una aplicación al dock compacto. Conservadas en concepts/ y enlazadas desde
proposals.md junto con los prompts de estilo y restricciones. Sin edición del
código ni sustitución de assets activos. El propietario confirmó explícitamente:
«Usaremos la propuesta 1. Outline». Quedan aprobados el dock compacto y la dirección
visual Outline para sus seis acciones; la lámina es referencia conceptual, no un
spritesheet listo para cargar en producción.

## Cierre de decisiones — 2026-10-26

Respuestas explícitas del propietario a la comparación visual:

- Barra: «Centrada (recomendado): alineada con la imagen activa».
- Tamaño activo: «117 DIP, +30 % (recomendado): resaltado equilibrado».
- Indicador: «Sin rótulo (recomendado): tamaño y borde cian bastan».

Comparación thumbnail-decisions.html en el directorio de visualizaciones del chat:
tres alineaciones y tres tamaños frente a miniaturas base de 90 DIP. Edge/Playwright
comprobó 27 combinaciones (736/480/320 px × tres posiciones × tres tamaños): tamaño
activo correcto, separación de al menos 12 px respecto a la barra, sin overflow de
barra ni errores JavaScript. Captura revisada. Evidencia local ignorada en
build/session-015/check-thumbnail-decisions.cjs y thumbnail-decisions.png.
Los rótulos numéricos/«activa» del comparador son anotaciones de evaluación;
el producto no llevará rótulo permanente en la miniatura activa.

Se actualizan especificación, plan, propuestas, mapa del lote y roadmap. Se cierra
016 como diseño; 017–022 siguen pendientes de planificación/implementación, sin
atribuirles aceptación funcional ni evidencia de rendimiento del preview.
