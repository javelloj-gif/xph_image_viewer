# Guía de uso

Estado: **confirmado en código**, salvo las verificaciones concretas registradas en
[validación](validation.md). El texto de ayuda dentro de la aplicación puede contener
atajos heredados. Los overlays y las páginas pueden consumir teclas antes de los
handlers globales.

## Abrir imágenes y cambiar de modo

Ejecuta `image_viewer.exe` con la ruta de un archivo o directorio, o arrástralo a la ventana.
Las carpetas visitadas se guardan en la sesión. La configuración de instancia única
envía aperturas externas al proceso activo; `--multiple` o `--lite` la omiten.

La imagen y la lista de su carpeta se preparan en segundo plano. El original
anterior permanece hasta cargar correctamente el nuevo. Zoom y paneo están
disponibles al aparecer la imagen; la navegación espera la lista de su carpeta.
La tira y Cascada reutilizan automáticamente una caché de miniaturas bajo
`user_data/cache/thumbnails-v1`, sin opciones adicionales ni cambios de calidad.
La primera visita todavía necesita generar las miniaturas desde las fotografías.

Desde 018, la tira del visor usa 44 DIP si no hay tamaño guardado. La imagen abierta
aparece un 30 % mayor (57,2 DIP con base 44), con borde cian; hover y selección múltiple
son independientes. La selección lleva un check; la separación es de 3 DIP. El nombre/dimensiones/zoom se
muestran en una barra sobre la tira con la fuente original de 13 unidades cliente. Los nombres largos se truncan; al pasar el
puntero se muestra el nombre completo y la rueda lo desplaza si el tooltip no cabe.
La barra y tira permanecen estables mientras se consultan. La burbuja se desplaza
suavemente al aparecer/desaparecer la tira y conserva después su ocultamiento por inactividad. M conserva el control de la información. En Waterfall se
comparte la misma composición de imagen, tira y metadatos desde 019.

| Entrada | Resultado cuando no la consume un overlay/modo |
| --- | --- |
| NUM1 | Ir al visor principal |
| NUM2 o 2 | Reservados, sin acción de navegación; 2 sigue disponible en editores |
| NUM3 o 3 | Cascada |
| NUM4 o 4 | Tablero; al entrar se incorpora la imagen activa si no está ya |
| O o tecla de tilde | Abrir opciones |
| F11 o F | Alternar fullscreen/ventana |
| Doble clic izquierdo | Alternar fullscreen en la imagen de Viewer y su modal en Waterfall; controles/overlays preceden y el fondo transparente compacto queda excluido |
| Escape | Depende del contexto: cierra overlay, deselecciona en Board o sale globalmente |
| T, también Ctrl+T | Etiquetas/metadatos de la imagen objetivo o selección de Board |
| Supr | Papelera para imágenes en los modos que lo ofrecen; en Board elimina ítems del lienzo |

Viewer, Waterfall y Board ofrecen un dock compacto Outline centrado arriba.
Acerca el puntero a la franja superior (48 DIP) o pulsa Tab. Viewer muestra
Waterfall, Board, Rotate 90°, Tags, Sort & Filter y Settings. En Waterfall, la primera
acción pasa a ser Viewer; en Board aparecen Viewer y Waterfall, y Rotate/Sort & Filter
están bloqueados. Tags en Board utiliza imágenes seleccionadas; en la cuadrícula
Waterfall, Rotate/Tags requieren abrir el visor modal. El icono Viewer vuelve al visor principal.

Tab/Shift+Tab y flechas recorren acciones; Enter/Espacio activan y Escape abandona
el foco. El dock permanece con hover/foco/overlay y se oculta tras 900 ms fuera,
con fade de 180 ms si effects está activo. Los modales tienen precedencia.
El radial sigue retirado. NUM1 permite volver al visor.

La opción de cierre a bandeja puede ocultar la ventana; usa la salida de su menú
para asegurar cierre completo.

## Visor

