> Actualización 2026-10-23: [014](changes/014-cancelacion-ar005/result.md) completa AR-005 con cancelación cooperativa por consumidor/WIC/escáner y envío IPC con presupuesto total. El hilo conserva sus dependencias hasta completar la operación activa; no se fuerza ni detacha. La base está lista para nuevos cambios funcionales.

# Contexto técnico de LenteUV

**Reporte funcional posterior, 2026-10-23:** en portable 014, abrir otra imagen
mediante el mismo ejecutable mantiene la anterior en el visor (punto 6 IPC).
El propietario lo identifica como error legado y difiere expresamente su
tratamiento. [IPC-014-01](changes/014-cancelacion-ar005/user-validation.md) es
evidencia de usuario, sin causa ni reproducción adicional confirmadas aquí.
Las pruebas aisladas y el cierre de implementación AR-005 no certifican la
aceptación global de IPC. No investigar ni corregir este caso hasta nueva instrucción.

Última revisión: 2026-10-22 (servicios R1/R2, contratos de carga y optimizaciones 006..009). Audiencia: agentes y desarrolladores que mantengan este repositorio.

Estructura vigente: `src/` contiene el código, la raíz contiene CMake y `tests/fixtures/` conserva las muestras. Los builds y paquetes se generan en `build/` y `dist/`.

## Cómo interpretar este contexto

- **Código**: comportamiento confirmado leyendo la implementación; no implica validación visual.
- **Ejecución**: comprobado con un comando o prueba; consultar el alcance exacto en [validación](validation.md).
- **Usuario**: intención o problema reportado por el propietario; puede diferir del código.
- **Pendiente**: hipótesis o comprobación todavía necesaria.

El código es la fuente de verdad del comportamiento actual. Los planes antiguos no prueban que un cambio esté implementado. Leer primero [AGENTS.md](../AGENTS.md), después este documento y la guía específica de la tarea.

## Producto, procedencia y plataforma

**Usuario:** visor inspirado en Picasa, basado en el proyecto de Sergei Krumas y migrado posteriormente a C++ mediante asistencia de agentes. Este fork se presenta como LenteUV; las cabeceras atribuyen sus extensiones a xPhang.

**Código:** aplicación de escritorio exclusiva de Windows. C++20, namespace `kv`, CMake y MSVC. El proyecto declara versión 0.90.0 y Windows 10 1903+ (18362); comprobar disponibilidad efectiva de las interfaces DirectX en cada sistema objetivo. `LICENSE` contiene GNU GPL versión 3; conservar atribuciones y no deducir otra licencia de comentarios antiguos del instalador. El recurso de versión Windows conserva una numeración histórica distinta, registrada como deuda técnica.

No hay dependencia de Qt ni de un runtime Python para ejecutar o compilar el visor. Python/Pillow solo regeneran el ICO; CairoSVG es alternativa para renderizar su fuente SVG. Los formatos admitidos por extensión no garantizan que el sistema tenga un decodificador WIC compatible.

## Arquitectura y mapa de intervención

**Código:** `App` es un coordinador central, no una arquitectura MVC estricta. Posee los servicios y las páginas por valor; varias páginas guardan `App*` y acceden a miembros privados mediante `friend`. Hay estado de transformación tanto en `App` como en `ViewerPage`: seguir el camino activo antes de modificar zoom o paneo.

| Área bajo `src/` | Responsabilidades y puntos de entrada |
| --- | --- |
| `main.cpp`, `common/` | `wWinMain`, captura SEH, COM RAII, tipos geométricos, temporizadores y logs |
| `core/app.*` | Arranque, enrutamiento, modo activo, apertura, navegación, lista actual, borrado y ajuste de ventana |
| `core/folder_data_manager.*` | Modelo compartido de carpetas/previews, selección, escaneo síncrono, sesión/favoritos e historial; App lo posee como `m_folderData` |
| `core/window.*` | HWND, estilos/transparencia, DPI, fullscreen, captura de ratón y traducción de mensajes a callbacks |
| `core/single_instance_ipc.*` | Cliente/servidor named pipe, envío de rutas y cierre del hilo servidor |
| `core/settings.*` | Preferencias y parser/escritor JSON propio |
| `rendering/` | Dispositivo D3D11, DXGI, Direct2D, DirectWrite, DirectComposition, curvas y animaciones por nombre |
| `loader/` | `ImageLoader`, decodificación WIC, trabajadores, colas de texturas y `FrameSequencer` |
| `viewer/`, `ui/thumbnail_strip.*` | Imagen principal, transformaciones, zoom de región y tira inferior |
| `waterfall/` | Distribución en columnas, desplazamiento y visor modal dentro de la misma aplicación |
| `board/` | Tablero, selección, edición, conexiones, serialización, búsqueda, escaneo y distribución |
| `metadata/` | Fachada XMP, caché, escaneo asíncrono y lectores/escritores por formato |
| `ui/options_menu.*`, `ui/tagging_modal.*` | Overlays de opciones y etiquetas/metadatos |
| `i18n/`, `shell/` | Traducciones `.po`, bandeja, integración con Explorer, inicio de Windows y Papelera |
| `resources/` | ICO, recurso RC y manifiesto Windows |

