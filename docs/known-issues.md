# Problemas conocidos y asuntos pendientes

La reforma de estructura no pretende corregir la lógica funcional. Las siguientes
entradas son puntos de partida verificables para futuras tareas, no un diagnóstico
completo ni una lista de fallos reproducidos visualmente.

| Área | Evidencia y alcance | Siguiente comprobación |
| --- | --- | --- |
| Instancias y responsividad | **Usuario:** problemas por detallar. **Código:** sondeo de pipe sin elección por mutex; cargas principales síncronas | Reproducir aperturas simultáneas, rutas desde Explorer y cierre con cola ocupada |
| Apertura IPC del portable | **Ejecución:** la segunda instancia termina con código 0, pero no cambió la imagen activa (BMP y PNG). El binario previo también falló al recibir JPEG. Las fuentes C++ de IPC/visor son idénticas al commit de preservación | Diagnosticar con trazas y pruebas de entrega; no certificar recepción solo por el exit code del cliente |
| Carpetas grandes | **Código:** enumeración síncrona; previews y listas por carpeta; no hay presupuesto global RAM/VRAM | Medir 10k–50k imágenes, scroll, latencia y memoria con tamaños/códecs conocidos |
| Compacto fuera de Viewer | **Usuario/Código:** tamaño depende de imagen global o queda previo si no existe; mínimo 512 | Definir criterio de producto por modo antes de cambiarlo |
| Estado y callbacks | **Código:** acceso `friend`, transformaciones duplicadas e índices capturados en cargas | Revisar cambios rápidos de carpeta, ordenación y borrado con cargas pendientes |
| Animaciones/memoria | **Código:** frames y bytes CPU permanecen al subir a GPU; tablero limita número de texturas, no bytes | Medir animaciones grandes y revisar presupuesto/expulsión |
| Cierre | **Código:** loader drena la cola y escáneres hacen join | Medir cancelación y latencia al salir durante carga/escaneo |
| Formatos opcionales | **Código:** lista por extensión depende de decodificadores WIC | Probar SVG/WebP/AVIF/HEIF/APNG en equipos con/sin códecs |
| Escritura de metadatos | **Código:** temporal, borrado y rename; fallos/interrupciones pueden afectar originales | Diseñar reemplazo seguro y preservación de metadatos no editados, con tests sobre copias |
| Catálogo de tags | **Código:** Ctrl+L omite escaneo si existe `xmpTags.json` | Definir actualización e invalidación; no asumir índice siempre vigente |
| Opciones sin consumidor | **Código:** campos persistidos sin uso funcional encontrado | Revisar `debug_page`, `do_not_show_start_dialog`, `run_on_windows_startup`, `show_console_output`, `show_noise_cells` |
| Idioma | **Código:** botones EN/RU; otros catálogos existen, pero no hay botones para ellos | Definir exposición de idiomas y cobertura de traducciones nativas |
| Persistencia | **Código:** resolución por ascendentes y formatos propios sin validación exhaustiva | Tratar futura migración a AppData y evolución binaria como tareas explícitas |
| Instalación protegida | **Código:** datos junto al EXE; destino histórico Program Files | Comprobar permisos con usuario estándar; no asumir que compilar instalador resuelve escritura |
| Versiones | **Código:** CMake/paquetes 0.90.0 y recurso RC 1.00.3.0 | Unificar política de versión en tarea posterior |
| Ayuda/atajos | **Código:** varias etiquetas de ayuda heredadas difieren de handlers; 1 y NUM1 tienen funciones distintas | Mantener guía basada en handlers y revisar textos en una tarea funcional |

## Auditoría de arquitectura

La [auditoría inicial de arquitectura de 2026-10-22](changes/001-refactorizacion-base/review.md)
añade siete hallazgos estáticos sobre servicios compartidos, callbacks, IPC, resolución
de datos, cierre, caché binaria y comentarios. Su cobertura es parcial y no incluye
reproducción nueva. El [plan de refactorización](changes/001-refactorizacion-base/plan.md)
registra el destino de cada hallazgo; no se consideran corregidos ni se atribuyen
automáticamente al arranque lento reportado.

## Información rescatada de los planes anteriores

- Se conservó el trabajo local de IPC, confirmación de borrado y gesto de ventana/paneo
  en un commit previo a la reforma. El plan antiguo hablaba de arrastre derecho;
  el código más reciente usa izquierdo. No reimplementar el texto histórico encima de él.
- Los informes de fallos portables motivaron runtime VC++ app-local, escritura sin BOM
  y evitar cargar imágenes antes de inicializar renderer/páginas. Estos antecedentes
  orientan regresiones, pero no prueban causa de cualquier fallo futuro.
- Los planes de sesión eran propuestas y contenían afirmaciones excesivas sobre el
  alcance de checks PE. El nuevo empaquetado distingue imports estáticos y ejecución real.
- `deleted.po` contenía solo el mensaje heredado `Over user points`, sin consumidor
  en las fuentes nativas; no se perdió una traducción vigente al retirarlo.

Registrar nuevos problemas con pasos, versión/commit, entorno y evidencia. Separar
los defectos previos de regresiones de la reforma en [validación](validation.md).