- `1` de la fila superior alterna ajuste y tamaño original; NUM1 tiene otra función.
- Rueda hace zoom y Ctrl+rueda navega con `zoom_on_mousewheel=true`; la opción invierte los gestos.
- Arriba/abajo amplían/reducen. Izquierda/derecha navegan, Inicio/Fin van al extremo de la lista.
- Ctrl+clic/arrastre define la región de ampliación; `1` también cancela una región activa.
- Arrastre izquierdo panea en fullscreen y mueve la ventana en compacto según los cambios previos conservados.
- A/D desplazan; `0` o NUM0 restablecen vista. R rota 90°, Shift+R invierte el sentido.
- Espacio pausa/reanuda animación. G alterna tercios, P interpolación pixelada, M información. C/I ya no tienen acción exclusiva en el visor.
- En Viewer transparente fullscreen, el primer clic simple exterior a los límites transformados de la imagen vuelve a compacto. No se reenvía a otra aplicación; otro clic fuera de la ventana resultante lo recibe Windows normalmente. El fondo compacto no vuelve a fullscreen; la transparencia dentro del bitmap sigue contando como imagen. Controles, overlays, modificadores y gestos activos tienen precedencia.
- PageUp/PageDown cambian opacidad. Acerca el puntero al borde inferior para la tira de miniaturas.

El modo compacto usa la imagen global para calcular tamaño y contiene un mínimo
de 512; no garantiza ajuste exacto a imágenes pequeñas. En otros modos el tamaño
puede depender de la última imagen abierta.

## Cascada

En Waterfall, scroll recorre las columnas y el clic
abre el visor modal completo, incluida tira y metadatos. Escape o un clic exterior
libre regresan a la misma carpeta y scroll. Tira, dock y overlays consumen primero
sus entradas, sin cerrar el modal al utilizarlos. Reutiliza imagen, estado y loader
dentro de la misma ventana. El hover aumenta el ancho unos 66,7 píxeles cliente,
con centro y proporción conservados.

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

El zoom animado está activo sin preferencia previa; respeta `animated_zoom` guardado y `effects`. Las flechas arriba/abajo conservan su camino de animación propio.

O abre opciones de opacidad, gestos, animación, arranque y otros campos. Algunas
preferencias solo se guardan y no tienen consumidor funcional identificado; consultar
la [tabla técnica](project-context.md#configuración-valores-y-consumidores).
La interfaz ofrece botones de inglés, ruso y Español (ES), con cambio inmediato y
persistencia; otros catálogos pueden seleccionarse
mediante `ui_lang` en configuración. No equiparar idiomas del instalador con botones
disponibles dentro de la aplicación.

## Ordenar, filtrar y enviar al tablero (026–030)

Sort & Filter abre el modal de orden por nombre, modificación o tamaño, con dirección
ascendente/descendente. Las casillas de tipos se combinan; Todos activa o desactiva
todas. Otros agrupa formatos admitidos menos habituales. No se añade ningún códec.
Aplicar confirma y guarda; Cancelar/Escape descartan el borrador. Tab/flechas recorren
los controles y Enter/Espacio activan. Viewer y Waterfall comparten preferencias;
Board no permite cambiar orden/filtro desde el dock.

La lista anterior sigue utilizable mientras se prepara el cambio. Si la imagen
abierta queda excluida, permanece visible sin marcar otra miniatura: siguiente/previa
van al primer/último resultado. Sin coincidencias aparece un mensaje y no se navega.
Volver a Todos recupera el inventario; los originales nunca se eliminan al filtrar.

El menú contextual del visor y su modal contiene Eliminar, Regla de los tercios,
Etiquetas, Enviar al tablero, Configuración y Salir, en ese orden. Enviar incorpora la imagen
abierta una sola vez al tablero de trabajo, sin cambiar de pantalla. Las notas y
envíos se conservan al entrar; Ctrl+S mantiene el guardado manual actual. Los menús
especializados de Board y de la cuadrícula Waterfall conservan sus operaciones.
