# Guía de uso

Estado: **confirmado en código**, salvo las verificaciones concretas registradas en
[validación](validation.md). El texto de ayuda dentro de la aplicación puede contener
atajos heredados. Los overlays y las páginas pueden consumir teclas antes de los
handlers globales.

## Abrir imágenes y cambiar de modo

Ejecuta `lenteuv.exe` con la ruta de un archivo o directorio, o arrástralo a la ventana.
Las carpetas visitadas se guardan en la sesión. La configuración de instancia única
envía aperturas externas al proceso activo; `--multiple` o `--lite` la omiten.

La imagen y la lista de su carpeta se preparan en segundo plano. El original
anterior permanece hasta cargar correctamente el nuevo. Zoom y paneo están
disponibles al aparecer la imagen; la navegación espera la lista de su carpeta.
La tira y Cascada reutilizan automáticamente una caché de miniaturas bajo
`user_data/cache/thumbnails-v1`, sin opciones adicionales ni cambios de calidad.
La primera visita todavía necesita generar las miniaturas desde las fotografías.

| Entrada | Resultado cuando no la consume un overlay/modo |
| --- | --- |
| NUM1 | Ir al visor principal |
| NUM2 o 2 | Reservados, sin acción de navegación; 2 sigue disponible en editores |
| NUM3 o 3 | Cascada |
| NUM4 o 4 | Tablero; al entrar se incorpora la imagen activa si no está ya |
| O o tecla de tilde | Abrir opciones |
| F11 o F | Alternar fullscreen/ventana |
| Doble clic izquierdo | Alternar fullscreen si la opción está activa |
| Escape | Depende del contexto: cierra overlay, deselecciona en Board o sale globalmente |
| T, también Ctrl+T | Etiquetas/metadatos de la imagen objetivo o selección de Board |
| Supr | Papelera para imágenes en los modos que lo ofrecen; en Board elimina ítems del lienzo |

El menú radial aparece en la esquina superior izquierda; clic derecho abre el menú
del modo. La posición `—` permanece gris e inactiva y muestra `Unavailable`. La opción de cierre a bandeja puede ocultar la ventana en vez de terminar
el proceso. Usa la salida de su menú para asegurar cierre completo.

## Visor

- `1` de la fila superior alterna ajuste y tamaño original; NUM1 tiene otra función.
- Rueda hace zoom y Ctrl+rueda navega con `zoom_on_mousewheel=true`; la opción invierte los gestos.
- Arriba/abajo amplían/reducen. Izquierda/derecha navegan, Inicio/Fin van al extremo de la lista.
- Ctrl+clic/arrastre define la región de ampliación; `1` también cancela una región activa.
- Arrastre izquierdo panea en fullscreen y mueve la ventana en compacto según los cambios previos conservados.
- A/D desplazan; `0` o NUM0 restablecen vista. R rota 90°, Shift+R invierte el sentido.
- Espacio pausa/reanuda animación. C alterna marco, G tercios, M información.
- PageUp/PageDown cambian opacidad. Acerca el puntero al borde inferior para la tira de miniaturas.

El modo compacto usa la imagen global para calcular tamaño y contiene un mínimo
de 512; no garantiza ajuste exacto a imágenes pequeñas. En otros modos el tamaño
puede depender de la última imagen abierta.

## Cascada

En Waterfall, scroll recorre las columnas y el clic
abre el visor modal; hacer clic fuera regresa a la cuadrícula. El modal reutiliza el
visor dentro de la misma ventana, sin crear otro proceso.

## Tablero y metadatos

| Entrada | Acción |
| --- | --- |
| Arrastre izquierdo | Selección/movimiento de ítems |
| Arrastre central / rueda | Paneo / zoom del lienzo |
| Alt+clic derecho | Iniciar conexión entre ítems |
| Clic derecho | Menú con nodos, notas y opciones de ítems/conexiones |
| Ctrl+F | Búsqueda de tags |
| Ctrl+L | Escanear tags del directorio para `xmpTags.json`, si el archivo todavía no existe |
| Ctrl+Shift+L | Traer imágenes del directorio al tablero |
| Ctrl+S | Guardar como `!board.kbrd` en la carpeta de imágenes actual |
| T / Ctrl+T | Editar tags de las imágenes seleccionadas |

El tablero guarda posiciones, enlaces y rutas; conservar el archivo no copia las
imágenes originales. Al entrar puede cargar `!board.kbrd` del directorio activo.
Las sugerencias de búsqueda dependen de `xmpTags.json`; un catálogo existente puede
quedar obsoleto. Las consultas de tags combinan condiciones sin distinguir mayúsculas.

Escribir metadatos modifica el archivo de imagen. Usa copias para probar. JPEG y PNG
tienen inyectores propios; el resto depende de WIC/propiedades Windows y puede fallar.

## Opciones e idioma

O abre opciones de opacidad, gestos, animación, arranque y otros campos. Algunas
preferencias solo se guardan y no tienen consumidor funcional identificado; consultar
la [tabla técnica](project-context.md#configuración-valores-y-consumidores).
La interfaz ofrece botones de inglés/ruso; otros catálogos pueden seleccionarse
mediante `ui_lang` en configuración. No equiparar idiomas del instalador con botones
disponibles dentro de la aplicación.
