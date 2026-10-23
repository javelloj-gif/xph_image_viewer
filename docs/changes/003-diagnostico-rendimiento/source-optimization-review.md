# Revisión de oportunidades de optimización conservadora

Seguimiento de implementación: [006](../006-optimizaciones-locales/result.md) completa
OPT-001, OPT-002-tira y el guard de opacidad de OPT-003. Equivalencia y microbenchmark
local registrados allí. [007](../007-layouts-por-cambios/result.md) añade reutilización
de layouts Library/Waterfall con revisión compartida. Índice de visibles masonry,
última medida del overlay y OPT-007 siguen pendientes. OPT-006 implementado en
[011](../011-vigencia-antes-upload/result.md), con callbacks conservados y WARP;
sin mejora temporal global demostrada. OPT-005 implementado
en 010, pendiente de aceptación visual; medición sin aceleración clara.
[008](../008-formatos-texto/result.md) añade formatos DirectWrite privados acotados,
sin cachear layouts/texto ni medidas. [009](../009-decoder-compartido/result.md)
implementa OPT-004 con decoder compartido normal y fallback fresco acotado. El informe de abajo
conserva el estado y evidencia de la auditoría previa, no describe todo el estado actual.

Fecha: 2026-10-22. Prioridad nueva del propietario: analizar funcionamiento y fuentes
para reducir tiempo/recursos conservando experiencia y presentación. Se detiene la
campaña de reproducción de un síntoma como siguiente actividad. No nuevas ejecuciones,
benchmarks ni cambios de fuentes en esta revisión; no porcentajes de mejora prometidos.
Git: ac4a5da, codex/repository-foundation y diff previo R1/R2/003/004/005 preservado.
Habilidad: lenteuv-architecture-review; sin delegación.

## Cobertura y funcionamiento

Revisados los caminos relevantes de App::Initialize, Run, RenderFrame, LoadImage,
LoadFolder; Window::RequestRedraw/WM_PAINT; Renderer de texto/primitivas/presentación;
ThumbnailStrip (reconstrucción, update, dibujo y entrega); FolderDataManager
(AddFolder, selección, sesión y orden); layouts/dibujo/previews de Library/Waterfall;
Viewer de imagen/overlay; ImageLoader (WIC, carga síncrona/async y entrega/upload).
MetadataCache/XmpTagManager solo en su entrada de inicialización. Búsquedas de
consumidores de CreateTextFormat y pixels completan trazabilidad, no auditoría global.
No revisados íntegramente Board/editor/búsqueda, codecs, shell, persistencia ni device
loss de todos los consumidores. Se conservan atribuciones, modos y contratos existentes.

Flujo actual: incluso el emisor secundario inicializa workers/caché antes de consultar
IPC. El primario restaura carpetas antes del argumento explícito y carga la imagen
sincrónicamente. El bucle UI procesa mensajes, actualiza, drena texturas, dibuja y
presenta en cada vuelta; después espera hasta 16 ms si no detecta actividad continua.
Layout, preparación de texto y búsquedas de elementos se repiten dentro de ese flujo.
La optimización buscada elimina trabajo redundante, no reduce contenido ni calidad.

## Reglas comunes de aceptación

Mismos píxeles/orden/crop/interpolación, texto, animaciones, controles, selección,
sesión y favoritos. Recursos gráficos y UI siguen en hilo principal. Preservar tokens
004, duplicados por ocurrencia, estados de error y descartes. Sin modificar valores
persistidos, Library, ACK, elección de instancia ni cierre. Cachés propuestas son
acotadas y con invalidación explícita; su pequeña retención de RAM se declara, no se
presenta como reducción garantizada de todas las métricas a la vez.

Los hallazgos OPT son oportunidades de implementación, no defectos visuales confirmados.
Confianza: repetición/complejidad confirmadas por código; ahorro real no medido.
Prioridad combina frecuencia y alcance, no sustituye profiling. Ningún cambio está
implementado por este informe ni queda garantizada equivalencia solo por inspección.

## OPT-001 — Reconstrucción cuadrática de la tira

- Ubicación: thumbnail_strip.cpp:25, búsquedas :36/:44 y erase :41.
  Consumidores: folder_data_manager.cpp:116 y app.cpp:644–649, además de sort/navegación.
