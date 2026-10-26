# 017: Simplificación funcional del visor

Estado: implementado y comprobado el 2026-10-26; límites de comparación conservados.
Evidencia y límites en [result.md](result.md).
Origen: tareas 1 y 2 del [lote acordado](../product-batch-20261025.md).

## Problema y evidencia de código previa a 017

Settings inicializa animated_zoom=false; el paquete también. Las opciones retiradas
tienen controles y serialización. La desaturación cambia el color del botón de
cierre de App. Cyberpunk corners crea geometrías D2D al dibujar; el backplate
opcional rellena el fondo de la tira. Invert Colors cambia m_invertImage, sin un
consumidor de render encontrado en la búsqueda del símbolo. No se afirma que un
filtro de inversión esté funcionando ni que retirarlo mejore tiempos.

Waterfall ya delega doble clic al mismo ViewerPage. El snapshot del harness de
previews activa explícitamente el backplate; debe adaptarse a su retirada.

## Resultado esperado y compatibilidad

- Zoom animado habilitado si no existe preferencia previa o falta la clave.
  Respetar animated_zoom=false/true guardado y conservar su control. effects sigue
  condicionando las animaciones; no forzarlo ni cambiar curvas/duraciones.
- Doble clic izquierdo alterna fullscreen en el visor, incluido el superpuesto de
  Waterfall, sin preferencia que pueda deshabilitarlo. Mantener precedencia de
  controles y semántica de doble clic de tira/Board; no hacerlo global a toda UI.
- Retirar Desaturate corner buttons, Toggle fullscreen with double click como
  opción, Invert Colors, Cyberpunk corners y Draw backplate for control panel.
  Eliminar sus controles, acciones, atajos exclusivos C/I, miembros, funciones,
  claves leídas/escritas, defaults del paquete y traducciones exclusivas.
- El botón de cierre conserva el rojo actual del camino no desaturado. La tira
  conserva su contenido/reflejos sin fondo opcional. Conservar superficies de
  Settings/etiquetas, atenuación de Waterfall, tercios G y pixelated zoom P.
- Archivos antiguos con claves retiradas se cargan sin alterar otros campos; al
  guardar dejan de contener esas claves. No reescribir configuración personal
  durante desarrollo ni añadir migración de rutas/formato.

Fuera de alcance: tamaño 90/117, barra centrada, dock Outline, compactación por
clic, hover ampliado y cambio de marca (018–022). Se mantienen thumbnail_width=50
como default actual hasta 018, estructura, enums y formatos persistidos.

## Criterios de aceptación

| ID | Resultado observable | Comprobación |
| --- | --- | --- |
| AC-01 | Sin preferencia, zoom animado activo; preferencias true/false sobreviven carga/guardado | Prueba de Settings aislada y recorrido nativo |
| AC-02 | Opciones/features retiradas sin entradas, atajos o consumidores exclusivos activos | Búsqueda, diff y recorridos de Settings/contextual/teclado |
| AC-03 | Doble clic fullscreen funciona aunque la configuración antigua tenga false | Recorrido Viewer/Waterfall; controles siguen consumiendo primero |
| AC-04 | Claves antiguas ignoradas y omitidas al guardar; demás preferencias conservadas | Round trip en datos sintéticos aislados |
| AC-05 | Build/CTest y contratos de UI/carga existentes conservados | Release, suites y recorridos proporcionales |
| AC-06 | Sin regresión reproducible respecto a las mediciones conservadas de la última modificación | Comprobar candidato frente a referencia histórica identificada, con límites de comparabilidad explícitos; no volver a medir la base |

No quedan preguntas de producto para este alcance. Una prueba de Settings o una
compilación no acredita UX ni rendimiento. No cerrar con criterios necesarios pendientes.
