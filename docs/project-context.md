# Contexto técnico de LenteUV

Última revisión: 2026-10-21. Audiencia: agentes y desarrolladores que mantengan este repositorio.

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
| `core/window.*` | HWND, estilos/transparencia, DPI, fullscreen, captura de ratón y traducción de mensajes a callbacks |
| `core/single_instance_ipc.*` | Cliente/servidor named pipe, envío de rutas y cierre del hilo servidor |
| `core/settings.*` | Preferencias y parser/escritor JSON propio |
| `rendering/` | Dispositivo D3D11, DXGI, Direct2D, DirectWrite, DirectComposition, curvas y animaciones por nombre |
| `loader/` | `ImageLoader`, decodificación WIC, trabajadores, colas de texturas y `FrameSequencer` |
| `viewer/`, `ui/thumbnail_strip.*` | Imagen principal, transformaciones, zoom de región y tira inferior |
| `library/` | Historial de carpetas, sesión/favoritos, miniaturas y vista previa al hover |
| `waterfall/` | Distribución en columnas, desplazamiento y visor modal dentro de la misma aplicación |
| `board/` | Tablero, selección, edición, conexiones, serialización, búsqueda, escaneo y distribución |
| `metadata/` | Fachada XMP, caché, escaneo asíncrono y lectores/escritores por formato |
| `ui/options_menu.*`, `ui/tagging_modal.*` | Overlays de opciones y etiquetas/metadatos |
| `i18n/`, `shell/` | Traducciones `.po`, bandeja, integración con Explorer, inicio de Windows y Papelera |
| `resources/` | ICO, recurso RC y manifiesto Windows |

Patrones observados: composición de componentes, callbacks `std::function`, RAII con `ComPtr`, propiedad compartida de imágenes con `shared_ptr`, productores/consumidor para cargas y fachada `XmpTagManager`. `BoardPage` delega a `BoardCanvas`, `BoardSelectionManager`, `BoardTextEditor`, `BoardSerializer`, `BoardSearchManager`, `BoardTagScanner` y `BoardLayoutHelper`. No añadir una nueva jerarquía abstracta para una simple reorganización.

## Arranque, entrada, renderizado y cierre

**Código:** `wWinMain` consulta el flag de diagnóstico temprano; `wWinMainImpl` establece DPI Per-Monitor V2, inicializa COM, convierte argumentos y crea `App`. `ParseCommandLine` admite archivo/carpeta y `--frameless`, `--windowed`, `--opaque`, `--maximized`, `--fullscreen`, `--multiple`/`--lite` (también con un guion). Las dos últimas alternativas desactivan el control de instancia para ese proceso; no confundir `--lite` con una implementación independiente de visor.

`App::Initialize` carga settings, traducciones y favoritos; crea el loader de cuatro trabajadores y la fachada XMP; prepara los overlays, la ventana y el IPC; registra bandeja/callbacks; crea el renderer; muestra la ventana según preferencias; inicializa las páginas y restaura sesión antes de cargar el argumento explícito. El orden importa: una carga prematura antes del renderer/páginas produjo problemas históricos de arranque. Sin argumentos ni carpetas restauradas se intenta el directorio de trabajo.

`Page` conserva valores persistidos: Start=1, Viewer=2, Board=3, Library=4, Waterfall=5. Los atajos de cambio de modo NO corresponden directamente a esos valores. Start permanece en el enum/opciones, pero `DrawViewerContent` solo dibuja las cuatro páginas funcionales.

`App::Run` vacía mensajes mediante `PeekMessageW`, actualiza/renderiza y espera hasta 16 ms cuando no considera necesaria animación continua. No es un renderer que duerma indefinidamente hasta invalidación. `RenderFrame` limita `dt`, actualiza animaciones y página, procesa cargas pendientes, dibuja el contenido, overlays y menú radial, y presenta. `WindowEvents` convierte mensajes nativos en llamadas a `App`; los handlers ofrecen primero eventos a overlays y páginas. Un handler puede consumir una tecla antes del switch global.