Patrones observados: composición de componentes, callbacks `std::function`, RAII con `ComPtr`, propiedad compartida de imágenes con `shared_ptr`, productores/consumidor para cargas y fachada `XmpTagManager`. `BoardPage` delega a `BoardCanvas`, `BoardSelectionManager`, `BoardTextEditor`, `BoardSerializer`, `BoardSearchManager`, `BoardTagScanner` y `BoardLayoutHelper`. No añadir una nueva jerarquía abstracta para una simple reorganización.

`FolderDataManager` reside en core y se usa en el hilo UI. App mantiene referencias
`m_folders`/`m_currentFolderIdx` a ese estado; mover el servicio no las convierte en
copias. La API sigue combinando navegación y persistencia: AddFolder escanea y ordena
sincrónicamente, y normalmente selecciona/carga/guarda; `restoringSession=true` suprime
selección y guardado para restaurar sesión. El gestor/modelo compartido permanece
en core tras retirar la página en 002.

## Arranque, entrada, renderizado y cierre

**Código:** `wWinMain` consulta el flag de diagnóstico temprano; `wWinMainImpl` establece DPI Per-Monitor V2, inicializa COM, convierte argumentos y crea `App`. `ParseCommandLine` admite archivo/carpeta y `--frameless`, `--windowed`, `--opaque`, `--maximized`, `--fullscreen`, `--multiple`/`--lite` (también con un guion). Las dos últimas alternativas desactivan el control de instancia para ese proceso; no confundir `--lite` con una implementación independiente de visor.

`App::Initialize` carga settings/traducciones y crea HWND oculto; instala un callback temporal propietario de rutas tempranas y resuelve IPC. Solo si continúa inicializa gestor/favoritos, loader de cuatro trabajadores, fachada XMP y overlays; registra bandeja/callbacks restantes y crea el renderer; muestra la ventana según preferencias; inicializa las páginas y restaura sesión antes de cargar el argumento explícito. El orden importa: una carga prematura antes del renderer/páginas produjo problemas históricos de arranque. Sin argumentos ni carpetas restauradas se intenta el directorio de trabajo. Tras contenido inicial y timer instala el callback IPC definitivo y aplica rutas retenidas; los secundarios evitan los servicios pesados y conservan errores 005.

`Page` conserva valores persistidos: Start=1, Viewer=2, Board=3, Waterfall=5; el valor retirado 4 no tiene enumerador. Los atajos de cambio de modo NO corresponden directamente a esos valores. NUM2/2 quedan reservados y la posición radial — / Unavailable está inactiva. No hay diálogo ni migración añadidos para preferencias antiguas; 4 ya no designa un modo soportado. Start permanece en el enum/opciones, pero `DrawViewerContent` solo dibuja las tres páginas funcionales.

`App::Run` vacía mensajes mediante `PeekMessageW`, actualiza/renderiza y espera hasta 16 ms cuando no considera necesaria animación continua. No es un renderer que duerma indefinidamente hasta invalidación. `RenderFrame` limita `dt`, actualiza animaciones y página, procesa cargas pendientes, dibuja el contenido, overlays y menú radial, y presenta. `WindowEvents` convierte mensajes nativos en llamadas a `App`; los handlers ofrecen primero eventos a overlays y páginas. Un handler puede consumir una tecla antes del switch global.