- Evidencia/clasificación: oportunidad algorítmica confirmada. Por ruta se busca en
  previous o en newThumbnails; erase desplaza el resto del vector. Incluso una lista
  nueva sin hits busca en todos los resultados previos. Peor caso O(N²).
- Consecuencia: trabajo de UI y movimiento de strings/ComPtr/tokens crecen más que
  el número de imágenes. El camino LoadFolder también puede reconstruir la tira en
  SetCurrentFolderIndex y otra vez al volver a App::LoadFolder; no elimina por sí solo
  una de las llamadas sin revisar selección, rutas fallidas y efectos de reset.
- Propuesta: índice temporal ruta→cola de ocurrencias antiguas, consumidas en orden,
  y primer hit completado del resultado para duplicados extra. Sin erase por elemento.
  Mantener orden de entrada, texturas, loadFailed y token de cada ocurrencia retenida;
  duplicado nuevo jamás hereda solicitud pendiente de otra ocurrencia. Promedio O(N)
  con hashing, no garantía de peor caso lineal. Alternativa determinista O(N log N).
- Prioridad alta para listas; coste medio. Memoria temporal O(N) a cambio de menos
  búsquedas/movimientos; sin nuevo límite de imágenes o cache persistente.
- Validación: equivalencia de resultado/selección/tokens con cache vacío, orden
  inverso, duplicados y cargas pendientes; conservar 98 checks de 004. Comparar
  operaciones/tiempo de reconstrucción, sin atribuirlo al arranque completo.
- Relación: candidato inicial, compatible con presentación actual. No cancelar loader.

## OPT-002 — Layout completo y recorrido de listas en cada frame

- Ubicación: LibraryPage::DrawPreviewsGrid (:267/:274) y CalculateLayout (:426);
  WaterfallPage::DrawPreviewsGrid (:145/:151) y CalculateLayout (:81).
  ThumbnailStrip::Update (:125) y Draw (:225).
- Evidencia/clasificación: oportunidad confirmada. Grids recalculan todos los rects,
  crean colHeights y buscan columna mínima por elemento aunque nada haya cambiado;
  después recorren todos para descartar fuera de pantalla. La tira ya carga/dibuja
  solo visibles, pero primero recorre N para encontrarlos: no confundirlo con dibujar N.
- Propuesta: conservar el mismo algoritmo de layout y sus resultados mientras no
  cambien carpeta/orden/lista, dimensiones de previews, tamaño de panel o parámetros
  de columnas/espaciado. Scroll aplica offset sin reconstruir base. En tira, calcular
  intervalo visible por su disposición regular y conservar el filtro exacto de borde.
  Para masonry, índice por columnas/intervalos; no asumir alturas uniformes.
- Consecuencia esperada: evitar O(N·columnas) de layout por frame estable; enumerar
  visibles en lugar de N donde el índice lo permita. Coste real pendiente de medir.
- Prioridad alta en grids/listas; coste pequeño para tira, medio para invalidación de
  grids. Los rects ya existen; no duplicar modelo ni crear índices sin límite.
- Compatibilidad/validación: mismas coordenadas y regla de empate de columnas,
  clipping, ampliación al hover, sombras, dibujo del activo al final y selección.
  Invalidar al completar textura que cambie proporción, al ordenar/borrar/cambiar
  carpeta y al resize. Comparar rects y conjuntos visibles en límites fraccionarios,
  drag y carga progresiva. No ocultar ni bajar resolución de miniaturas.
- Relación: primera porción de trabajo repetido; no depende de retirar Library.

## OPT-003 — Preparación repetida de texto e incluso de un overlay invisible

- Ubicación: Renderer::DrawText (:349/:353), MeasureText (:376/:389), CreateTextFormat
  (:513/:518). ViewerPage::DrawOverlayInfo (:378/:413/:420).
- Evidencia/clasificación: oportunidad confirmada. Cada DrawText crea formato;
  MeasureText crea otro formato/layout y copia texto para sustituir espacios. El
  overlay reconstruye su string y lo mide antes de comprobar metadataOpacity>0.