`SingleInstanceIpc` usa `\\.\pipe\lenteuv`, rutas UTF-8 y un hilo servidor que entrega la apertura mediante mensaje a la ventana (`WM_USER + 4501`). La comprobación inicial hace un sondeo de la pipe; no hay un mutex de elección de propietario. No asumir ausencia de carreras entre arranques simultáneos. El receptor normaliza la ruta, distingue carpeta/archivo, cambia de página, restaura y solicita primer plano. Una ruta vacía se ignora.

La opción de cerrar a bandeja puede ocultar la ventana; Escape global solicita salir. En `WM_QUIT`, `App::Run` cierra IPC, espera los trabajadores del loader y retira bandeja. El IPC despierta su servidor con un mensaje centinela. El loader drena tareas encoladas antes de terminar: una cola extensa puede retrasar el cierre. Otros escáneres tienen sus propios hilos y destructores; revisar propiedad, cancelación y callbacks si se altera el ciclo de vida.

## Modos y controles

**Código:** consultar la [guía de uso](usage.md) para la matriz operativa. Las selecciones de miniaturas, imagen actual, hover de biblioteca e ítems del tablero son estados distintos.

- **Viewer:** fondo compuesto con alfa, imagen estática/animada, metadatos y tira inferior que aparece al acercar el puntero. `ViewerPage` implementa zoom, paneo, región con Ctrl+clic y menú contextual. `1` de la fila superior alterna ajuste/1:1; NUM1 cambia a Viewer en el handler global. La rueda usa `zoom_on_mousewheel` para intercambiar zoom y navegación con Ctrl. Arriba/abajo se consumen como zoom en esta página. El código local previo a la reforma usa arrastre izquierdo de ventana en compacto y paneo en fullscreen.
- **Library:** panel de carpetas a la izquierda, distribución de previews a la derecha y original al hover. `FolderDataManager` enumera el directorio de forma síncrona y crea entradas de preview. El clic abre Viewer. NUM4/4 puede llevar el archivo bajo hover al tablero; seguir `LibraryPage::OnKeyDown` y `BoardPage::OnEnterPage`.
- **Waterfall:** miniaturas en columnas y scroll suavizado. El clic carga la imagen mediante `App::LoadImage` y activa `m_viewerModal`: no crea una aplicación secundaria ni un proceso. El clic externo regresa a la cuadrícula. El modo modal reutiliza el visor y tiene sus propias reglas de teclas.
- **Board:** cámara en coordenadas de mundo, imágenes, notas, nodos y enlaces. Alt+clic derecho inicia conexión; menús nativos añaden/configuran ítems. Ctrl+F abre búsqueda; Ctrl+L genera catálogo de tags si no existe; Ctrl+Shift+L trae imágenes del directorio. El catálogo `xmpTags.json` alimenta sugerencias: no es una base de rutas de imágenes. Los formatos y selección tienen gestores separados.

**Usuario:** se desea ajuste al contorno de imagen en compacto. **Código:** `AdjustWindowToImage` depende de la imagen global actual, impone mínimos de 512 y limita al área del monitor. `OnResize` detecta salida de fullscreen y lo llama. Sin imagen retorna sin ajuste; en otros modos puede usar una imagen anterior. No cambiar ese criterio durante tareas de estructura.

## Imágenes, memoria y concurrencia

**Código:** WIC decodifica en CPU a BGRA premultiplicado de 32 bits; D2D/GPU dibuja, escala y compone. `ImageData` mantiene bytes CPU y `ID2D1Bitmap1`; subir a GPU no libera automáticamente los píxeles. `AnimatedImageData` conserva todos los frames decodificados; `UploadAnimationToGpu` crea sus bitmaps y `FrameSequencer` administra retrasos, bucles, pausa y velocidad.