`SingleInstanceIpc` usa `\\.\pipe\lenteuv`, rutas UTF-8 y un hilo servidor que entrega la apertura mediante mensaje a la ventana (`WM_USER + 4501`). La comprobación inicial hace un sondeo de la pipe; no hay un mutex de elección de propietario. No asumir ausencia de carreras entre arranques simultáneos. El receptor normaliza la ruta, distingue carpeta/archivo, cambia de página, restaura y solicita primer plano. Una ruta vacía se ignora.

La instrumentación de 003 permite consultar GetDiagnostics por objeto: detección,
escritura/error, recepción/post y escucha (snapshot atómico por campo, no transaccional).
El handler validación/apertura reside en App::OnIpcOpenPath, extraído sin cambiar su cuerpo.
El diagnóstico 003 reprodujo error 2 tras el sondeo en cinco aperturas. La corrección
[005](changes/005-entrega-ipc/result.md) reintenta apertura busy/missing durante 2000 ms
y distingue Primary, Forwarded y ForwardFailed; busy ya cuenta como receptor detectado.
Otros errores del sondeo también retornan fallo, sin iniciar servidor. App retorna
ForwardFailed y main muestra error Win32 con salida 2; Forwarded lleva a AlreadyRunning
y salida 0, sin ACK de aplicación. La ruta vacía sigue sin provocar apertura. El timeout
desde 014 cubre conexión y escritura overlapped en el mismo presupuesto; no hay FlushFileBuffers síncrono. Al expirar una escritura pendiente se cancela y observa completion antes de liberar almacenamiento. Se conservan errores busy/missing de conexión y ERROR_TIMEOUT en escritura. PostMessage transfiere propiedad
solo al tener éxito; ante fallo el payload se destruye por RAII y se registra error.
La pipe privada de tests no se expone como opción de producto.

**Ejecución posterior:** [validación real de 003](changes/003-diagnostico-rendimiento/real-validation.md)
observó PNG presentado tras IPC, ventanas completas de --multiple/--lite y diálogo
Win32 231 con salida 2 de wWinMain. --lite mostró capturas negras iniciales hasta F11:
indicio AR-008 pendiente de separar captura/oclusión de dibujo real. Existe una muestra
CPU/memoria con logging activo, sin GPU/VRAM ni comparación. No primera ejecución real.

Seguimiento [AR-008](changes/003-diagnostico-rendimiento/ar008-reproduction.md): seis
arranques secuenciales sin/con flags mostraron JPEG en capturas sin F11/resize.
Capturas negras no exclusivas de --lite y una captura ocluida; el usuario informó
paisaje estable en el último contraste. Limitación de captura constatada, sin fallo
persistente de producto reproducido. No se cambió renderer ni se certifica todo frame.

La opción de cerrar a bandeja puede ocultar la ventana; Escape global solicita salir.
Desde [012](changes/012-cierre-colas-ipc/result.md), el cierre terminal para IPC antes
de DestroyWindow; ocultar mantiene servicios. En WM_QUIT, App::Run para IPC/loader y
retira bandeja. El destructor para ambos antes de destruir consumidores, también ante
fallo parcial. IPC usa evento stop y ConnectNamedPipe/ReadFile overlapped: cancela y
observa completion antes de liberar almacenamiento, sin enviar centinela local.
Conserva aceptación del centinela histórico. Une hilo y libera payloads ya posteados
al HWND vivo sin aplicar rutas, conservando WM_QUIT. Lifecycle en hilo dueño, serializado.
El loader deja de admitir tareas, descarta la cola aún no tomada y espera solo activas;
libera pendientes/WIC sin callbacks ni uploads. No interrumpe WIC en curso y no ofrece
plazo universal para una llamada ya iniciada. Desde 014, stop_token y lease débil cancelan CPU entre etapas/frames/filas y suprimen entregas. Ambos escáneres cancelan cooperativamente; Board publica catálogo desde temporal propio solo al completar. Progreso Board atómico, callback de búsqueda puede solicitar stop sin join circular. WM_CLOSE une escáneres antes de destruir HWND; destructor/WM_QUIT preservan sus dependencias.

## Modos y controles

**Código:** consultar la [guía de uso](usage.md) para la matriz operativa. Las selecciones de miniaturas, imagen actual, hover de cascada e ítems del tablero son estados distintos.

