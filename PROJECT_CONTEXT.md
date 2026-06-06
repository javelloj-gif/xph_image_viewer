# LenteUV Image Viewer (basado en Sergkruma's image viewer) — Complete Native Project Context

> **Purpose**: This document provides the complete, consolidated context for any agent or developer needing to understand, maintain, or build the native C++20 implementation of LenteUV Image Viewer. It contains only native details and eliminates legacy Python context.

---

## 1. Project Identity

| Field | Value |
|---|---|
| **Name** | LenteUV Image Viewer (Native) |
| **Version** | v0.90 Alpha (C++20 Migration) |
| **Author** | Sergei Krumas ([github.com/sergkrumas](https://github.com/sergkrumas)) |
| **License** | GPL v2+ |
| **Language** | C++20 (MSVC x64 compiler) |
| **UI Backend** | Direct2D 1.3 / Direct3D 11 / DirectWrite / DirectComposition |
| **Platform** | Windows 10 (Build 18362+) / Windows 11 |
| **Status** | **Fases 1-3 estructuralmente COMPLETAS**. Core engine, UI pages, settings, IPC, shell integration, commenting, i18n, Options HUD, y el modal transversal de tagging (`TaggingModal`) completamente implementados. El sistema de tagging XMP lee/escribe palabras clave (`System.Keywords` bidireccional), estrellas y metadatos Dublin Core. Se corrigieron de manera definitiva todos los bugs de ajuste de ventana (límite mínimo `512*512` e inflado no-cliente por `AdjustWindowRectEx`), estabilidad en las transiciones a fullscreen de doble click, el zoom de visualización (escala 1:1 para imágenes pequeñas y fit-to-screen para imágenes grandes sin deformaciones), y se rediseñó el `TaggingModal` ampliándolo un 20% (1152x840). **Adicionalmente, se completó la búsqueda por tags en Board (CTRL+F), el barrido e indexación en segundo plano (CTRL+L), el soporte nativo de Alt+F4 en Windows, y las mejoras de usabilidad/escalabilidad (click directo para copiar, enfoque continuo al crear tag, categorías con cabeceras cian en xmpTags.json, etiquetado masivo, listado de directorio masivo en grid con desalojo VRAM inteligente de texturas lejanas, enlace con Alt+Click y selección cyberpunk cian)**. **Se ha generado el plan de optimización, refactorización y división de clases grandes (como BoardPage de 1700+ líneas y wic_metadata_reader_writer.cpp de 1054 líneas en sub-componentes independientes, y eliminación de CommentsManager), así como la estrategia de rebranding a LenteUV (basado en LenteUV Image Viewer y Google Picasa) documentado en el archivo del proyecto implementation_plan.md. Recientemente se incorporó al plan de implementación la eliminación de info flotante de FPS/Opacidad del visor y la búsqueda de tags en tablero por agrupación/enfoque con un botón de toggle en la esquina inferior derecha**. Phase 4 (Packaging/Installer) pendiente. |
| **Inspiration** | Google Picasa Photo Viewer + PureRef |

---

## 2. Codebase Metrics (Native C++ Source Files)

Sorted by lines of code:

| File | Lines | Size (Bytes) | Role |
|---|---|---|---|
| [viewer_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.cpp) | 969 | 37,063 | Main single-image viewer page with GPU zoom & pan |
| [app.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp) | 889 | 33,561 | Application orchestrator, game loop, and main event router |
| [waterfall_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/waterfall/waterfall_page.cpp) | 821 | 30,293 | Masonry column grid layout with virtual scrolling and modal viewer |
| [library_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/library_page.cpp) | 787 | 33,042 | Sidebar list & masonry grid card library explorer |
| [board_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.cpp) | 1476 | 55,225 | Coordinator for virtual infinite workspace page |
| [wic_metadata_reader_writer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/wic_metadata_reader_writer.cpp) | 600 | 29,812 | WIC Dublin Core query parser & metadata transcoder fallback |
| [thumbnail_strip.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/thumbnail_strip.cpp) | 578 | 22,900 | Bottom thumbnail strip row with selection/reordering |
| [renderer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/renderer.cpp) | 558 | 17,354 | Direct2D/D3D11 rendering backend wrapper |
| [options_menu.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/options_menu.cpp) | 489 | 18,129 | In-app HUD settings interface modal page |
| [window.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.cpp) | 473 | 13,642 | Win32 window wrapper & hit-testing for transparent frames |
| [image_loader.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/image_loader.cpp) | 383 | 13,442 | Multi-threaded WIC file decoder |
| [folder_data_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/folder_data_manager.cpp) | 369 | 12,137 | History, favorites, and session persistence management |
| [types.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/common/types.h) | 299 | 11,290 | Geometric vectors, colors, logging macros, and clocks |
| [tagging_tests.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/tagging_tests.cpp) | 298 | 11,976 | Unit tests suite verifying tagging cache, R/W, and search |
| [single_instance_ipc.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/single_instance_ipc.cpp) | 239 | 7,307 | Named pipe single-instance connection & file path IPC |
| [png_chunk_injector.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/png_chunk_injector.cpp) | 221 | 9,054 | Binary chunk injection of XMP iTXt metadata in PNG streams |
| [settings.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/settings.cpp) | 220 | 11,965 | JSON settings file serializer/deserializer |
| [shell_integrator.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/shell/shell_integrator.cpp) | 215 | 7,297 | COM bindings, system tray menus, & Recycle Bin wrapper |
| [renderer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/renderer.h) | 200 | 9,026 | Direct2D/D3D11 rendering backend wrapper |
| [async_directory_scanner.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/async_directory_scanner.cpp) | 196 | 7,877 | Thread-pool recursive directory scan tag indexer |
| [i18n_engine.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/i18n/i18n_engine.cpp) | 180 | 6,000 | Translation loader for gettext PO files |
| [board_serializer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_serializer.cpp) | 176 | 6,371 | Serialization of BoardItems to .board binary format |
| [metadata_cache.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/metadata_cache.cpp) | 173 | 6,238 | In-memory database & file-based index (LTAG v1) serialization |
| [board_item_renderer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_item_renderer.cpp) | 167 | 7,574 | Direct2D rendering of board nodes, links, and text notes |
| [app.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.h) | 167 | 7,457 | Application orchestrator, game loop, and main event router |
| [jpeg_segment_injector.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/jpeg_segment_injector.cpp) | 166 | 6,697 | APP1/XMP marker segment injection in JPEG files |
| [board_selection_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_selection_manager.cpp) | 158 | 6,534 | Lasso selection & item scale/translation tools |
| [image_loader.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/image_loader.h) | 154 | 7,873 | Multi-threaded WIC file decoder |
| [xmp_tag_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/xmp_tag_manager.cpp) | 131 | 4,971 | Primary interface/facade for metadata tags query/search |
| [window.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.h) | 128 | 6,359 | Win32 window wrapper & hit-testing for transparent frames |
| [main.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/main.cpp) | 121 | 5,431 | WinMain entry point & Scoped COM apartment initialization |
| [viewer_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.h) | 118 | 5,982 | Main single-image viewer page with GPU zoom & pan |
| [animation_controller.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/animation_controller.cpp) | 108 | 3,627 | Transition scheduler & animation timer management |
| [thumbnail_strip.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/thumbnail_strip.h) | 101 | 4,269 | Bottom thumbnail strip row with selection/reordering |
| [board_text_editor.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_text_editor.cpp) | 99 | 3,714 | Caret blinking & note text insertion/deletion edits |
| [waterfall_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/waterfall/waterfall_page.h) | 97 | 3,488 | Masonry column grid layout with virtual scrolling and modal viewer |
| [board_canvas.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_canvas.cpp) | 96 | 3,627 | Canvas viewport transforms & checkerboard/grid backgrounds |
| [folder_data_manager.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/folder_data_manager.h) | 93 | 3,973 | History, favorites, and session persistence management |
| [library_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/library_page.h) | 93 | 3,679 | Sidebar list & masonry grid card library explorer |
| [board_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.h) | 110 | 3,933 | Coordinator for virtual infinite workspace page |
| [frame_sequencer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/frame_sequencer.cpp) | 74 | 2,746 | GIF/APNG/WebP animation frame delay sequencer |
| [board_item.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_item.h) | 69 | 2,046 | Structural state & geometry representing a single canvas element |
| [animation_curve.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/animation_curve.cpp) | 67 | 2,143 | Mathematical easing equations for UI transitions |
| [pch.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/common/pch.h) | 62 | 2,885 | Precompiled headers for Windows SDK, DirectX, WIC, and STL |
| [options_menu.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/options_menu.h) | 62 | 1,841 | In-app HUD settings interface modal page |
| [settings.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/settings.h) | 59 | 2,407 | JSON settings file serializer/deserializer |
| [single_instance_ipc.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/single_instance_ipc.h) | 57 | 2,345 | Named pipe single-instance connection & file path IPC |
| [comments_manager.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/comments_manager.h) | 53 | 1,767 | Image comment persistence in comments.txt |
| [animation_controller.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/animation_controller.h) | 52 | 1,911 | Transition scheduler & animation timer management |
| [async_directory_scanner.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/async_directory_scanner.h) | 50 | 1,705 | Thread-pool recursive directory scan tag indexer |
| [wic_metadata_reader_writer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/wic_metadata_reader_writer.h) | 50 | 1,608 | WIC Dublin Core query parser & metadata transcoder fallback |
| [frame_sequencer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/frame_sequencer.h) | 49 | 1,659 | GIF/APNG/WebP animation frame delay sequencer |
| [xmp_tag_manager.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/xmp_tag_manager.h) | 49 | 1,549 | Primary interface/facade for metadata tags query/search |
| [board_selection_manager.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_selection_manager.h) | 48 | 1,990 | Lasso selection & item scale/translation tools |
| [app.rc](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/resources/app.rc) | 44 | 1,827 | Executable binary resource description sheet |
| [board_item_renderer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_item_renderer.h) | 42 | 1,236 | Direct2D rendering of board nodes, links, and text notes |
| [app.manifest](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/resources/app.manifest) | 41 | 1,395 | DPI awareness configuration manifest file |
| [board_canvas.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_canvas.h) | 40 | 1,259 | Canvas viewport transforms & checkerboard/grid backgrounds |
| [metadata_cache.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/metadata_cache.h) | 38 | 1,208 | In-memory database & file-based index (LTAG v1) serialization |
| [board_text_editor.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_text_editor.h) | 34 | 1,284 | Caret blinking & note text insertion/deletion edits |
| [board_serializer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_serializer.h) | 33 | 995 | Serialization of BoardItems to .board binary format |
| [animation_curve.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/animation_curve.h) | 33 | 956 | Mathematical easing equations for UI transitions |
| [png_chunk_injector.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/png_chunk_injector.h) | 29 | 1,094 | Binary chunk injection of XMP iTXt metadata in PNG streams |
| [shell_integrator.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/shell/shell_integrator.h) | 29 | 1,150 | COM bindings, system tray menus, & Recycle Bin wrapper |
| [i18n_engine.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/i18n/i18n_engine.h) | 27 | 844 | Translation loader for gettext PO files |
| [jpeg_segment_injector.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/jpeg_segment_injector.h) | 21 | 760 | APP1/XMP marker segment injection in JPEG files |
| [pch.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/common/pch.cpp) | 6 | 419 | Precompiled headers for Windows SDK, DirectX, WIC, and STL |


## 3. Native Architecture & Design Decisions

### Win32 & DirectComposition Blending
- **The Decision**: Create a frameless `WS_POPUP` window using the `WS_EX_NOREDIRECTIONBITMAP` extended style, and bind it to a `DirectComposition` target.
- **Why**: Allows true per-pixel desktop alpha-translucency without relying on heavy UI frameworks (like Qt) or causing the rendering flicker and DWM composition failure found in Python.
- **Implementation**: Managed by `Window` and `Renderer` classes.

### Flip-Model DXGI Present
- **The Decision**: Configure the DXGI Swap Chain with `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL` and premultiplied alpha.
- **Why**: Direct hardware page-flipping bypasses GDI entirely, eliminating visual tearing and keeping latency to an absolute minimum.
- **Implementation**: Managed in `Renderer::Initialize`.

### Bounded Worker Thread Pool (Async Image Loading)
- **The Decision**: Decode image metadata and frame pixels on a pool of background worker threads using **WIC (Windows Imaging Component)**. Post completed pixel data to a thread-safe queue. Upload textures on the main thread only.
- **Why**: Keeps the UI main thread 100% responsive. Isolating Direct3D resource allocation (creating `ID2D1Bitmap1` from raw pixels) to the render thread avoids race conditions, crashes, and driver thread-safety locks.
- **Implementation**: Found in `ImageLoader` (`WorkerLoop` vs `ProcessPendingTextures`).

### UI Virtualization & LRU Caching
- **The Decision**: Render only the visible subsets of large directory files inside the Waterfall and Thumbnail Strip views. Evict unused textures using a Least-Recently-Used (LRU) policy.
- **Why**: Keeps memory usage low and rendering loops tight even when scanning directories with 20,000+ images.
- **Implementation**: Found in `WaterfallPage` and `ThumbnailStrip`.

### Mitigating Performance Pitfalls (Lessons from Legacy Version)
- **Preserving the Translucent Background**: Windows blending with the desktop background using native hardware composition is forced by initializing the Win32 window with the `WS_EX_NOREDIRECTIONBITMAP` extended style, extending the frame into the client area via `DwmExtendFrameIntoClientArea`, clearing the DXGI Swap Chain render target with zero alpha (`{ 0.0f, 0.0f, 0.0f, 0.0f }`), and drawing transparent backgrounds.
- **Eliminating Flickering and Tearing**: Use Flip-Model presentation (`DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL`). Render all elements (images, boards, grid lines, text overlays) to an off-screen Direct2D render target buffer, and call `IDXGISwapChain::Present` exactly once per frame (VSync-aligned) to update the screen atomically.
- **Guaranteeing High Performance**: Upload all decoded images to GPU VRAM as `ID2D1Bitmap1` textures immediately after loading. Scaling, translations, and rotations are computed on the GPU via hardware vertex transforms. Frustum culling on the Board Page ensures only items intersecting the viewport are drawn.
- **Robust and Async Thumbnail Loading**: All file decoding (via WIC) occurs on background worker threads. Once decoded, pass raw pixel data to the main thread via a thread-safe queue, and let the main thread instantiate the `ID2D1Bitmap1` texture to isolate GPU texture creation and prevent driver context issues.

### Scaling Strategy for Large Directories (20,000+ Images)
- **Asynchronous Incremental Directory Scanning**: `std::filesystem::directory_iterator` scans inside a background worker thread. Paths are discovered incrementally and posted in batches (e.g., 100 paths) back to the main UI thread via thread-safe callbacks to prevent blocking the Win32 message pump. Sorting is executed on the background thread once scanning completes.
- **UI Virtualization (DIP Viewport Mapping)**: Compute layout boundaries using arithmetic virtualization (cell index calculations based on scroll offsets: `first_visible = scroll_pos / cell_width`). Only vertices and rendering calls for items currently within the viewport bounds are processed.
- **Priority-Based Bounded Task Queue**: The `ImageLoader` worker queue is bounded and prioritizes tasks dynamically. Tasks for items leaving the viewport before loading starts are automatically cancelled or deprioritized.
- **LRU GPU Texture Cache (VRAM Management)**: Impose a strict Least Recently Used (LRU) limit on `ID2D1Bitmap1` textures. Limit thumbnail cache memory (e.g., 1000 active thumbnail textures) and full-size image VRAM cache (e.g., 5-10 high-resolution textures). Release texture handles when the limit is exceeded.
- **Metadata Database & Binary File Indices**: Cache file properties (timestamps, sizes, dimensions) in a fast in-memory map or a compact binary index. File sorting actions query the memory-cache directly, completing in under 10ms.

---

## 4. Subsystems Overview

### App Orchestrator (`app.h` / `app.cpp`)
- **Role**: Employs a continuous rendering game-loop pattern using `PeekMessageW` and `MsgWaitForMultipleObjects`.
- **Game Loop**: Computes delta-time (`dt`), updates animations, processes pending textures, renders active page components, and invokes `IDXGISwapChain::Present`. Idles automatically at 16ms waits when the screen state is static.

### Window & Renderer (`window.cpp`, `renderer.cpp`)
- **Role**: Handles window message translation (resizing via custom hit-test, drag-and-drop file loads, tray mouse events) and Direct2D primitives drawing (transform stacks, line/rect vectors, formatted text).

### Image Loader (`image_loader.cpp`)
- **Role**: Standard WIC imaging wrapper. Decodes and scales JPG, PNG, GIF, WebP, AVIF, HEIC, TIFF, and BMP frames asynchronously.

### Named-Pipe IPC (`ipc.cpp`)
- **Role**: Ensures single-instance application control. The pipe server listens on a background thread. When a second instance launches, it writes target files to the pipe and exits immediately. The primary instance intercepts the path, navigates to it, and pulls itself to the foreground.
- **Poison-pill Shutdown**: Resolves zombie process locks by executing a sentinel write to unblock synchronous named pipe waits cleanly on exit.

### Settings HUD (`settings.cpp`, `options_menu.cpp`)
- **Role**: Parses and dumps options inside `user_data/settings.json` using a fast, low-overhead parsing system. Renders sections dynamically inside an overlays settings HUD panel.

### XMP Metadata Tagging (`tagging.cpp`)
- **Status**: **CORE IMPLEMENTED** (multithreaded tag search and persistent binary indexing cache remain pending under the active [implementation_plan.md](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/implementation_plan.md)).
- **Role**: Direct reading and writing of standard-compliant XMP keywords using the Dublin Core `dc:subject` Bag property, replacing the legacy file-based MD5 system.
- **Algorithms / Features**:
  - Uses `IWICFastMetadataEncoder` for fast in-place tag writes.
  - Falls back to `IWICBitmapEncoder` copy-and-reencode (using `IWICMetadataBlockWriter` to copy other EXIF/metadata blocks) if padding is missing.
  - Planned: Highly parallelized background search logic checking `std::filesystem::last_write_time` against a persistent binary `xmp_tag_cache.bin` to ensure search execution in large folders remains instant.

---

## 5. Application Modes & Page Navigation

### Functional Modes
- **Standard Mode** (Default): Full-featured execution. Spawns system tray icon, activates Named Pipe IPC server, and enforces single-instance rules.
- **Lite Mode** (Triggered via `-lite` or `-multiple` flags): Starts as a separate, isolated copy. Skips the tray icon and the Named Pipe IPC listener check, permitting multiple active window overlays.

### Page Flow
- **Page 2: Viewer Page** (Single-image screen with smooth Direct2D transform scaling/rotation, Cyberpunk frame geometries, and metadata HUD overlay).
- **Page 3: Board Page** (PureRef-style virtual canvas interface containing nodes, connection links, custom frames, and text note items).
- **Page 4: Library Page** (Sidebar containing folder list entries alongside a main grid panel displaying folder previews).
- **Page 5: Waterfall Page** (Dynamic column masonry layout with virtual scroll rendering and detailed modal overlay viewer).

---

## 6. Supported Image Formats

Decoded using native **WIC (Windows Imaging Component)** decoder APIs:

| Format | Extension(s) | Decoder Pipeline | Animation Support |
|---|---|---|---|
| **JPEG / JFIF** | `.jpg`, `.jpeg`, `.jfif` | WIC Native Decoder | No |
| **PNG** | `.png` | WIC Native Decoder | No |
| **APNG** | `.apng`, `.png` | WIC + Custom Chunk Parser | Yes |
| **GIF** | `.gif` | WIC Frame Sequencer | Yes |
| **WebP** | `.webp` | WIC Native Codec | Yes (Animated WebP) / No (Static) |
| **TIFF** | `.tif`, `.tiff` | WIC Native Decoder | No |
| **BMP** | `.bmp` | WIC Native Decoder | No |
| **ICO** | `.ico` | WIC Native Decoder | No |
| **AVIF** | `.avif`, `.avf` | WIC (via installed AV1 extension) | No |
| **HEIF / HEIC** | `.heif`, `.heic` | WIC (via installed HEVC extension) | No |
| **JPEG XR** | `.jxr`, `.wdp`, `.hdp` | WIC Native (HD Photo) | No |
| **SVG** | `.svg`, `.svgz` | WIC System Rasterizer | No |

---

## 7. Directory Layout (Native Project)

```
image_viewer/
├── native/
│   ├── CMakeLists.txt                  # CMake project configuration
│   ├── build/
│   │   └── bin/Release/
│   │       ├── lenteuv.exe     # Compiled executable
│   │       └── kv_log.txt              # Diagnostic file-logging output
│   └── src/
│       ├── main.cpp                    # WinMain entry point
│       ├── pch.h / pch.cpp             # Precompiled headers (DirectX, STL)
│       ├── types.h                     # Colors, geometric vectors, clocks
│       ├── window.h / window.cpp       # Frameless Win32 window logic
│       ├── renderer.h / renderer.cpp   # Direct2D rendering context
│       ├── image_loader.h / .cpp       # Async WIC frame decoder
│       ├── animation.h / .cpp          # Easing timelines & animation states
│       ├── app.h / app.cpp             # Application orchestrator, game loop
│       ├── library_data.h / .cpp       # Session, favorites, folders lists
│       ├── settings.h / settings.cpp   # JSON settings file management
│       ├── ipc.h / ipc.cpp             # Named pipe instance control
│       ├── commenting.h / .cpp         # Image region commenting logic
│       ├── tagging.h / tagging.cpp     # XMP dc:subject tagging — raw PNG iTXt + JPEG APP1/XMP injection, async search, binary cache
│       ├── tagging_tests.cpp           # Unit tests for tagging subsystem (4 tests)
│       ├── explorer_integration.h/.cpp # System tray, shell icons, recycle bin
│       ├── i18n.h / i18n.cpp           # Translate framework (loads PO)
│       ├── options_menu.h / .cpp       # HUD settings menu modal
│       ├── viewer_page.h / .cpp        # Page 2: Single-image viewer
│       ├── thumbnail_strip.h / .cpp    # Sub-component: Bottom thumbnails row
│       ├── library_page.h / .cpp       # Page 4: Library folders & grids
│       ├── board_page.h / .cpp         # Page 3: Infinite virtual canvas
│       ├── waterfall_page.h / .cpp     # Page 5: Virtual masonry grid
│       └── resources/
│           ├── app.manifest            # PerMonitorV2 HiDPI compatibility
│           └── app.rc                  # Binary version resources info
├── user_data/                          # Persisted runtime directory
│   ├── settings.json                   # User settings values
│   ├── session.txt                     # Saved folders list
│   ├── fav.txt                         # Favorites list
│   ├── comments.txt                    # Image comment records
│   ├── xmp_tag_cache.bin               # Persistent binary XMP tags cache
│   └── boards/                         # Infinite canvas .kbrd states
├── test/                               # Testing images (PNG, WebP, JPG, etc.)
└── implementation_plan.md              # Active plan for XMP tagging development
```

---

## 8. Test Images Asset Inventory

Located in the [test/](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/test) directory:

| File | Format | Size | Characteristics |
|---|---|---|---|
| `animated.webp` | WebP | 7.4 MB | Heavy animated WebP (multi-frame, transparent background) |
| `apng.apng` | APNG | 258 KB | Dedicated animated APNG file |
| `apng_png.png` | APNG (as `.png`) | 258 KB | APNG extension disguised as standard PNG |
| `copyleft.svg` | SVG | 212 B | Lightweight vector graphics file |
| `icon.ico` | ICO | 17 KB | Standard multi-resolution Windows icon file |
| `kimono.avif` | AVIF | 85 KB | Modern compressed AVIF image |
| `linux.png` | PNG | 818 KB | Large static lossless PNG image |
| `noname.bmp` | BMP | 4.0 MB | Raw uncompressed Windows Bitmap |
| `noname.jpeg` | JPEG | 36 KB | Lossy compressed JPEG image |
| `noname.jpg` | JPG | 45 KB | Standard JPEG image file |
| `noname.tif` / `.tiff` | TIFF | 22 KB | Lossless tagged image file |
| `rubik.gif` | GIF | 763 KB | Standard multi-frame animated GIF |
| `sample.jfif` | JFIF | 81 KB | JPEG File Interchange Format image |
| `sample.tga` | TGA | 818 KB | Targa image file format |
| `static.webp` | WebP | 83 KB | Static WebP image |

---

## 9. Build and Compilation Guide

### Prerequisites
- **Compiler**: Visual Studio 2022 with **"Desktop development with C++"** workload.
- **Build System**: CMake 3.20+.

### cmake path en este sistema
```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S native -B native/build
& $cmake --build native/build --config Release
& $cmake --build native/build --config Release --target tagging_tests
```
> **IMPORTANTE**: `cmake` no está en el PATH del sistema. Debe usarse la ruta absoluta del Visual Studio BuildTools.


---

## 10. Appendix: Estado del sistema de tagging XMP — Errores Resueltos

### Trabajo realizado y errores resueltos en esta sesión (Junio 3, 2026)

#### ✅ Correcciones de errores críticos y optimizaciones

1. **Resolución del Deadlock de Mutex en `WriteTags`**:
   - **Problema**: El ejecutable `tagging_tests.exe` lanzaba una excepción `resource deadlock would occur` debido a que el mutex `m_cacheMutex` era adquirido dos veces en el mismo hilo (en `WriteTags` y posteriormente al llamar a `SaveCache()` dentro del mismo lock).
   - **Solución**: Se separó la adquisición del lock para actualizar la caché en memoria de la llamada a `SaveCache()`. El mutex se libera explícitamente cerrando el bloque de alcance (`scope`) antes de guardar la caché en disco en los bloques de PNG, JPEG, Fast Encoding y Full Transcoding fallback.

2. **Resolución de la Pérdida de Salida en Consola por Codificación (Wide-Character Bug)**:
   - **Problema**: Windows silenciaba la salida estándar debido a caracteres no ASCII (como la raya de diálogo `—`) escritos en flujos de caracteres anchos (`std::wcout`) sin la configuración adecuada, activando el indicador `failbit` de los flujos de consola.
   - **Solución**: Se configuraron los flujos `stdout`/`stderr` en modo UTF-16 (`_O_U16TEXT`) usando `_setmode` en la función `wmain` en [tagging_tests.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/tagging_tests.cpp). Además, se reemplazó la raya de diálogo (`—`) con un guión estándar en las salidas de prueba para garantizar una salida de diagnóstico robusta y legible.

3. **Corrección de Discrepancias en Mayúsculas/Minúsculas en Rutas (Path Casing)**:
   - **Problema**: Variaciones entre rutas en minúsculas y mayúsculas para la letra de unidad (por ejemplo, `e:\...` frente a `E:\...`) causaban fallos en las búsquedas con `std::unordered_map::find`, resultando en falsos fallos en la caché de metadatos.
   - **Solución**: Se implementó una función `NormalizePath` en [tagging.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/tagging.cpp) para unificar separadores y capitalizar la letra de unidad. Se sombrearon las variables `filePath` en los puntos de entrada públicos (`ReadTags`, `WriteTags`, `LoadCache`, `SearchTagsAsync`) con sus versiones normalizadas.

4. **Corrección de Abortos en el Destructor de Hilos (`std::thread`)**:
   - **Problema**: Al destruir el objeto `XmpTagManager`, el hilo de búsqueda `m_searchThread` aún se consideraba ejecutable (joinable), lo que resultaba en la llamada a `std::terminate` y la posterior finalización abrupta de la prueba 4 de cancelación.
   - **Solución**: Se actualizó `CancelSearch()` para llamar a `join()` siempre que el hilo de búsqueda `m_searchThread.joinable()` sea verdadero (true), asegurando la correcta sincronización y finalización limpia del hilo antes de descartar el objeto.

#### ✅ Estado de las Pruebas Unitarias
Todas las pruebas de metadatos XMP en `tagging_tests.exe` pasan exitosamente:
- **Test 1 (JPEG Read/Write)**: PASS. Verifica la inyección de segmentos APP1/XMP sin usar el decodificador/codificador nativo WIC.
- **Test 2 (PNG Read/Write)**: PASS. Verifica la inyección raw de chunks iTXt.
- **Test 3 (Cache Persistence)**: PASS. Verifica la persistencia binaria (`xmp_tag_cache.bin`) y tiempos rápidos de respuesta en búsquedas.
- **Test 4 (Search Cancellation)**: PASS. Comprueba la detención segura de hilos de fondo sin fugas ni abortos de programa.

---

## 11. Appendix: Trabajo completado en la Sesión (Junio 4, 2026)

Se implementaron, compilaron y verificaron con éxito las tareas especificadas en el [implementation_plan.md](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/implementation_plan.md):

### A. Refactorización e Infraestructura de Metadatos
1. **Refactorización de `WicMetadataReaderWriter`**: Se redujo el tamaño de [wic_metadata_reader_writer.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/wic_metadata_reader_writer.cpp) de ~1220 a ~600 líneas al modularizar tareas de apertura de streams, PROPVARIANTs y transcodificación de archivos.
2. **Corrección de persistencia de guardado**: Inicialización ordenada del pool de hilos de `ImageLoader` antes de `XmpTagManager`, garantizando la disponibilidad del factory.
3. **Evitar caché de metadatos de WIC**: Uso de `SHCreateStreamOnFileEx` en los lectores WIC para bypassear la retención de ficheros y caché de metadatos del decodificador.
4. **Caché rápida de consulta**: Validación rápida por marca de tiempo frente al archivo para retornar de inmediato tags de `MetadataCache` sin leer disco.
5. **TaggingModal en Doble Columna**: Layout del panel izquierdo modificado a 2 columnas con mapeo adaptado de mouse.
6. **Interactividad de Scroll en `TaggingModal`**: Arrastre interactivo en el track de scroll derecho.
7. **Soporte para Espacios en Etiquetas**: Modificación en la medición del ancho del cursor/texto reemplazando espacios con espacios de no-rompimiento.
8. **Sincronización con Windows Keywords**: Integración bidireccional vía `IPropertyStore` (`System.Keywords`).

### B. Mejoras y Correcciones de Fine-Tuning
1. **Auto-Ocultamiento de la Isla de Metadatos**: El HUD de metadatos en el Visor se desvanece tras 2 segundos de inactividad del mouse.
2. **Enviar Imagen de Biblioteca al Tablero**: Menú contextual de previews para mandar imagen directo al Tablero Infinito (`Page::Board`).
3. **Eliminar Carpeta de Lista Izquierda**: Menú contextual de carpetas en Biblioteca para remover directorios de la sesión.
4. **Ordenamiento Global (Order-By)**: Modos por Nombre (Asc/Desc), Fecha (Asc/Desc) y Tamaño (Asc/Desc) en Visor, Grid y Waterfall, persistidos en settings.
5. **Eliminación con Tecla [Supr] Preservando Fullscreen**: Diálogo de confirmación nativo para reciclar imagen sin interrumpir el modo pantalla completa.
6. **Ajuste de Ventana al Salir de Fullscreen**: Redimensionamiento y centrado inteligente en ventana normal para coincidir con la proporción de la imagen.
7. **Composición de GIFs con Transparencia**: Mezcla alfa y disposal acumulativo en memoria para evitar el ghosting.
8. **Navegación con PageUp / PageDown**: Desplazamiento por páginas de contenido en Waterfall, Biblioteca y tira de miniaturas (con prioridad de foco).

#### ✅ Estado de las Pruebas Unitarias
Todas las pruebas de metadatos XMP en `tagging_tests.exe` pasan exitosamente:
- **Test 1 (JPEG Read/Write)**: PASS.
- **Test 2 (PNG Read/Write)**: PASS.
- **Test 3 (Cache Persistence)**: PASS.
- **Test 4 (Search Cancellation)**: PASS.
- **Test 5 (Read/Write Extended Metadata)**: PASS (JPEG/PNG).
- **Test 6 (Windows Property Store Keywords Sync)**: PASS.

## 12. Appendix: Plan de Trabajo Pendiente (Siguiente Sesión)
1. **Fase 4: Packaging / Instalador**: Diseñar y empaquetar la aplicación nativa para distribución en sistemas Windows.

---

## 13. Appendix: Trabajo completado en la Sesión (Junio 5, 2026)

Se implementaron correcciones de estabilidad y diseño de interfaz:

### A. Ajuste de Ventana y Estabilidad de Fullscreen
1. **Resolución del Bug de Reducción en Fullscreen**: En [Window::SetFullscreen](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.cpp), se trasladó la asignación de `m_fullscreen = fullscreen` al inicio del método para evitar la desincronización de estado durante el procesamiento del evento síncrono `WM_SIZE` en `OnResize`.
2. **Refactorización de Sizing y Ajuste de Ventana**: En [App::AdjustWindowToImage](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp), se limitó el tamaño mínimo del cliente a `512*512` píxeles. Se implementó el soporte para compensar bordes en ventanas con marcos (`!IsFrameless()`) a través de la función de Win32 `AdjustWindowRectEx`, escalando de forma proporcional el cliente respetando el aspect ratio y clamps del área útil de trabajo del monitor (`rcWork`) antes de centrar la ventana.
3. **Ajuste del Zoom de Renderizado de Imagen**: En [App::CalculateImageRect](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp) y [ViewerPage::CalculateImageRect](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.cpp), se modificó la escala base para que sea `1.0f` (escala 1:1 original, centrada) si la imagen cabe por completo en el área cliente. Si es más grande que la pantalla, se aplica la escala fit-to-screen. El zoom inicial del visor se fijó a `1.0f` (`100%` en imágenes pequeñas y el factor de escala proporcional en imágenes grandes).

### B. Rediseño y Ampliación de TaggingModal (20% Más Grande)
1. **Aumento de Dimensiones del Modal**: En [tagging_modal.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.cpp), se incrementó la caja contenedora del modal de `960x700` a `1152x840` píxeles (aumento del 20% respecto del original). Se ajustaron proporcionalmente los anchos de los paneles izquierdo (Available Tags a `456.0f`) y derecho (Image Tags a `528.0f`), el botón de copiado intermedio (`36.0f` de ancho, posicionado en `leftPane.right + 36.0f`), y la altura de ambos paneles a `645.0f` (`bounds.Height() - 195.0f`).
2. **Legibilidad de las Etiquetas y Textos**:
   * Se incrementó el tamaño de fuente de las palabras clave de `11.0f` a `14.0f` y la altura de las píldoras de `22.0f` a `28.0f`, ampliando los paddings correspondientes para mantener el layout estético.
   * Se aumentó el tamaño de los inputs de búsqueda y creación a `15.0f`, y su altura a `36.0f`.
   * Se aumentó el tamaño de la estrella de clasificación a `22.0f` y las cajas de metadatos (Título, Autor a `40.0f` de alto, Descripción a `90.0f` de alto) con texto a `15.0f` para maximizar la legibilidad en pantallas estándar y de alta resolución.

### C. Correcciones en Modos y UI
1. **Library Mode hover -> Board Shortcut (`3`)**: En el modo biblioteca, presionar la tecla `3` mientras se posiciona el mouse sobre una miniatura carga esa imagen y redirige de forma automática al Board, insertándola en el canvas.
2. **Optimización de Sliders en OptionsMenu**: Se corrigió el lag del slider de opacidad de fondo eliminando el guardado síncrono en disco durante el movimiento (`OnMouseMove`). Ahora el guardado se realiza una única vez en `OnMouseUp` al soltar el slider.
3. **Corrección de Bucle Infinito en OnResize y Ajuste en Ventana**: Se corrigió la recursión infinita en `OnResize` al salir de fullscreen actualizando `m_wasFullscreen` anticipadamente. `AdjustWindowToImage()` escala ahora tanto ancho como alto respetando el aspect ratio y los clamps del área útil de trabajo del monitor (`mi.rcWork`). Se reconstruyen los rectángulos de colisión de los controles de configuración reconstruyendo `OptionsMenu` en `OnResize`.

### D. Mejoras en TaggingModal (Layout Dinámico y Píldoras Envueltas)
1. **Píldoras Envueltas en Available Tags**: La columna izquierda de etiquetas disponibles presenta ahora las etiquetas como píldoras (pills) envueltas dinámicamente según su longitud, idénticas a la derecha pero sin la "x" de cierre. Se adaptó la lógica de detección de scroll, clicks y hover a esta nueva disposición.
2. **Ocultamiento Dinámico sin `xmpTags.json`**: Si el archivo `xmpTags.json` no está presente en el directorio de la imagen abierta, el panel izquierdo (Available Tags) y el botón intermedio `[ > ]` se ocultan por completo. El panel derecho (Image Tags) se expande horizontalmente al ancho total del modal (~30px de margen a los lados), y el control de estrellas y clasificación inferior se alinea de forma dinámica al inicio del panel derecho expandido.

### E. Búsqueda por Tags, Barrido e Indexación en Board y Soporte Alt+F4
1. **Búsqueda por Tags en Board (CTRL+F)**:
   - Se diseñó e implementó un modal de búsqueda por tags que se activa mediante `CTRL+F` en el modo Board.
   - El estilo del modal de búsqueda se homologó con la estética de la aplicación (`TaggingModal` y `OptionsMenu`), utilizando un fondo oscuro translúcido `Color::FromRGBA(15, 15, 20, 245)`, borde cian cyberpunk con esquinas redondeadas y botones de acción tipo Save/Cancel.
   - Cuenta con autocompletado y sugerencias dinámicas obtenidas desde `xmpTags.json`. Al hacer clic en una píldora de sugerencia, se autocompleta el término en el input respetando exclusiones.
   - Soporta queries booleanos avanzados separando términos por comas (ej. `vacaciones, -mar` para incluir `vacaciones` y excluir `mar`).
   - Inserta de forma ordenada las imágenes resultantes en una rejilla (grid) de 3 columnas centrada en la posición de cámara actual del board.
2. **Barrido de Tags del Directorio (CTRL+L)**:
   - Se implementó un barrido indexador activado por `CTRL+L` que ejecuta la lectura asíncrona de metadatos XMP (`ReadTags`) de todos los archivos soportados en el directorio activo a través de un hilo de fondo (`std::thread`).
   - Durante el escaneo, se inmovilizan los controles e interacciones del tablero infinity (excepto para cerrar la app o salir vía Escape) y se dibuja una isla de progreso superior con el porcentaje de análisis.
   - Al finalizar, genera el archivo `xmpTags.json` ordenado con los tags únicos del directorio y persiste la caché de metadatos (`SaveCache`).
   - Si el archivo `xmpTags.json` ya existe en el directorio activo al pulsar `CTRL+L`, muestra un toast flotante temporal "Tags file already exists" de 2.5 segundos con un desvanecimiento suave de 0.5s en la esquina superior derecha.
3. **Cierre Universal con Alt+F4**:
   - Se modificó la intercepción de `WM_SYSKEYDOWN` en `window.cpp` para dejar fluir el atajo `Alt+F4` a `DefWindowProcW`, permitiendo que la ventana responda de forma estándar en Windows cerrándose de manera inmediata y limpia.

---

## 14. Appendix: Trabajo completado en la Sesión (Junio 5, 2026 — Continuación)

│   │       └── kv_log.txt              # Diagnostic file-logging output
│   └── src/
│       ├── main.cpp                    # WinMain entry point
│       ├── pch.h / pch.cpp             # Precompiled headers (DirectX, STL)
│       ├── types.h                     # Colors, geometric vectors, clocks
│       ├── window.h / window.cpp       # Frameless Win32 window logic
│       ├── renderer.h / renderer.cpp   # Direct2D rendering context
│       ├── image_loader.h / .cpp       # Async WIC frame decoder
│       ├── animation.h / .cpp          # Easing timelines & animation states
│       ├── app.h / app.cpp             # Application orchestrator, game loop
│       ├── library_data.h / .cpp       # Session, favorites, folders lists
│       ├── settings.h / settings.cpp   # JSON settings file management
│       ├── ipc.h / ipc.cpp             # Named pipe instance control
│       ├── commenting.h / .cpp         # Image region commenting logic
│       ├── tagging.h / tagging.cpp     # XMP dc:subject tagging — raw PNG iTXt + JPEG APP1/XMP injection, async search, binary cache
│       ├── tagging_tests.cpp           # Unit tests for tagging subsystem (4 tests)
│       ├── explorer_integration.h/.cpp # System tray, shell icons, recycle bin
│       ├── i18n.h / i18n.cpp           # Translate framework (loads PO)
│       ├── options_menu.h / .cpp       # HUD settings menu modal
│       ├── viewer_page.h / .cpp        # Page 2: Single-image viewer
│       ├── thumbnail_strip.h / .cpp    # Sub-component: Bottom thumbnails row
│       ├── library_page.h / .cpp       # Page 4: Library folders & grids
│       ├── board_page.h / .cpp         # Page 3: Infinite virtual canvas
│       ├── waterfall_page.h / .cpp     # Page 5: Virtual masonry grid
│       └── resources/
│           ├── app.manifest            # PerMonitorV2 HiDPI compatibility
│           └── app.rc                  # Binary version resources info
├── user_data/                          # Persisted runtime directory
│   ├── settings.json                   # User settings values
│   ├── session.txt                     # Saved folders list
│   ├── fav.txt                         # Favorites list
│   ├── comments.txt                    # Image comment records
│   ├── xmp_tag_cache.bin               # Persistent binary XMP tags cache
│   └── boards/                         # Infinite canvas .kbrd states
├── test/                               # Testing images (PNG, WebP, JPG, etc.)
└── implementation_plan.md              # Active plan for XMP tagging development
```

---

## 8. Test Images Asset Inventory

Located in the [test/](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/test) directory:

| File | Format | Size | Characteristics |
|---|---|---|---|
| `animated.webp` | WebP | 7.4 MB | Heavy animated WebP (multi-frame, transparent background) |
| `apng.apng` | APNG | 258 KB | Dedicated animated APNG file |
| `apng_png.png` | APNG (as `.png`) | 258 KB | APNG extension disguised as standard PNG |
| `copyleft.svg` | SVG | 212 B | Lightweight vector graphics file |
| `icon.ico` | ICO | 17 KB | Standard multi-resolution Windows icon file |
| `kimono.avif` | AVIF | 85 KB | Modern compressed AVIF image |
| `linux.png` | PNG | 818 KB | Large static lossless PNG image |
| `noname.bmp` | BMP | 4.0 MB | Raw uncompressed Windows Bitmap |
| `noname.jpeg` | JPEG | 36 KB | Lossy compressed JPEG image |
| `noname.jpg` | JPG | 45 KB | Standard JPEG image file |
| `noname.tif` / `.tiff` | TIFF | 22 KB | Lossless tagged image file |
| `rubik.gif` | GIF | 763 KB | Standard multi-frame animated GIF |
| `sample.jfif` | JFIF | 81 KB | JPEG File Interchange Format image |
| `sample.tga` | TGA | 818 KB | Targa image file format |
| `static.webp` | WebP | 83 KB | Static WebP image |

---

## 9. Build and Compilation Guide

### Prerequisites
- **Compiler**: Visual Studio 2022 with **"Desktop development with C++"** workload.
- **Build System**: CMake 3.20+.

### cmake path en este sistema
```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S native -B native/build
& $cmake --build native/build --config Release
& $cmake --build native/build --config Release --target tagging_tests
```
> **IMPORTANTE**: `cmake` no está en el PATH del sistema. Debe usarse la ruta absoluta del Visual Studio BuildTools.


---

## 10. Appendix: Estado del sistema de tagging XMP — Errores Resueltos

### Trabajo realizado y errores resueltos en esta sesión (Junio 3, 2026)

#### ✅ Correcciones de errores críticos y optimizaciones

1. **Resolución del Deadlock de Mutex en `WriteTags`**:
   - **Problema**: El ejecutable `tagging_tests.exe` lanzaba una excepción `resource deadlock would occur` debido a que el mutex `m_cacheMutex` era adquirido dos veces en el mismo hilo (en `WriteTags` y posteriormente al llamar a `SaveCache()` dentro del mismo lock).
   - **Solución**: Se separó la adquisición del lock para actualizar la caché en memoria de la llamada a `SaveCache()`. El mutex se libera explícitamente cerrando el bloque de alcance (`scope`) antes de guardar la caché en disco en los bloques de PNG, JPEG, Fast Encoding y Full Transcoding fallback.

2. **Resolución de la Pérdida de Salida en Consola por Codificación (Wide-Character Bug)**:
   - **Problema**: Windows silenciaba la salida estándar debido a caracteres no ASCII (como la raya de diálogo `—`) escritos en flujos de caracteres anchos (`std::wcout`) sin la configuración adecuada, activando el indicador `failbit` de los flujos de consola.
   - **Solución**: Se configuraron los flujos `stdout`/`stderr` en modo UTF-16 (`_O_U16TEXT`) usando `_setmode` en la función `wmain` en [tagging_tests.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/tagging_tests.cpp). Además, se reemplazó la raya de diálogo (`—`) con un guión estándar en las salidas de prueba para garantizar una salida de diagnóstico robusta y legible.

3. **Corrección de Discrepancias en Mayúsculas/Minúsculas en Rutas (Path Casing)**:
   - **Problema**: Variaciones entre rutas en minúsculas y mayúsculas para la letra de unidad (por ejemplo, `e:\...` frente a `E:\...`) causaban fallos en las búsquedas con `std::unordered_map::find`, resultando en falsos fallos en la caché de metadatos.
   - **Solución**: Se implementó una función `NormalizePath` en [tagging.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/tagging.cpp) para unificar separadores y capitalizar la letra de unidad. Se sombrearon las variables `filePath` en los puntos de entrada públicos (`ReadTags`, `WriteTags`, `LoadCache`, `SearchTagsAsync`) con sus versiones normalizadas.

4. **Corrección de Abortos en el Destructor de Hilos (`std::thread`)**:
   - **Problema**: Al destruir el objeto `XmpTagManager`, el hilo de búsqueda `m_searchThread` aún se consideraba ejecutable (joinable), lo que resultaba en la llamada a `std::terminate` y la posterior finalización abrupta de la prueba 4 de cancelación.
   - **Solución**: Se actualizó `CancelSearch()` para llamar a `join()` siempre que el hilo de búsqueda `m_searchThread.joinable()` sea verdadero (true), asegurando la correcta sincronización y finalización limpia del hilo antes de descartar el objeto.

#### ✅ Estado de las Pruebas Unitarias
Todas las pruebas de metadatos XMP en `tagging_tests.exe` pasan exitosamente:
- **Test 1 (JPEG Read/Write)**: PASS. Verifica la inyección de segmentos APP1/XMP sin usar el decodificador/codificador nativo WIC.
- **Test 2 (PNG Read/Write)**: PASS. Verifica la inyección raw de chunks iTXt.
- **Test 3 (Cache Persistence)**: PASS. Verifica la persistencia binaria (`xmp_tag_cache.bin`) y tiempos rápidos de respuesta en búsquedas.
- **Test 4 (Search Cancellation)**: PASS. Comprueba la detención segura de hilos de fondo sin fugas ni abortos de programa.

---

## 11. Appendix: Trabajo completado en la Sesión (Junio 4, 2026)

Se implementaron, compilaron y verificaron con éxito las tareas especificadas en el [implementation_plan.md](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/implementation_plan.md):

### A. Refactorización e Infraestructura de Metadatos
1. **Refactorización de `WicMetadataReaderWriter`**: Se redujo el tamaño de [wic_metadata_reader_writer.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/wic_metadata_reader_writer.cpp) de ~1220 a ~600 líneas al modularizar tareas de apertura de streams, PROPVARIANTs y transcodificación de archivos.
2. **Corrección de persistencia de guardado**: Inicialización ordenada del pool de hilos de `ImageLoader` antes de `XmpTagManager`, garantizando la disponibilidad del factory.
3. **Evitar caché de metadatos de WIC**: Uso de `SHCreateStreamOnFileEx` en los lectores WIC para bypassear la retención de ficheros y caché de metadatos del decodificador.
4. **Caché rápida de consulta**: Validación rápida por marca de tiempo frente al archivo para retornar de inmediato tags de `MetadataCache` sin leer disco.
5. **TaggingModal en Doble Columna**: Layout del panel izquierdo modificado a 2 columnas con mapeo adaptado de mouse.
6. **Interactividad de Scroll en `TaggingModal`**: Arrastre interactivo en el track de scroll derecho.
7. **Soporte para Espacios en Etiquetas**: Modificación en la medición del ancho del cursor/texto reemplazando espacios con espacios de no-rompimiento.
8. **Sincronización con Windows Keywords**: Integración bidireccional vía `IPropertyStore` (`System.Keywords`).

### B. Mejoras y Correcciones de Fine-Tuning
1. **Auto-Ocultamiento de la Isla de Metadatos**: El HUD de metadatos en el Visor se desvanece tras 2 segundos de inactividad del mouse.
2. **Enviar Imagen de Biblioteca al Tablero**: Menú contextual de previews para mandar imagen directo al Tablero Infinito (`Page::Board`).
3. **Eliminar Carpeta de Lista Izquierda**: Menú contextual de carpetas en Biblioteca para remover directorios de la sesión.
4. **Ordenamiento Global (Order-By)**: Modos por Nombre (Asc/Desc), Fecha (Asc/Desc) y Tamaño (Asc/Desc) en Visor, Grid y Waterfall, persistidos en settings.
5. **Eliminación con Tecla [Supr] Preservando Fullscreen**: Diálogo de confirmación nativo para reciclar imagen sin interrumpir el modo pantalla completa.
6. **Ajuste de Ventana al Salir de Fullscreen**: Redimensionamiento y centrado inteligente en ventana normal para coincidir con la proporción de la imagen.
7. **Composición de GIFs con Transparencia**: Mezcla alfa y disposal acumulativo en memoria para evitar el ghosting.
8. **Navegación con PageUp / PageDown**: Desplazamiento por páginas de contenido en Waterfall, Biblioteca y tira de miniaturas (con prioridad de foco).

#### ✅ Estado de las Pruebas Unitarias
Todas las pruebas de metadatos XMP en `tagging_tests.exe` pasan exitosamente:
- **Test 1 (JPEG Read/Write)**: PASS.
- **Test 2 (PNG Read/Write)**: PASS.
- **Test 3 (Cache Persistence)**: PASS.
- **Test 4 (Search Cancellation)**: PASS.
- **Test 5 (Read/Write Extended Metadata)**: PASS (JPEG/PNG).
- **Test 6 (Windows Property Store Keywords Sync)**: PASS.

## 12. Appendix: Plan de Trabajo Pendiente (Siguiente Sesión)
1. **Fase 4: Packaging / Instalador**: Diseñar y empaquetar la aplicación nativa para distribución en sistemas Windows.

---

## 13. Appendix: Trabajo completado en la Sesión (Junio 5, 2026)

Se implementaron correcciones de estabilidad y diseño de interfaz:

### A. Ajuste de Ventana y Estabilidad de Fullscreen
1. **Resolución del Bug de Reducción en Fullscreen**: En [Window::SetFullscreen](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.cpp), se trasladó la asignación de `m_fullscreen = fullscreen` al inicio del método para evitar la desincronización de estado durante el procesamiento del evento síncrono `WM_SIZE` en `OnResize`.
2. **Refactorización de Sizing y Ajuste de Ventana**: En [App::AdjustWindowToImage](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp), se limitó el tamaño mínimo del cliente a `512*512` píxeles. Se implementó el soporte para compensar bordes en ventanas con marcos (`!IsFrameless()`) a través de la función de Win32 `AdjustWindowRectEx`, escalando de forma proporcional el cliente respetando el aspect ratio y clamps del área útil de trabajo del monitor (`rcWork`) antes de centrar la ventana.
3. **Ajuste del Zoom de Renderizado de Imagen**: En [App::CalculateImageRect](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp) y [ViewerPage::CalculateImageRect](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.cpp), se modificó la escala base para que sea `1.0f` (escala 1:1 original, centrada) si la imagen cabe por completo en el área cliente. Si es más grande que la pantalla, se aplica la escala fit-to-screen. El zoom inicial del visor se fijó a `1.0f` (`100%` en imágenes pequeñas y el factor de escala proporcional en imágenes grandes).

### B. Rediseño y Ampliación de TaggingModal (20% Más Grande)
1. **Aumento de Dimensiones del Modal**: En [tagging_modal.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/tagging_modal.cpp), se incrementó la caja contenedora del modal de `960x700` a `1152x840` píxeles (aumento del 20% respecto del original). Se ajustaron proporcionalmente los anchos de los paneles izquierdo (Available Tags a `456.0f`) y derecho (Image Tags a `528.0f`), el botón de copiado intermedio (`36.0f` de ancho, posicionado en `leftPane.right + 36.0f`), y la altura de ambos paneles a `645.0f` (`bounds.Height() - 195.0f`).
2. **Legibilidad de las Etiquetas y Textos**:
   * Se incrementó el tamaño de fuente de las palabras clave de `11.0f` a `14.0f` y la altura de las píldoras de `22.0f` a `28.0f`, ampliando los paddings correspondientes para mantener el layout estético.
   * Se aumentó el tamaño de los inputs de búsqueda y creación a `15.0f`, y su altura a `36.0f`.
   * Se aumentó el tamaño de la estrella de clasificación a `22.0f` y las cajas de metadatos (Título, Autor a `40.0f` de alto, Descripción a `90.0f` de alto) con texto a `15.0f` para maximizar la legibilidad en pantallas estándar y de alta resolución.

### C. Correcciones en Modos y UI
1. **Library Mode hover -> Board Shortcut (`3`)**: En el modo biblioteca, presionar la tecla `3` mientras se posiciona el mouse sobre una miniatura carga esa imagen y redirige de forma automática al Board, insertándola en el canvas.
2. **Optimización de Sliders en OptionsMenu**: Se corrigió el lag del slider de opacidad de fondo eliminando el guardado síncrono en disco durante el movimiento (`OnMouseMove`). Ahora el guardado se realiza una única vez en `OnMouseUp` al soltar el slider.
3. **Corrección de Bucle Infinito en OnResize y Ajuste en Ventana**: Se corrigió la recursión infinita en `OnResize` al salir de fullscreen actualizando `m_wasFullscreen` anticipadamente. `AdjustWindowToImage()` escala ahora tanto ancho como alto respetando el aspect ratio y los clamps del área útil de trabajo del monitor (`mi.rcWork`). Se reconstruyen los rectángulos de colisión de los controles de configuración reconstruyendo `OptionsMenu` en `OnResize`.

### D. Mejoras en TaggingModal (Layout Dinámico y Píldoras Envueltas)
1. **Píldoras Envueltas en Available Tags**: La columna izquierda de etiquetas disponibles presenta ahora las etiquetas como píldoras (pills) envueltas dinámicamente según su longitud, idénticas a la derecha pero sin la "x" de cierre. Se adaptó la lógica de detección de scroll, clicks y hover a esta nueva disposición.
2. **Ocultamiento Dinámico sin `xmpTags.json`**: Si el archivo `xmpTags.json` no está presente en el directorio de la imagen abierta, el panel izquierdo (Available Tags) y el botón intermedio `[ > ]` se ocultan por completo. El panel derecho (Image Tags) se expande horizontalmente al ancho total del modal (~30px de margen a los lados), y el control de estrellas y clasificación inferior se alinea de forma dinámica al inicio del panel derecho expandido.

### E. Búsqueda por Tags, Barrido e Indexación en Board y Soporte Alt+F4
1. **Búsqueda por Tags en Board (CTRL+F)**:
   - Se diseñó e implementó un modal de búsqueda por tags que se activa mediante `CTRL+F` en el modo Board.
   - El estilo del modal de búsqueda se homologó con la estética de la aplicación (`TaggingModal` y `OptionsMenu`), utilizando un fondo oscuro translúcido `Color::FromRGBA(15, 15, 20, 245)`, borde cian cyberpunk con esquinas redondeadas y botones de acción tipo Save/Cancel.
   - Cuenta con autocompletado y sugerencias dinámicas obtenidas desde `xmpTags.json`. Al hacer clic en una píldora de sugerencia, se autocompleta el término en el input respetando exclusiones.
   - Soporta queries booleanos avanzados separando términos por comas (ej. `vacaciones, -mar` para incluir `vacaciones` y excluir `mar`).
   - Inserta de forma ordenada las imágenes resultantes en una rejilla (grid) de 3 columnas centrada en la posición de cámara actual del board.
2. **Barrido de Tags del Directorio (CTRL+L)**:
   - Se implementó un barrido indexador activado por `CTRL+L` que ejecuta la lectura asíncrona de metadatos XMP (`ReadTags`) de todos los archivos soportados en el directorio activo a través de un hilo de fondo (`std::thread`).
   - Durante el escaneo, se inmovilizan los controles e interacciones del tablero infinity (excepto para cerrar la app o salir vía Escape) y se dibuja una isla de progreso superior con el porcentaje de análisis.
   - Al finalizar, genera el archivo `xmpTags.json` ordenado con los tags únicos del directorio y persiste la caché de metadatos (`SaveCache`).
   - Si el archivo `xmpTags.json` ya existe en el directorio activo al pulsar `CTRL+L`, muestra un toast flotante temporal "Tags file already exists" de 2.5 segundos con un desvanecimiento suave de 0.5s en la esquina superior derecha.
3. **Cierre Universal con Alt+F4**:
   - Se modificó la intercepción de `WM_SYSKEYDOWN` en `window.cpp` para dejar fluir el atajo `Alt+F4` a `DefWindowProcW`, permitiendo que la ventana responda de forma estándar en Windows cerrándose de manera inmediata y limpia.

---

## 14. Appendix: Trabajo completado en la Sesión (Junio 5, 2026 — Continuación)

Se implementaron mejoras de usabilidad, categorización de tags, atajos de teclado y optimizaciones de escalabilidad en BoardPage:

### A. Mejoras de Usabilidad en `TaggingModal`
1. **Acción Directa en Tags Disponibles**: Al hacer click izquierdo en un tag de la lista de disponibles, se añade directamente a la imagen, se vacía la búsqueda, se restablece el scroll izquierdo y se re-enfoca automáticamente el buscador.
2. **Entrada Manual Fluida**: Al crear un tag de forma manual presionando `ENTER` o clickeando en el botón `+`, se mantiene el foco del teclado en el input manual para agilizar inserciones consecutivas.
3. **Soporte de Categorías**: Se diseñó una carga inteligente en `LoadAvailableTags()` para clasificar etiquetas provenientes de archivos `xmpTags.json` estructurados como diccionarios. Se dibujan cabeceras de categorías en color cian cyberpunk y las etiquetas en píldoras tabuladas.
4. **Etiquetado Masivo**: Al seleccionar múltiples imágenes en el tablero y presionar la tecla `T`, se abre el modal en modo multi-imagen, inhabilitando metadatos individuales y rating de estrellas. Al guardar, se calculan las diferencias incrementales (tags añadidos y removidos) y se escriben en disco de manera segura para cada archivo.

### B. Escalabilidad y Optimización del Tablero
1. **Listado Masivo de Directorios (`SHIFT + CTRL + L`)**: Lee de manera no bloqueante todos los archivos de imágenes soportados en la carpeta activa y los posiciona en una cuadrícula masiva de 12 columnas centrada en la cámara. Se ha agregado una dispersión aleatoria uniforme de `[-15.0f, 15.0f]` píxeles en el posicionamiento de las imágenes y una rotación aleatoria uniforme de `[-2.0f, 2.0f]` grados para lograr una sensación visual real y evitar el apilamiento perfecto.
2. **Carga Diferida**: Los elementos añadidos se inicializan con `texture = nullptr` y se decodifican bajo demanda cuando entran en la pantalla.
3. **Optimización de Memoria GPU (VRAM eviction)**: Se limita la carga a un máximo de 150 texturas simultáneas (`MAX_BOARD_TEXTURES = 150`). Las texturas de las imágenes lejanas a la vista de la cámara se descargan automáticamente (`texture = nullptr`), manteniendo el rendimiento con +10,000 elementos.
4. **Liberación de Atajo y Enlaces**: Se modificó la combinación para trazar enlaces de conectores a **ALT + Click**.
5. **Multi-Selección de Elementos**: Se habilitó la selección acumulativa con **CTRL + Click** y **SHIFT + Click**. Los clicks normales en vacío limpian la selección previa.
6. **Diseño Visual Cyberpunk**: Si se selecciona más de una imagen, el rectángulo delimitador externo que las agrupa se renderiza con tiradores y líneas en color cian cyberpunk (`0x00E5FF`) en lugar del naranja estándar.
7. **Filtrado y Dispersión de Búsquedas (CTRL + F)**: Al ejecutar una búsqueda de tags en el tablero, se eliminan del tablero de forma inmediata y segura todas las imágenes que no coinciden con los criterios (junto con sus enlaces asociados). Las nuevas imágenes coincidentes importadas al tablero se ubican en un grid con la misma dispersión y rotación aleatoria de aspecto natural.

---

## 15. Appendix: Trabajo completado en la Sesión - Fine-Tunings y Mejoras de Usabilidad (Junio 5, 2026 — Continuación 2)

Se implementaron y validaron con éxito las siguientes características de fine-tuning solicitadas:

### A. Reorganización de Atajos de Teclado y Teclado Numérico
1. **Reasignación de Teclas**: Los accesos directos de cambio de modo se movieron en su totalidad y de forma exclusiva al teclado numérico (Numpad):
   - `VK_NUMPAD1` -> **Viewer Mode** (cambia al modo Visor).
   - `VK_NUMPAD2` -> **Library Mode** (cambia al modo Library).
   - `VK_NUMPAD3` -> **Waterfall Mode** (cambia al modo Waterfall).
   - `VK_NUMPAD4` -> **Board Mode** (cambia al modo Board).
2. **Fila Numérica Superior**: En la fila numérica superior estándar del teclado, solo se mantiene el atajo `1` dentro del Visor para alternar entre la escala 1:1 original y fit-to-screen (Toggle1To1). Las teclas `1`, `2`, `3`, `4` de la fila numérica superior estándar ya no disparan la navegación entre modos, quedando desligadas de estos.
   
### B. Menú Radial Flotante en Pantalla Completa (Fullscreen)
1. **Lógica de Trigger y Travel Zone**: Al estar en pantalla completa (`F11`), colocar el mouse en la esquina superior derecha activa el menú radial. Se amplió el área de hover de activación a un cuadrante de `100x100` píxeles para lograr un trigger ágil y cómodo. Un área de transición o travel zone superior (`y < 80px`) y una zona de interacción (`x < 200px && y < 200px`) previenen el cierre prematuro del menú.
2. **Visualización y Estética Cyberpunk**: Se renderiza en la esquina superior izquierda un menú radial con un hub central brillante ("KV") y 4 botones radiales animados (`VIS`, `LIB`, `WAT`, `BRD`) con un arco dinámico. Cuenta con tooltips cyberpunk cian que indican el modo y su respectivo atajo. Bloquea clicks e interacciones sobre elementos del fondo mientras está activo.
3. **Corrección de Redibujado en Hover**: Se corrigió el refresco de pantalla al cambiar el estado de visibilidad del menú radial, forzando de forma explícita el redibujado de la ventana (`m_window.RequestRedraw()`) tanto al activarse como al desactivarse, lo cual garantiza que el menú se muestre y oculte en tiempo real de manera consistente.

### C. Prevención de Solapamiento en Board y "New Board"
1. **Posicionamiento Inteligente**: Al realizar una búsqueda masiva de tags (`CTRL+F`) o importar todas las imágenes del directorio (`SHIFT+CTRL+L`), el nuevo grupo de imágenes se coloca de forma automática por debajo de la coordenada vertical más baja (`maxY + spacingY`) de las imágenes existentes, evitando solapamientos de rejillas.
2. **Opción "New Board"**: Se añadió al menú contextual del lienzo vacío del tablero la opción "New Board", que limpia todos los elementos del canvas actual de forma instantánea.

### D. Eliminación Directa de Directorios en Library
1. **Botón Rápido "×"**: En la lista de carpetas del panel izquierdo del modo Biblioteca, cada ítem renderiza ahora un botón "×" de color cian/blanco que cambia a rojo tras recibir el hover. Al hacer clic izquierdo en él, se elimina la carpeta de la sesión de manera directa sin necesidad de invocar el menú contextual.

### E. Menús Contextuales Completos
1. **Mapeo de Atajos**: Los menús contextuales de clic derecho en todos los modos (Viewer, Board, Library, Waterfall) presentan ahora la totalidad de las opciones de interacción y navegación del teclado correspondientes a su respectivo modo de visualización.

---

## 16. Appendix: Corrección de Bugs e Integración de Atajos (Junio 5, 2026 — Continuación 3)

Se corrigieron detalles de navegación de la interfaz de usuario para una experiencia más fluida y consistente:

### A. Corrección del Hover del Menú Radial en Fullscreen
1. **Detección de Esquina Correcta**: En [app.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp), se cambió la condición de activación del hover de la esquina superior derecha (`x > size.width - 100.0f`) a la **esquina superior izquierda** (`x < 100.0f && y < 100.0f`). Esto hace que el área de activación coincida perfectamente con el lugar de dibujado del menú radial en la pantalla, evitando el comportamiento invertido.

### B. Accesos Adicionales a Páginas (Fila Superior)
1. **Atajos Adicionales**: Se habilitaron las teclas estándar de la fila numérica del teclado `'2'`, `'3'` y `'4'` para alternar de página de manera idéntica a las teclas del Numpad:
   - Tecla `2` $\rightarrow$ **Library Mode**
   - Tecla `3` $\rightarrow$ **Waterfall Mode**
   - Tecla `4` $\rightarrow$ **Board Mode**
2. **Preservación del Atajo Zoom (1)**: La tecla numérica estándar `1` no fue modificada, manteniendo su funcionalidad nativa y exclusiva de alternar entre escala 1:1 original y fit-to-screen (`Toggle1To1`) cuando se está en el Visor.
3. **Comportamiento en Library Page**: En [library_page.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/library_page.cpp), se actualizó la detección del atajo para enviar la imagen bajo el cursor directo al Board, de modo que responda tanto a `VK_NUMPAD4` como a la tecla estándar `4`.

### C. Reducción de la Transparencia en la Lista de Carpetas
1. **Mejora de Contraste y Legibilidad**: En [library_page.cpp](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/library_page.cpp), se disminuyó la transparencia de los elementos de la lista de carpetas en un 30% (aumentando su opacidad).
   * El fondo normal pasó de un alpha de `5` a `10`.
   * El fondo hovered pasó de un alpha de `15` a `25` (y su borde de `40` a `52`).
   * El fondo seleccionado pasó de un alpha de `40` a `60` (y su borde de `150` a `195`).
   * El icono por defecto del folder pasó de un alpha de `180` a `234`.
   * Los textos de nombre y descripción aumentaron su opacidad para mejorar la visibilidad (nombre de `0.9f` a `1.0f`, descripción de `0.45f` a `0.58f`).
   Esta modificación no tiene impacto alguno en el desempeño de la renderización acelerada por GPU de la aplicación, pero incrementa significativamente la legibilidad de la lista de directorios.

---

## 17. Appendix: Rebranding completo y Algoritmo de Colocación Radial Espiral (Junio 6, 2026)

En esta sesión se completaron los cambios fundamentales solicitados por el usuario antes del empaquetado final de la aplicación:

### A. Rebranding a LenteUV
1. Se renombraron todas las referencias internas y externas de la aplicación de xviewer y Krumassan Image Viewer a LenteUV (ej. en nombres de proyectos de CMakeLists.txt, ejecutables, registros de Win32 de shell_integrator.cpp, títulos de ventanas en app.cpp y window.h/cpp, manifestos de ensamblado, y pipe IPC single-instance).
2. Se agregaron las atribuciones correctas de lenteuv imageviewer basado en Sergkruma's image viewer tanto en main.cpp como en toda la documentación del proyecto.

### B. Evasión de Colisiones y Posicionamiento Radial Espiral en Board
1. Se reemplazó el esquema anterior de colocar múltiples imágenes al final vertical del tablero por un nuevo Algoritmo de Colocación Radial en Espiral de Arquímedes concéntrico en BoardPage::FindSpiralPosition.
2. Cuando se importan múltiples imágenes (ya sea por listado de directorio masivo SHIFT+CTRL+L o por resultados de consultas de etiquetas CTRL+F), el algoritmo busca posiciones libres a partir del centro de la pantalla del usuario en el espacio virtual, validando colisiones contra los cuadros delimitadores (Bounding Boxes) de los elementos existentes y otros elementos recién añadidos en la misma transacción, aplicando una holgura segura de 40px.
3. Esto garantiza que las imágenes se coloquen orgánicamente alrededor de los elementos existentes, sin taparse entre sí ni tapar lo ya creado.

### C. Remoción de CommentsManager
1. Se eliminó por completo el CommentsManager y las referencias a comments_manager.cpp y comments_manager.h en la configuración de compilación de CMake y en el orquestador principal App.

### D. Verificación y Validación
1. Se compiló con éxito el binario final en modo Release en native/build/bin/Release/lenteuv.exe.
2. Se ejecutaron y pasaron exitosamente todas las pruebas de tagging y metadatos en tagging_tests.exe.
3. Se actualizó la guía de uso native (NATIVE_USAGE_GUIDE.md) para reflejar las nuevas características y el comportamiento espiral del tablero.




