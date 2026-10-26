# Tres propuestas del visor

Estado: diseño completado y elegido por el propietario el 2026-10-26.
Dock compacto y familia de iconos Outline; barra informativa centrada sobre la tira;
miniatura activa 117 DIP frente a base 90 DIP (+30 %), borde cian sin rótulo.
Referencia aprobada: [Outline](concepts/outline.png) para las seis acciones.
No hay implementación en C++. Los iconos del primer mockup quedan como exploración
histórica; la elección de Outline fija la dirección visual de la implementación.
Las medidas siguientes son DIP nativos propuestos; el preview responsive reduce
espacios y cantidad de miniaturas para representar una ventana pequeña.

## Lenguaje visual y comportamiento común

Superficies oscuras, borde gris fino, sombra discreta, iconos lineales planos de
20 DIP y acento cian. Sin nuevas animaciones continuas, blur dinámico ni rediseño
del renderer. El fondo del preview y el paisaje son sintéticos: no es una captura
de producto. No decide cambios en transparencia/opacidad de la ventana nativa.

| Acción | Referencia visual del icono | Estado |
| --- | --- | --- |
| Waterfall | Tres columnas de imágenes (`columns-3`) | Acción |
| Board | Distribución de rectángulos (`layout-dashboard`) | Acción |
| Rotate 90° | Flecha circular horaria (`rotate-cw`) | Acción sobre imagen |
| Tags (T) | Etiquetas superpuestas (`tags`) | Activo al mostrar su editor |
| Sort & Filter | Líneas descendentes con filtro (`list-filter`) | Deshabilitado; futuro |
| Settings | Engranaje (`settings`) | Activo al mostrar opciones |

Los iconos del preview son referencias Lucide; no añaden una dependencia C++.
Reproducir el lenguaje mediante primitivas/recursos nativos tras la elección,
verificando licencias si se incorporan assets de terceros.

Panel anclado a 12 DIP del borde superior/izquierdo, activación en franja superior
izquierda de 48 DIP de alto sobre su ancho. Aparece al acercar el puntero y permanece
mientras haya hover/foco o un overlay iniciado desde él. Ocultar tras 900 ms fuera
de esas áreas, con transición de 180 ms; respetar la opción general de efectos.
Los botones mantienen objetivos de al menos 44 DIP. El panel oculto no intercepta
clics. Fuera de los visores no hay panel; sus menús contextuales ofrecen Viewer.

Reposo: icono/texto claro. Hover/foco: superficie ligeramente más clara y borde.
Activo: borde y texto cian; solo los controles con estado persistente muestran ese
estado en producto. Deshabilitado: gris, sin acción. La selección temporal de botones
del mockup sirve para comparar el tratamiento activo; no define nuevas semánticas.

## Exploración histórica: barra horizontal

Seis botones en una fila de aproximadamente 660 × 58 DIP, con iconos y nombres
visibles. Prioriza identificación y un recorrido corto del puntero.
En anchos menores de 730 DIP: tres columnas/dos filas; bajo 440: dos columnas/tres
filas. Barra informativa centrada, 12 DIP por encima de la envolvente real de la tira.

No elegida: los seis comandos son reconocibles sin memorizar iconos, pero el
propietario prefirió el dock compacto.

## Exploración histórica: panel de dos filas

Tres columnas/dos filas, aproximadamente 398 × 152 DIP. Cada celda muestra icono
arriba y texto debajo; bajo 440 DIP usa dos columnas/tres filas con icono al lado.
Barra informativa alineada a la izquierda sobre la tira. Reduce ancho ocupado pero
cubre más altura del visor que la barra horizontal.

## Dock compacto

Seis iconos en una fila de aproximadamente 288 × 58 DIP. Nombre completo en tooltip
al pasar el puntero o dar foco; cada control conserva nombre accesible. Sort & Filter
se identifica como función futura sin activarse. La primera exploración mostraba
la barra informativa a la derecha; la elección final la centra sobre la tira,
alineada con la imagen activa. Minimiza superficie ocupada y exige aprender iconos.

## Miniaturas y metadatos

Elección aprobada: base predeterminada de 90 DIP, activa a 117 DIP (30 % mayor), borde cian de 2 DIP
y opacidad plena. Las otras mantienen el tratamiento atenuado actual. La marca
de multiselección usa check independiente; hover no sustituye la marca de imagen
abierta. No mostrar rótulo permanente Viewing: tamaño y borde identifican la imagen
abierta. El texto de los previews era una ayuda de evaluación y no forma parte del
diseño aprobado. No aumentar resolución de decode ni variantes de caché por estética.

En nativo, calcular envolvente de tira y barra según geometría real, no multiplicar
solo el desplazamiento por una constante de altura. Mantener al menos 12 DIP entre
barra y miniatura activa; acotar el ancho de texto al cliente, truncar nombre largo
con acceso al texto completo y preservar dimensiones/zoom. No mover el editor T.
Se compararon barra centrada/izquierda/derecha y tamaños activos 108/117/126 DIP.
El propietario eligió centrada y 117 DIP. Las preferencias de tamaño guardadas
siguen respetándose según el mapa del lote; 90/117 describen la configuración base.

## Elección final — 2026-10-26

Composición elegida: Dock compacto. Iconos elegidos: propuesta 1, Outline.
Barra informativa centrada sobre la tira, alineada con la imagen activa; tamaño
activo 117 DIP (+30 % respecto a base 90 DIP), borde cian y sin rótulo permanente.
Estos criterios se trasladarán a 018/020. 019 mantiene siempre el mismo visor
completo y retorno a Waterfall. No quedan decisiones de elección visual pendientes.

## Exploración de iconos con generación de imágenes

Las tres familias conservan las seis acciones y el dock horizontal compacto; cada
lámina muestra iconos ampliados, labels en inglés y aplicación pequeña en el dock.

Láminas conservadas: [Outline](concepts/outline.png), [Soft Solid](concepts/soft-solid.png)
y [Technical Duotone](concepts/technical-duotone.png). Estas imágenes son conceptos
de diseño, no recursos cargados por la aplicación.

- **Outline:** trazos redondeados, ligero, columnas de fotografías para Waterfall,
  tarjetas libres para Board, giro horario, doble etiqueta, líneas/filtro y engranaje.
- **Soft Solid:** siluetas sólidas redondeadas con recortes negativos; mayor peso
  visual y los mismos significados, manteniendo espacios claros entre formas.
- **Technical Duotone:** geometría angular blanca/cian, mosaico para Waterfall,
  tarjetas sobre lienzo para Board; sliders de ajuste como alternativa al engranaje.

Generación con herramienta integrada image_gen, sin CLI ni API externa adicional.
Prompt común: lámina de diseño UI, fondo carbón, glifos legibles a 20–24 px,
objetivos de 44 px, seis acciones exactas, estados default/hover/active/disabled,
Sort & Filter atenuado, sin marca definitiva, brillo neón ni 3D. Prompts específicos:
monolínea redondeada para Outline; masas planas con vacíos para Soft Solid; contornos
angulares y planos cian para Technical Duotone. Son propuestas raster de evaluación,
no sprites finales ni autorización para sustituir el render nativo.