- **Viewer:** fondo compuesto con alfa, imagen estática/animada, metadatos y tira inferior que aparece al acercar el puntero. `ViewerPage` implementa zoom, paneo, región con Ctrl+clic y menú contextual. `1` de la fila superior alterna ajuste/1:1; NUM1 cambia a Viewer en el handler global. La rueda usa `zoom_on_mousewheel` para intercambiar zoom y navegación con Ctrl. Arriba/abajo se consumen como zoom en esta página. El código local previo a la reforma usa arrastre izquierdo de ventana en compacto y paneo en fullscreen.
- **Waterfall:** miniaturas en columnas y scroll suavizado. El clic carga la imagen mediante `App::LoadImage` y activa `m_viewerModal`: no crea una aplicación secundaria ni un proceso. El clic externo regresa a la cuadrícula. El modo modal reutiliza el visor y tiene sus propias reglas de teclas.
- **Board:** cámara en coordenadas de mundo, imágenes, notas, nodos y enlaces. Alt+clic derecho inicia conexión; menús nativos añaden/configuran ítems. Ctrl+F abre búsqueda; Ctrl+L genera catálogo de tags si no existe; Ctrl+Shift+L trae imágenes del directorio. El catálogo `xmpTags.json` alimenta sugerencias: no es una base de rutas de imágenes. Los formatos y selección tienen gestores separados.

**Usuario:** se desea ajuste al contorno de imagen en compacto. **Código:** `AdjustWindowToImage` depende de la imagen global actual, impone mínimos de 512 y limita al área del monitor. `OnResize` detecta salida de fullscreen y lo llama. Sin imagen retorna sin ajuste; en otros modos puede usar una imagen anterior. No cambiar ese criterio durante tareas de estructura.

## Imágenes, memoria y concurrencia

**Código:** WIC decodifica en CPU a BGRA premultiplicado de 32 bits; D2D/GPU dibuja, escala y compone. `ImageData` mantiene bytes CPU y `ID2D1Bitmap1`; subir a GPU no libera automáticamente los píxeles. `AnimatedImageData` conserva todos los frames decodificados; `UploadAnimationToGpu` crea sus bitmaps y `FrameSequencer` administra retrasos, bucles, pausa y velocidad.

`App::LoadImage` usa `LoadForViewerSync`, bloqueante y solo CPU. Mantiene selección
por extensión, comparte decoder entre count y decode normal y devuelve estático o
animación válida. Ante apertura/count/animación inválidos o reuse estático fallido
libera el intento y reabre para fallback estático. No garantizar una sola apertura
en errores. `LoadSync`/`LoadAnimatedSync` siguen bloqueantes y conservan contrato para
otros consumidores. Las cargas asíncronas encolan tareas, los trabajadores decodifican
y guardan resultados bajo mutex. `ProcessPendingTextures` intercambia colas, comprueba
el predicado opcional de vigencia fuera del mutex, sube texturas si procede y ejecuta
callbacks EN EL HILO PRINCIPAL. Un predicado false solo omite upload: entrega el mismo
resultado CPU, incluso nullptr por fallo. Excepción terminal desde 012: Shutdown
cancela entregas pendientes sin callbacks; desde 014 también una cancelación explícita por consumidor omite predicado/upload/callback, independientemente de la vigencia 011.
Predicado de solo lectura, sin excepciones
ni bombeo; consumidor/modelo deben vivir durante la entrega. Los callbacks de
`AsyncDirectoryScanner` proceden de sus hilos; no extrapolar el contrato del loader.

La lista de extensiones incluye JPEG/JFIF, PNG/APNG, GIF, BMP, TIFF, TGA, ICO, SVG/SVGZ, WebP, AVIF/AVF, HEIF/HEIC y JXR/WDP/HDP. La decodificación real depende de WIC/códecs instalados; las muestras por extensión no certifican soporte. PNG se considera candidato animado. Consultar `DecodeAnimatedImage` antes de atribuir soporte completo a una animación/códec concreto.