- Propuesta mínima: salir antes de preparar el overlay cuando la condición actual
  impediría dibujarlo; no alterar Update/fade. Conservar último texto/medida mientras
  sus entradas sean iguales. Caché acotada de formatos internos con clave completa
  (familia, tamaño, peso/estilo, locale, alineaciones y demás propiedades efectivas).
  Mantener la fábrica pública de formatos mutables independiente; no compartir un
  formato que un consumidor pueda modificar y contaminar otros dibujos.
- Prioridad media transversal y candidato muy pequeño para el overlay invisible;
  coste pequeño/medio. Caché introduce retención limitada de objetos CPU, no calidad
  inferior. Evitar cachear cada string/frame animado indefinidamente.
- Validación: mismo ancho y rasterizado, Unicode, espacios, DPI/resize, zoom, índice,
  frame animado y modal Waterfall; misma tipografía/alineación. Invalidar por cambios
  efectivos de texto/fuente/factory. No medir ni formatear de nuevo solo por opacidad.
- Relación: candidato inicial sin cambiar cadencia de presentación.

## OPT-004 — Doble apertura de decoder en imágenes estáticas candidatas a animación

- Ubicación: App::LoadImage (app.cpp:571, rama animada seguida de LoadSync);
  ImageLoader::DecodeAnimatedImage (:223/:229) y DecodeImage (:192).
- Evidencia/clasificación: oportunidad confirmada para formatos de la lista animada
  cuyo decoder informa ≤1 frame, por ejemplo PNG estático admitido por ese codec.
  El intento animado abre decoder y consulta frames; devuelve null y LoadSync abre
  otro decoder. No afirmar dos decodificaciones completas de píxeles en ese caso.
- Propuesta: una entrada de carga que abra el decoder una vez y seleccione estática/
  animada conservando la composición/disposal/delays y fallback actuales. Reutilizar
  lo que sea válido en el camino estático; preservar errores/fallback de codecs.
- Prioridad media para aperturas; coste medio. No saltar detección de animación de
  PNG/WebP ni reducir frames/calidad como optimización. Menos aperturas/objetos,
  beneficio de tiempo dependiente de codec/archivo, aún no medido.
- Validación: estáticos, un frame, APNG/GIF/WebP multiframe, transparencias, fallos
  y metadatos; mismos píxeles/frames/duración. Codec no disponible = límite explícito.
- Relación: entrega propia pequeña de loader; no convertir en carga async por defecto.

## OPT-005 — Inicialización pesada antes de reenviar una apertura

- Ubicación: App::Initialize (:85 workers, :92 tags, :118 IPC);
  XmpTagManager::Initialize (xmp_tag_manager.cpp:21/:23, LoadCache).
- Evidencia/clasificación: oportunidad confirmada. El proceso que solo reenviará
  ya carga favoritos, crea cuatro workers/WIC, carga caché XMP y prepara overlays.
- Propuesta: resolver la derivación con prerrequisitos mínimos, y dejar servicios
  costosos para el proceso que continuará como primario. Mantener resultados/errores
  de 005, endpoint y detección; no añadir ACK/mutex/cierre. Reordenar exige seguir
  creación de HWND, callbacks y mensajes iniciales antes de cambiar el arranque.
- Prioridad alta para dobles clics con receptor; coste medio. No asumir que reduce
  el coste de primera apertura sin receptor, ni trasladar una espera al primer gesto.
- Validación: mismo archivo aplicado y mismo receptor; fallos explícitos; arranque
  primario, modos y restauración intactos. Revisar orden de errores/subsistemas.
- Relación: candidato posterior a optimizaciones locales; no cambio trivial ni
  autorización para rediseñar IPC. El portable asociado tiene hash distinto a 005.

## OPT-006 — Upload de resultados que el consumidor ya descartará

- Ubicación: ImageLoader::ProcessPendingTextures (:438/:449/:452), y completions
  de tira/previews con token/ruta implementados por 004.
- Evidencia/clasificación: oportunidad confirmada de orden de trabajo. Se crea
  bitmap GPU antes de invocar callback, donde se descubre que el token ya no existe.
- Propuesta: vigencia opcional comprobada en UI antes del upload; para solicitudes
  caducadas evitar recurso inútil sin suprimir efectos de completado válidos. No
  tocar todos los consumidores con una suposición de cancelación universal.
- Prioridad media en navegación rápida; coste medio, contrato de loader a especificar.
  Evita uploads/asignaciones caducadas, no cancela la decodificación ya realizada.
