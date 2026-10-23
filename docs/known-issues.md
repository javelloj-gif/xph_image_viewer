> Estado vigente 2026-10-23: [014](changes/014-cancelacion-ar005/result.md) completa AR-005 (consumidores, WIC cooperativo caracterizado, escáneres y cliente IPC). AR-006 corregido en 013. AR-007 preexistente sigue progresivo. Las campañas históricas citadas abajo no sustituyen estos estados actuales.

# Problemas conocidos y asuntos pendientes

La reforma de estructura no pretende corregir la lógica funcional. Las siguientes
entradas son puntos de partida verificables para futuras tareas, no un diagnóstico
completo ni una lista de fallos reproducidos visualmente.

| Área | Evidencia y alcance | Siguiente comprobación |
| --- | --- | --- |
| IPC-014-01: nueva imagen no visible (cerrado por reporte) | **Reporte posterior del propietario, 2026-10-23, al aceptar 002:** IPC opera correctamente y el visor se actualiza al abrir una nueva imagen desde Windows. Se conserva el fallo histórico del portable 014, sin causa confirmada. [Evidencia y estado](changes/014-cancelacion-ar005/user-validation.md) | **Cerrado por funcionamiento actual reportado.** Sin investigación ni cambio de código en esta sesión; no se atribuye una corrección a 002 ni se certifican todos los escenarios IPC |
| Instancias y responsividad | **Usuario:** problemas por detallar. **Código:** sondeo de pipe sin elección por mutex; cargas principales síncronas | Reproducir aperturas simultáneas, rutas desde Explorer y cierre con cola ocupada |
| Apertura IPC | **003 histórico:** cinco clientes fallaban con error 2 tras detectar receptor. **005:** reintento de apertura, resultado explícito y payload RAII; regresiones correctas. **Validación real posterior:** PNG presentado y error Win32 231 con salida 2 | Completar Explorer/asociaciones y aclarar AR-008; no hay ACK ni elección atómica y no se atribuye toda la historia a una causa |
| Captura/presentación (AR-008) | **Campaña aislada:** seis procesos capturaron JPEG sin F11/resize; negros en los tres tipos de entrada. Oclusión en una captura y usuario viendo paisaje estable durante último contraste negro | Limitación de captura constatada; no fallo persistente de --lite reproducido. No corregir renderer por este indicio; reabrir con negro/parpadeo nativo |
| Carpetas grandes | **Código:** enumeración síncrona; previews y listas por carpeta; no hay presupuesto global RAM/VRAM | Medir 10k–50k imágenes, scroll, latencia y memoria con tamaños/códecs conocidos |
| Compacto fuera de Viewer | **Usuario/Código:** tamaño depende de imagen global o queda previo si no existe; mínimo 512 | Definir criterio de producto por modo antes de cambiarlo |
| Estado y callbacks | **Código/ejecución:** 004 corrige los índices capturados en tira/previews con tokens; quedan otras áreas de estado/hover sin auditoría completa | Conservar regresiones de entrega diferida; revisar otros consumidores por su propio alcance |
| Animaciones/memoria | **Código:** frames y bytes CPU permanecen al subir a GPU; tablero limita número de texturas, no bytes | Medir animaciones grandes y revisar presupuesto/expulsión |
| Cierre / AR-005 | **014 completado:** leases por consumidor, parada cooperativa WIC/escáneres, catálogo transaccional, cliente overlapped sin flush bloqueante y join antes de HWND/dependencias. CTest 7/7 y trece procesos de cierre | Una llamada WIC/E/S ya iniciada debe retornar; límite caracterizado, sin interrupción forzada ni plazo universal. No quedan implementaciones de AR-005 diferidas |
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

Seguimiento: AR-004 se resolvió en R1 y AR-001 en R2; AR-002 se corrigió en
[004](changes/004-vigencia-miniaturas/result.md). AR-003 se corrige en el alcance de
[005](changes/005-entrega-ipc/result.md); AR-005/006 y la cobertura
documental restante siguen pendientes. La nueva regresión reprodujo asociación
errónea de resultado en la tira, no un crash ni un problema de rendimiento.

Actualización 2026-10-23: [inventario acumulado 004–011](changes/001-refactorizacion-base/coverage-004-011.md)
reconcilia cobertura. [012](changes/012-cierre-colas-ipc/result.md) implementa la primera
porción AR-005: admisión/cola al cerrar, servidor cancelable, payloads y orden antes de
destruir HWND/consumidores. Release/CTest 6/6 y diez escenarios nativos pasados. No
declara resueltos WIC activo, otros scanners ni cancelación por consumidor; AR-006/007
siguen separados. Paquete temporal 012 listo para aceptación funcional del usuario.

003 añadió diagnóstico IPC con reproducción nueva de AR-003, no su corrección; la
matriz y los límites de imagen aplicada/presentación figuran en su [resultado](changes/003-diagnostico-rendimiento/result.md).
005 convierte esa matriz en regresión de aplicación exacta y error explícito, sin
ACK, elección atómica ni rediseño de cierre. No se midió rendimiento.

La [validación real posterior a 005](changes/003-diagnostico-rendimiento/real-validation.md)
confirma PNG presentado tras IPC y diálogo Win32 231/salida 2 reales, con ventanas
independientes de ambos flags y límite AR-008. Añade CPU/memoria en reposo con logging
activo, sin GPU/VRAM ni comparación. Primera ejecución real y Explorer siguen pendientes.

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