Miniaturas de cascada se solicitan al necesitarse y se guardan en previews.
Desde 004, sus callbacks y los de la tira llevan un token único por solicitud y ruta;
se aplica el resultado una sola vez al elemento actual, localizado sin índices capturados.
Ordenar/desplazar conserva el token; borrar o reemplazar lo descarta, incluso al volver
a añadir la misma ruta. Las entradas retenidas de la tira conservan caché y solicitudes;
ocurrencias duplicadas no comparten solicitudes pendientes. Desde 014, tira/previews/hover y Board poseen leases transitorios que solo se prestan débilmente al loader. Borrar/reemplazar o salir de carpeta/página cancela solicitudes; ordenar conserva previews pero cancela hover cuyo índice puede cambiar. Board identifica cada solicitud por ítem, incluso con rutas duplicadas. El loader poda la cola y detiene trabajo activo en límites cooperativos. Desde [011](changes/011-vigencia-antes-upload/result.md),
tira/previews añaden el mismo contraste token/ruta antes del upload, sin consumir token
ni invalidar layout. Comprobación final de 004 y revisión 007 permanecen. Otros
consumidores y cargas animadas conservan su camino sin predicado de upload, pero admiten cancelación por lease. No hay presupuesto global de RAM/VRAM ni límite general
de tareas pendientes. El tablero solicita texturas de hasta 400x400 y expulsa las más
lejanas cuando supera `MAX_BOARD_TEXTURES=150`; ese límite cuenta objetos con textura,
no bytes ni toda la memoria del proceso.

**Pendiente:** medir carpetas de 10 000–50 000 imágenes, picos al cargar animaciones, latencia de cancelación, memoria CPU/GPU, trabajo en idle y pérdida de dispositivo. No describir aceleración de dibujo como garantía de decodificación GPU o responsividad.

## Persistencia y compatibilidad

**Código:** `Settings` (incluido diagnóstico temprano), `FolderDataManager` y `MetadataCache`
comparten `common/user_data_paths.*`: consultan `exists` para `user_data` junto al EXE
y en cinco padres, aceptando también una entrada de tipo archivo. Gana el primer candidato;
si no existe, cada archivo usa directamente el directorio del EXE. Se resuelve en cada
llamada sin caché ni migración. i18n conserva su búsqueda independiente de
`locales/<idioma>/LC_MESSAGES/base.po` en ascendentes. La compilación prepara carpetas
propias junto a cada EXE para no tomar datos personales del checkout. Los logs usan
rutas relativas al directorio de trabajo.

| Dato | Contrato existente |
| --- | --- |
| `settings.json` | UTF-8, objeto `settings`, valores como arrays `[valor, descripción/rangos]`; lector por búsqueda de strings, no parser JSON general |
| `session.txt` | UTF-8 sin BOM, bloques de cuatro líneas por carpeta; las líneas históricas de hash/modificadores están vacías; se restaura la primera carpeta válida, no toda la selección/cámara |
| `fav.txt` | UTF-8, bloques de cuatro líneas por archivo favorito |
| Historial | `FolderDataManager` mantiene historial en memoria; no asumir persistencia de todos los archivos históricos del visor Python |
| `xmp_tag_cache.bin` | Firma `KTAG`, uint32 versión 1/cantidad; timestamps int64; rutas/tags con longitudes uint32 y `wchar_t` Windows; invalidación por última escritura |
| `!board.kbrd` (serializador genérico) | Firma `KBRD`, int32 versión 1; cámara, cantidad, tipos/geometría, enlaces por índices, color, flags de un byte y strings con longitud int32 + `wchar_t` |
| `xmpTags.json` | Array UTF-8 de tags únicos del directorio; lo escribe el escáner de Board y lo lee su autocompletado |

Los tableros referencian imágenes mediante rutas; no contienen una copia incrustada de ellas. Mantener valores del enum, orden binario y tamaños; una futura evolución requiere migración/versionado. Los lectores binarios no equivalen a validadores robustos de archivos arbitrarios. `MetadataCache::NormalizePath` normaliza separadores y letra de unidad, pero no es una canonicalización general. Los datos locales no se publican ni se usan como fixtures de tests.

## Etiquetas y metadatos

**Código:** `XmpTagManager` coordina lectura/escritura, cache y búsqueda. `ReadTags` consulta cache por timestamp, intenta WIC y recurre a `IPropertyStore/PKEY_Keywords` cuando es necesario. JPEG/JFIF usa inyección de segmento APP1/XMP; PNG/APNG usa chunk iTXt; otros formatos pasan por WIC y pueden fallar si su códec no permite escritura. `XmpTagManager::WriteMetadataExtended` aplica los mismos inyectores a JPEG/PNG y usa caminos WIC fast/transcode para otros formatos.

