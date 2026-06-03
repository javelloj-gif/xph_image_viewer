# Krumassan Image Viewer — Complete Native Project Context

> **Purpose**: This document provides the complete, consolidated context for any agent or developer needing to understand, maintain, or build the native C++20 implementation of Krumassan Image Viewer. It contains only native details and eliminates legacy Python context.

---

## 1. Project Identity

| Field | Value |
|---|---|
| **Name** | Krumassan Image Viewer (Native) |
| **Version** | v0.90 Alpha (C++20 Migration) |
| **Author** | Sergei Krumas ([github.com/sergkrumas](https://github.com/sergkrumas)) |
| **License** | GPL v2+ |
| **Language** | C++20 (MSVC x64 compiler) |
| **UI Backend** | Direct2D 1.3 / Direct3D 11 / DirectWrite / DirectComposition |
| **Platform** | Windows 10 (Build 18362+) / Windows 11 |
| **Status** | **Fases 1-3 estructuralmente COMPLETAS**. Core engine, UI pages, settings, IPC, shell integration, commenting, i18n, Options HUD completamente implementados. El sistema de tagging XMP (`tagging.cpp`) está implementado con sus 4 componentes principales: `ReadTags`, `WriteTags`, `SearchTagsAsync` y `LoadCache`/`SaveCache`. Se implementó la inyección raw de chunks PNG (iTXt) y segmentos JPEG (APP1/XMP) para bypassear las limitaciones del encoder WIC. Los bugs de deadlock por mutex, problemas de encoding en consola, discrepancias de mayúsculas/minúsculas en rutas y crashes del destructor de hilos han sido completamente resueltos, y todos los tests pasan con éxito. Phase 4 (Packaging/Installer) pendiente. |
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
| [board_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.cpp) | 664 | 25,446 | Coordinator for virtual infinite workspace page |
| [wic_metadata_reader_writer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/wic_metadata_reader_writer.cpp) | 600 | 29,812 | WIC Dublin Core query parser & metadata transcoder fallback |
| [thumbnail_strip.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/thumbnail_strip.cpp) | 578 | 22,900 | Bottom thumbnail strip row with selection/reordering |
| [renderer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/renderer.cpp) | 558 | 17,354 | Direct2D/D3D11 rendering backend wrapper |
| [options_menu.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/options_menu.cpp) | 489 | 18,129 | In-app HUD settings interface modal page |
| [window.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.cpp) | 469 | 13,518 | Win32 window wrapper & hit-testing for transparent frames |
| [image_loader.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/image_loader.cpp) | 383 | 13,442 | Multi-threaded WIC file decoder |
| [folder_data_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/folder_data_manager.cpp) | 369 | 12,137 | History, favorites, and session persistence management |
| [types.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/common/types.h) | 299 | 11,290 | Geometric vectors, colors, logging macros, and clocks |
| [tagging_tests.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/tagging_tests.cpp) | 298 | 11,976 | Unit tests suite verifying tagging cache, R/W, and search |
| [comments_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/comments_manager.cpp) | 268 | 9,147 | Image comment persistence in comments.txt |
| [single_instance_ipc.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/single_instance_ipc.cpp) | 239 | 7,307 | Named pipe single-instance connection & file path IPC |
| [png_chunk_injector.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/png_chunk_injector.cpp) | 221 | 9,054 | Binary chunk injection of XMP iTXt metadata in PNG streams |
| [settings.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/settings.cpp) | 220 | 11,965 | JSON settings file serializer/deserializer |
| [shell_integrator.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/shell/shell_integrator.cpp) | 215 | 7,297 | COM bindings, system tray menus, & Recycle Bin wrapper |
| [renderer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/renderer.h) | 200 | 9,026 | Direct2D/D3D11 rendering backend wrapper |
| [async_directory_scanner.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/async_directory_scanner.cpp) | 196 | 7,877 | Thread-pool recursive directory scan tag indexer |
| [i18n_engine.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/i18n/i18n_engine.cpp) | 180 | 6,000 | Translation loader for gettext PO files |
| [board_serializer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_serializer.cpp) | 176 | 6,371 | Serialization of BoardItems to .board binary format |
| [metadata_cache.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/metadata_cache.cpp) | 173 | 6,238 | In-memory database & file-based index (KTAG v1) serialization |
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
| [board_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.h) | 82 | 2,985 | Coordinator for virtual infinite workspace page |
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
| [metadata_cache.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/metadata_cache.h) | 38 | 1,208 | In-memory database & file-based index (KTAG v1) serialization |
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
│   │       ├── KrumassanViewer.exe     # Compiled executable
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

## 11. Appendix: Plan de Refactorización de Estructura y Buenas Prácticas (Junio 3, 2026)

Se ha redactado y guardado en [implementation_plan.md](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/implementation_plan.md) un plan completo de refactorización estructural del proyecto para su posterior ejecución.
Este plan contempla:
1. **Agrupación por dominio**: Clasificación de los archivos fuente planos de `native/src/` en subcarpetas (`core/`, `rendering/`, `loader/`, `metadata/`, `library/`, `board/`, `viewer/`, `waterfall/`, `ui/`, `shell/`, `i18n/`, `common/`).
2. **Eliminación de clases spaghetti (>1000 líneas)**: Desacoplamiento de `board_page.cpp` y `tagging.cpp` en componentes modulares con responsabilidad única y de tamaño acotado (ej. `BoardCanvas`, `BoardSelectionManager`, `BoardTextEditor`, `BoardSerializer`, `PngChunkInjector`, `JpegSegmentInjector`, `MetadataCache`, `AsyncDirectoryScanner`).
3. **Nombres y descripciones de clases**: Estandarización de las cabeceras descriptivas de todas las clases usando placeholders (`[IMAGE_VIEWER_APP_NAME]`) en lugar del nombre real de la aplicación, exceptuando la clase `main.cpp` que mantiene la mención original ("based on Krumassan's Image Viewer").