- Validación: consumidor reemplazado/borrado, misma ruta recreada, entregas repetidas,
  fallo y recursos válidos; cero uploads de obsoletos y efectos existentes preservados.
- Relación: ampliación propia de vigencia, separada de AR-005/cancelación/cierre.

## OPT-007 — Presentación continua cuando el contenido puede estar estable

- Ubicación: App::Run (:253/:274/:289), RenderFrame (:296), Renderer::EndFrame (:250),
  Window::RequestRedraw (:129); OnPaint vacío (app.cpp:879).
- Evidencia/clasificación: oportunidad estructural confirmada. RenderFrame y Present1
  ocurren antes de esperar incluso en reposo. RequestRedraw invalida HWND pero hoy
  no es el contrato que decide si se dibuja. No cuantificar coste GPU solo por esto.
- Propuesta: separar actualización/cargas de dibujar/presentar; dirty y próximos
  vencimientos para contenido que cambia. Mantener cadencia actual mientras haya
  animación, scroll, fade, drag, edición o feedback visible. Despertar UI por resultados
  del loader y mensajes; recursos gráficos siguen en UI.
  Una etapa más conservadora mantendría inicialmente la actualización/sondeo actuales
  y omitiría solo dibujo/presentación sin cambios, después de completar el mapa dirty;
  dormir indefinidamente y nuevos despertares serían otra entrega. No confundir menos
  Present con retrasar callbacks, tick de animación o comprobación de cursor.
- Prioridad estratégica alta, coste grande/transversal. No aceptar simplemente
  bajar FPS, aumentar Sleep o dormir solo esperando WM_PAINT: workers no notifican
  hoy la cola por mensaje y Update sondea cursor/timers. Eso sí puede degradar UX.
- Validación requerida antes de adoptarlo: mapa completo de invalidación de páginas,
  overlays, cursor, auto-hide, vídeo/animaciones y entrega; presentación al restaurar,
  DPI, resize, oclusión y pérdida de dispositivo. Imagen estable no exige frames
  idénticos nuevos, pero toda transición visible debe conservarse.
- Relación: mayor potencial para reposo, no primera intervención bajo la restricción
  de equivalencia visual hasta completar ese mapa. No se considera seguro por inspección.

## Secuencia recomendada y límites

1. Optimización local: overlay invisible/última medida, tira visible y reconstrucción
   con índice, preservando semántica 004. No cambiar algoritmos de dibujo.
2. Layouts por cambios y formatos de texto acotados: mismas entradas, misma salida;
   invalidación completa y memoria de cachés explicitada.
3. Apertura única de decoder y arranque secundario mínimo, con regresiones por contrato.
4. Evaluar scheduler de presentación por cambios con mapa completo; no throttle visual.

No se propone bajar resolución/interpolación, eliminar efectos/animaciones, retrasar
servicios hasta el primer uso sin comprobar su latencia, limitar contenido visible o
liberar pixels sin revisar sus consumidores: ImageData::IsValid (:23) exige pixels.
Retención CPU/GPU no demuestra fuga; el informe no prescribe expulsión agresiva.
Los brushes ya se reutilizan en Renderer::GetBrush; no se inventa una creación por
primitiva. Los pools de workers ya duermen con condition_variable; no hay sondeo ahí.

Conclusión de revisión: sí existe una ruta conservadora basada en trabajo repetido
y algoritmos. No hay evidencia suficiente para prometer un porcentaje o que cualquier
implementación de esas ideas preserve UX. Cada porción debe exigir equivalencia como
aceptación; las hipótesis más amplias no bloquean optimizaciones locales. Sin nuevos
tests/runtime en esta auditoría estática. Próxima especificación: primera porción local
OPT-001/002-tira/003, dividida si hace falta para mantener revisión proporcional.

Seguimiento OPT-005 (2026-10-22): [010](../010-inicializacion-secundaria/spec.md)
define inicialización secundaria mínima y prerrequisitos HWND/callbacks/servicios.
Implementación y medición registradas en [resultado 010](../010-inicializacion-secundaria/result.md);
Release/CTest 5/5 y diez escenarios nativos pasados. Aceptación visual pendiente;
sin reducción clara de latencia en la muestra. No cambia el estado
de OPT-006/007 ni demuestra mejora global.