Los inyectores preservan el contenido binario salvo la sección de metadatos que sustituyen. Escriben un temporal, eliminan el archivo anterior y renombran: no asumir reemplazo atómico ni seguridad ante interrupción. Probar siempre con copias de imágenes. Se sincronizan palabras clave con Windows cuando la ruta lo permite; fallo del Property Store no demuestra ausencia de XMP.

`AsyncDirectoryScanner` descubre recursivamente archivos y distribuye trabajo entre hilos; las consultas requieren coincidencia de todos los tags, sin distinguir mayúsculas. `BoardTagScanner` hace otro escaneo, no recursivo, de tags únicos y escribe `xmpTags.json`. Ctrl+L no lo regenera si ya existe: conservar esta distinción al diagnosticar resultados obsoletos.

## Configuración: valores y consumidores

**Código:** `Settings::Load/Save` persiste campos aunque no tengan efecto visible. `OptionsMenu::BuildControls` enlaza controles a miembros; un control visible no prueba que exista consumidor. Los defaults de distribución conservan las decisiones del antiguo ZIP (por ejemplo sin inicio automático y sin ocultar a bandeja al cerrar), distintas de algunos defaults de `settings.h`.

| Campos | Consumidor o estado |
| --- | --- |
| `ui_lang` | `I18nEngine`, recarga desde opciones EN/RU; inglés usa msgid, es/de/fr/it/ru cargan `.po` si se configura su código |
| `page_on_app_launch_from_explorer`, `page_to_open_on_drag_and_drop` | `App::Initialize/OnFileDrop`, valores del enum Page |
| `show_fullscreen`, `hide_on_app_start`, `single_instance`, `hide_to_tray_on_close` | Arranque, IPC y callback de cierre |
| `doubleclick_toggle` | `ViewerPage::OnMouseDoubleClick` |
| `effects`, `animated_zoom`, `zoom_on_mousewheel` | Animaciones/zoom y selección de gesto; arriba/abajo tiene camino propio |
| `window_opacity` | Limpieza alfa del frame; no es un ajuste de todas las ventanas del sistema |
| `thumbnail_width`, `sort_mode` | Tira, layout/ordenación y persistencia |
| `show_thirds`, `show_cyberpunk`, `show_metadata` | Dibujo del visor |
| `enable_file_log`, `enable_debug_ipc` (clave `debug_ipc`) | Flags globales de diagnóstico; consulta temprana antes de crear App |
| `desaturated_corner_buttons_and_corner_menus` | Color del botón de cierre en App; no asumir que cubre todos los menús |
| `settings_window_opacity` | Fondo del overlay de opciones |
| `draw_control_panel_backplate`, `draw_default_thumbnail` | Fondo de tira e indicador de carga en `ThumbnailStrip` |
| `debug_page`, `do_not_show_start_dialog`, `run_on_windows_startup`, `show_console_output`, `show_noise_cells` | Sin consumidor funcional encontrado fuera de persistencia/controles. `ShellIntegrator::SetRunOnStartup` existe pero no tiene llamadores en las fuentes actuales |

Para cambiar esta tabla, buscar el identificador en todas las fuentes: carga/guardado y construcción de controles no cuentan como consumidor funcional.

## Compilación, distribución y trabajo futuro

La [revisión de oportunidades de 003](changes/003-diagnostico-rendimiento/source-optimization-review.md)
traza arranque, carga, tira, grids y presentación. Documenta trabajo repetido en
reconstrucción/layout/texto, apertura de decoder e inicialización secundaria, además
de uploads caducados y dibujo continuo. La auditoría por sí sola no demuestra ahorro;
el seguimiento de implementación se indica debajo. La ruta local precede a cualquier
cambio transversal de presentación; preservar
calidad, animaciones y latencia exige validar equivalencia e invalidación completa.

[006](changes/006-optimizaciones-locales/result.md) implementa la primera porción:
SetFileList conserva ocurrencias mediante índice temporal, Update/Draw de tira buscan
el intervalo visible por bisección de la geometría existente y el overlay no prepara
metadatos a opacidad no positiva. Texturas/tokens, orden y filtros de borde se conservan.
Equivalencia diferencial y 12 frames WARP idénticos; ahorro de tiempo medido solo para
reconstrucción sintética, sin prueba de menor RAM o mejor arranque global. Los demás
candidatos del informe siguen siendo propuestas.

