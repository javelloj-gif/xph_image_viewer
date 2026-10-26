# Lote de producto 015–022

Estado: mapeo aprobado por el propietario el 2026-10-25. La autorización comprende
registrar el lote, ejecutar 015 y presentar 016. Los cambios funcionales posteriores
se desarrollarán mediante planes propios. Diseño 016 completado y elegido el
2026-10-26: dock compacto, Outline, barra centrada y miniatura activa 117 DIP sin rótulo.

## Acuerdos de comportamiento

- 017: `animated_zoom=true` será el valor sin preferencia previa; respetar los valores
  guardados y el control existente. No eliminar ni alterar incidentalmente `effects`.
  Doble clic fullscreen permanente; ignorar la antigua preferencia desactivada y
  dejar de escribirla. Retirar desaturación de botones, inversión de imagen,
  cyberpunk corners y el backplate opcional de la tira, incluidos menús, atajos,
  código y claves exclusivos. No retirar superficies de Settings, etiquetas ni el
  atenuado de Waterfall por compartir la palabra backplate. Las claves retiradas
  de configuraciones anteriores deben tolerarse sin cambiar el resto de preferencias.
- 018: `thumbnail_width=90` sin preferencia guardada, en defaults de código y paquete.
  Imagen abierta resaltada y mayor independientemente de multiselección/hover.
  Ajustar límites visibles, clipping, hit testing, scroll y altura de tira según el
  diseño elegido; mantener la calidad y reutilización de miniaturas. Reubicar la
  barra de nombre/dimensiones/zoom; no rediseñar el editor de etiquetas T.
  Elección 016: barra centrada sobre la tira y alineada con la imagen activa;
  activa 117 DIP frente a base 90 DIP (+30 %), borde cian, sin rótulo permanente.
- 019: compartir la composición y entradas del visor completo en Waterfall. Clic
  exterior y Escape regresan a la cuadrícula conservando carpeta y scroll. Los
  overlays y la tira consumen primero sus eventos; no cerrar al usarlos.
- 020: panel solo en Viewer y visor superpuesto de Waterfall. Acciones: Waterfall,
  Board, Rotate 90°, Tags (T), Sort & Filter (deshabilitada; futura), Settings.
  Retirar el radial de todos los modos; añadir Viewer a los menús contextuales de
  Waterfall y Board y conservar NUM1. Mantener los demás atajos salvo los retirados
  explícitamente en 017.
  Elección 016: formato dock compacto con iconos de la familia Outline.
- 021: primer clic izquierdo en fondo transparente exterior a la imagen compacta
  solo Viewer; segundo clic interactúa con la ventana inferior. No reenviar eventos
  a otras aplicaciones. Excluir controles, overlays, menús y gestos activos. El
  interior transparente de una imagen no cuenta como exterior. En compacto no
  alternar a fullscreen al pulsar el fondo. Hover Waterfall: incremento de ancho
  actual +40 pasa a +66,7 aproximadamente, con proporción y centro conservados.
- 022: Image Viewer / image_viewer son placeholders neutrales, documentados en el
  contexto. Retirar marca existente de código y herramientas activas; preservar
  temporalmente contratos de datos/IPC/integración Windows y registrar excepciones.
  No reescribir entregas históricas, licencia ni atribuciones.

## Diseño y arquitectura

016 presentará barra horizontal, panel de dos filas y dock compacto. Cada propuesta
mostrará iconos, hover, activo/deshabilitado, activación/ocultamiento, compacto y
barra informativa sin solapamientos. No son tres temas de color del mismo layout.
La elección del propietario, completada el 2026-10-26, fija los criterios visuales
de los planes dependientes; no demuestra implementación nativa.

Código inspeccionado: Waterfall ya usa `App::m_viewerPage`; faltan composición y
enrutamiento de tira compartidos. La solución mínima conserva App como coordinador,
composición y `friend`; no añade un visor, loader o jerarquía de páginas nuevos.
Conservar UI/GPU en principal, callbacks en ProcessPendingTextures, leases y tokens,
prioridades de carga, caché 100/200, virtualización y layouts por cambios. No cambiar
enums persistidos, formatos KTAG/KBRD, rutas ni gestos como efecto de reorganización.

## Validación del lote

Identidad inicial: Git `9a334ad2b775bd5fe7244e805384ea4bed2038cd`, árbol limpio al
comenzar; portable final 003 conservado. En 015 registrar hashes e inventario antes
de retirar artefactos. Esto fija una referencia reproducible, no una medición nueva.