`LoadSync` y `LoadAnimatedSync` bloquean; los usa `App::LoadImage`. Las cargas asíncronas encolan tareas, los trabajadores decodifican y guardan resultados bajo mutex. `ProcessPendingTextures` intercambia las colas, sube las texturas y ejecuta los callbacks EN EL HILO PRINCIPAL. Los callbacks de `AsyncDirectoryScanner` son otro contrato: proceden de sus hilos de búsqueda. No extrapolar el contrato del loader al escáner.

La lista de extensiones incluye JPEG/JFIF, PNG/APNG, GIF, BMP, TIFF, TGA, ICO, SVG/SVGZ, WebP, AVIF/AVF, HEIF/HEIC y JXR/WDP/HDP. La decodificación real depende de WIC/códecs instalados; las muestras por extensión no certifican soporte. PNG se considera candidato animado. Consultar `DecodeAnimatedImage` antes de atribuir soporte completo a una animación/códec concreto.

Miniaturas de biblioteca/cascada se solicitan al necesitarse y se guardan en previews. Callbacks capturan índices o rutas; revisar invalidez al cambiar de carpeta, ordenar o borrar. La tira tiene su propio cache. No hay un presupuesto global de RAM/VRAM ni límite general de tareas pendientes. El tablero solicita texturas de hasta 400x400 y expulsa las más lejanas cuando supera `MAX_BOARD_TEXTURES=150`; ese límite cuenta objetos con textura, no bytes ni toda la memoria del proceso.

**Pendiente:** medir carpetas de 10 000–50 000 imágenes, picos al cargar animaciones, latencia de cancelación, memoria CPU/GPU, trabajo en idle y pérdida de dispositivo. No describir aceleración de dibujo como garantía de decodificación GPU o responsividad.

## Persistencia y compatibilidad

**Código:** `Settings`, `FolderDataManager`, `MetadataCache` e i18n repiten búsqueda de hasta seis directorios ascendentes desde el ejecutable. El primer `user_data` existente determina settings/sesión/favoritos/caché; si no existe, los archivos correspondientes pueden acabar junto al EXE. i18n busca `locales/<idioma>/LC_MESSAGES/base.po` en ascendentes. La compilación prepara carpetas propias junto a cada EXE para no tomar datos personales del checkout. Los logs usan rutas relativas al directorio de trabajo.

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

Consultar [desarrollo y distribución](development.md), [problemas conocidos](known-issues.md) y [validación](validation.md). CMake compila contra bibliotecas del Windows SDK sin gestor de dependencias C++ externo. MSVC usa `/W4 /WX` para la app, PCH y compilación paralela; el harness de tags permite warnings. `tagging_tests` enlaza un stub de creación de bitmap: no prueba GPU.

Las herramientas viven en `scripts/`, el instalador en `packaging/`, las muestras en `tests/fixtures/` y el material de identidad en `assets/branding/`. Los scripts no copian datos personales; cada paquete usa staging nuevo y runtime VC++ x64 app-local. Inno Setup conserva identidad de aplicación, tareas y asociaciones de Explorer.

Para futura adopción de GitHub Spec Kit se reservan `.specify/` y `specs/`, sin simular una instalación ni generar comandos. Mantener especificaciones futuras separadas del comportamiento observado. No hay rediseño de arquitectura ni cambio de formato de datos en esta reforma.

## Protocolo de mantenimiento

1. Identificar el modo y el propietario real del estado afectado; leer handlers antes de cambiar atajos.
2. Consultar el estado de Git y preservar cambios locales y datos. Probar metadatos, borrado, registro y empaquetado en copias aisladas.
3. Ejecutar validación proporcional al cambio; no usar el paso de una compilación como evidencia de UX ni de rendimiento.
4. Actualizar esta documentación, guía de uso o formatos si cambia su contrato. Registrar limitaciones reproducibles en problemas conocidos, con evidencia y sin confundir hipótesis con defectos confirmados.
5. Mantener comandos reproducibles desde checkout limpio, sin rutas de una máquina ni historial narrativo de sesiones.