[007](changes/007-layouts-por-cambios/result.md) reutiliza los rectángulos de
Waterfall mediante clave de revisión transitoria de FolderDataManager,
selección, ancho/cantidad y parámetros de Waterfall. El greedy y el dibujo no cambian.
Mutadores del gestor notifican automáticamente; las rutas directas App::DeleteImage
también. Todo acceso mutable futuro a carpetas/previews que
cambie estructura/orden/dimensiones o rects debe llamar NotifyPreviewLayoutChanged
en UI. No notificar scroll/hover. No se duplica geometría ni se persiste la revisión.
Evidencia histórica de 007: 24 frames offscreen idénticos con ambas páginas; métricas de cálculo aislado, sin ahorro global demostrado.

[008](changes/008-formatos-texto/result.md) reutiliza formatos privados de Renderer
en UI/render, máximo 32/FIFO y clave de familia hasta 128 wchar_t, sin truncar inputs
que pasan sin caché. Familia/tamaño/alineaciones y factory definen identidad efectiva;
colección/peso/estilo/stretch/locale son constantes del contrato interno. No compartir
estos objetos por API pública ni mutarlos después de publicar; CreateTextFormat sigue
creando formatos independientes. Limpiar por CreateDeviceResources/Discard y reemplazo
de factory. Layouts/textos/medidas siguen por llamada; retención CPU acotada, sin prueba
de menor RAM o mejora global. Resultado registra equivalencia y variación inicial del
baseline, también observada con camino sin reutilización, sin causa confirmada.

[009](changes/009-decoder-compartido/result.md) extrae helpers de decoder prestado
sin modificar cuerpos de frame 0/composición. El resultado CPU no contiene objetos WIC;
App conserva upload, secuenciador, rama animada con retorno temprano y sincronización
estática. No hay nuevos workers/cachés persistentes. Equivalencia CPU y conteos WIC
validados; capacidades APNG/WebP y coste de fallback adicionales quedan explícitos.

Consultar [desarrollo y distribución](development.md), [problemas conocidos](known-issues.md) y [validación](validation.md). CMake compila contra bibliotecas del Windows SDK sin gestor de dependencias C++ externo. MSVC usa `/W4 /WX` para la app, PCH y compilación paralela; el harness de tags permite warnings. `tagging_tests` enlaza un stub de creación de bitmap: no prueba GPU.

Las herramientas viven en `scripts/`, el instalador en `packaging/`, las muestras en `tests/fixtures/` y el material de identidad en `assets/branding/`. Los scripts no copian datos personales; cada paquete usa staging nuevo y runtime VC++ x64 app-local. Inno Setup conserva identidad de aplicación, tareas y asociaciones de Explorer.

El [flujo de trabajo](workflow.md) registra los cambios deseados en `docs/changes/` y su orden en la [hoja de ruta](roadmap.md). Mantener las especificaciones separadas del comportamiento observado. No hay rediseño de arquitectura ni cambio de formato de datos en esta reforma.

## Protocolo de mantenimiento

1. Identificar el modo y el propietario real del estado afectado; leer handlers antes de cambiar atajos.
2. Consultar el estado de Git y preservar cambios locales y datos. Probar metadatos, borrado, registro y empaquetado en copias aisladas.
3. Ejecutar validación proporcional al cambio; no usar el paso de una compilación como evidencia de UX ni de rendimiento.
4. Actualizar esta documentación, guía de uso o formatos si cambia su contrato. Registrar limitaciones reproducibles en problemas conocidos, con evidencia y sin confundir hipótesis con defectos confirmados.
5. Mantener comandos reproducibles desde checkout limpio, sin rutas de una máquina ni historial narrativo de sesiones.

[010](changes/010-inicializacion-secundaria/result.md) implementa derivación secundaria
antes de favoritos/WIC/XMP/overlays, con HWND oculto y callback temporal de rutas.
Release/CTest 5/5 y diez escenarios nativos automatizados pasados; revisión visual
de usuario pendiente. Se omite trabajo, pero la muestra temporal no demuestra
aceleración. Errores 005/RAII/endpoint conservados; sin ACK ni rediseño de cierre.