**Ajuste expreso del propietario, 2026-10-26:** usar las mediciones conservadas de la
última modificación; no dedicar recursos a ejecutar, recompilar o medir de nuevo
una base sin cambios funcionales. Esta decisión sustituye el requisito anterior
de obtener nuevas series de base antes de implementar. 017 enlaza los informes
del último EXE y su identidad comprobada en 015.

Concentrar la validación nueva en el candidato y contrastar con la referencia
conservada: arranque, original, tira/Waterfall, navegación y recursos según cobertura
disponible. Registrar revisión, hashes y condiciones; separar las limitaciones
históricas de los resultados nuevos, sin inventar métricas ausentes ni atribuir
causalidad a comparaciones entre entornos distintos. No repetir oportunidades frías
consumidas ni campañas generales por inercia. Un empeoramiento reproducible fuera
de la variación histórica disponible bloquea entrega; no hay tolerancia porcentual
aprobada. Los datos de pruebas y staging siguen aislados de datos personales.

Release/CTest más recorridos de UI proporcionales: configuración nueva/existente,
compacto/fullscreen, DPI, rotación, multiselección, animación, navegación rápida,
retorno a Waterfall, overlays y cierre/cancelación. Compilar o inspeccionar un mockup
no demuestra UX nativa ni rendimiento. Cada resultado debe separar código,
ejecución, reporte e hipótesis.

## Continuidad

Consultar [roadmap](../roadmap.md). No reabrir la base aceptada ni sus seguimientos
como trabajo implícito. Crear spec/plan/result de 017–022 al desarrollarlos;
documentar responsabilidad de cada clase/método nuevo o modificado.

## Ajuste posterior de 018 — 2026-10-26

Tras probar la implementación, el propietario sustituye el tamaño inicial 90/117
por una aproximación a su captura del slider: **44 DIP base / 57,2 DIP activa**
(20 % del rango lineal 30–100, activa +30 %). Solo parametrización; preferencias
guardadas, borde, barra y demás decisiones conservadas. Sin nuevas pruebas de
rendimiento por instrucción expresa. La elección de 016 permanece como historial;
la especificación y resultado de 018 contienen los valores vigentes y evidencia.


## Ejecución posterior — 019–022

El propietario autorizó expresamente los pasos 2 y 3 del estado: especificar y
planificar 019, implementar secuencialmente 019/020/021/022. No autorizó mediante
esa petición confirmar selectivamente el árbol anterior (paso 1); no se crean
commits ni se mezclan los documentos locales 015/016. Resultados independientes
y [evidencia final de 022](022-neutralizacion-marca/result.md) registran el cierre,
paquetes, pruebas y límites. La excepción de no medir del ajuste de parámetro 018
se respetó; las series nuevas corresponden a los cambios funcionales 019–022,
siempre candidato, sin ejecutar/recompilar una base histórica.


## Revisión visual posterior del propietario

La referencia Outline original es vinculante para los detalles, no solo para la
familia: fotos/columnas, tres fotos, arco+cuadro, dos tags, orden+embudo y engranaje.
El dock pasa expresamente a **centrado superior**; se recupera la fuente flotante
original 13 y se reduce gap de tira de 6 a 3 DIP. El tamaño 44/57,2 y preferencias
se conservan. Especificación/pruebas/resultado de la corrección en 020 y ajuste
asociado en 018. No se reescribe el diseño histórico 016 ni se repite rendimiento.

Aprobación posterior, 2026-10-26: el propietario confirma «Cambios aprobados» para la corrección visual entregada (dock Outline centrado, fuente original y gap 3 DIP). Aceptación registrada en 020/result.md; se mantienen los límites y no se infiere autorización de commit/publicación ni nueva evidencia de rendimiento.

## Cierre de sesión — 2026-10-26

Posteriormente el propietario autoriza expresamente el commit de cierre de esta
sesión. Se incluyen la implementación y documentación de 018–022 y los archivos
locales preservados de preparación/diseño 015/016. Esta autorización sustituye
el estado anterior pendiente de commit; no autoriza publicación remota.
Se conserva la evidencia del candidato aprobado, sin nuevas mediciones ni cambios
funcionales durante el cierre. La siguiente etapa revisará picos de uso de GPU;
su reproducción, medición y diagnóstico quedan pendientes para esa etapa.
