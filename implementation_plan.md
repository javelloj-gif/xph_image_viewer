# Refactoring Plan — Project Restructuring & Coding Best Practices

This document outlines a plan to refactor the flat native C++20 codebase into structured domain folders, enforce clean coding principles (Single Responsibility Principle, descriptive naming, class line counts strictly under 1,000 lines), and define standardized header comments for every class using placeholders.

> [!IMPORTANT]
> **No execution requested**: This plan is intended for review and planning purposes only and will be executed by other agents.

---

## User Review Required

> [!WARNING]
> - **Build System Update**: Moving source files into subfolders requires adjusting file paths inside [CMakeLists.txt](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/CMakeLists.txt).
> - **Header Inclusions**: Refactoring file paths requires updating `#include` directives throughout all C++ header and source files. We will use relative includes from `native/src/` (e.g., `#include "core/app.h"`) and configure CMake's `target_include_directories` to include `src/`.

---

## Proposed Changes

We will group the flat `native/src/` directory into structured packages by domain, and break up the large spaghetti files ([board_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board_page.cpp) and [tagging.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/tagging.cpp)) into single-responsibility units under 1,000 lines.

### Domain Folder Structure

```
native/src/
├── common/             # Common structures, vectors, and compiler utilities
├── core/               # App controller, Window loop, IPC, and Settings
├── rendering/          # GPU Rendering backend, animations, and curves
├── loader/             # Multi-threaded file decoders and frame sequencer
├── metadata/           # Tag manager, injectors, async scanners, and comments
├── library/            # Recent folders persistence and Library page
├── board/              # Infinite canvas, selection tools, note text editing, and serialization
├── viewer/             # Main single-image viewer page
├── waterfall/          # Virtual scrolling masonry grid page
├── ui/                 # Bottom thumbnails row and Option overlay HUDs
├── shell/              # Tray icons, Shell context paths, and Recycle bin
└── i18n/               # Po translation loader
```

---

### Component Refactoring & Relocation Details

For each file and class to be created or modified, the required description header is listed below. In accordance with user requirements, the name of the application has been replaced by the placeholder `[IMAGE_VIEWER_APP_NAME]` in all file descriptions, except for `main.cpp` which maintains the credit based on *Krumassan's Image Viewer*.

---

### 1. Core Package (`core/`)

Responsible for application lifecycle, system configurations, and windowing.

#### [NEW] [app.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.h) / [app.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/app.cpp)
* **Class**: `App`
* **Description**:
  ```cpp
  // App is the central coordinator for the [IMAGE_VIEWER_APP_NAME] application,
  // managing the main event loop, routing system messages, and page navigation.
  ```
* **Responsibility**: Orchestrate the Win32 window message loop and delegate actions to page components.

#### [NEW] [window.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.h) / [window.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/window.cpp)
* **Class**: `Window`
* **Description**:
  ```cpp
  // Window encapsulates the Win32 window creation and message dispatching
  // for [IMAGE_VIEWER_APP_NAME], providing support for transparent overlays.
  ```
* **Responsibility**: Handle low-level Win32 window creation, Custom Hit-Testing, drag-and-drop registration, and DPI scaling events.

#### [NEW] [settings.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/settings.h) / [settings.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/settings.cpp)
* **Class**: `Settings`
* **Description**:
  ```cpp
  // Settings handles serialization, deserialization, and runtime updates
  // of user preferences for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Load and save configurations from `settings.json`.

#### [NEW] [single_instance_ipc.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/single_instance_ipc.h) / [single_instance_ipc.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/core/single_instance_ipc.cpp) *(Renamed from `ipc.cpp`)*
* **Class**: `SingleInstanceIpc`
* **Description**:
  ```cpp
  // SingleInstanceIpc implements named pipe communication to ensure a single
  // instance of [IMAGE_VIEWER_APP_NAME] runs and handles file opening requests.
  ```
* **Responsibility**: Handle named-pipe client/server IPC to prevent multiple instances and handle external path requests using a poison-pill shutdown handler.

---

### 2. Rendering Package (`rendering/`)

Responsible for GPU hardware-accelerated drawing and animations.

#### [NEW] [renderer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/renderer.h) / [renderer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/renderer.cpp)
* **Class**: `Renderer`
* **Description**:
  ```cpp
  // Renderer wraps Direct2D and Direct3D 11 initialization and provides
  // hardware-accelerated drawing APIs for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: DXGI swap-chains, DirectComposition target blending, and Direct2D primitive draw calls.

#### [NEW] [animation_controller.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/animation_controller.h) / [animation_controller.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/animation_controller.cpp) *(Renamed from `AnimationManager`)*
* **Class**: `AnimationController`
* **Description**:
  ```cpp
  // AnimationController schedules, updates, and dispatches properties
  // interpolated over time for active UI transitions in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Manage active UI transitions and prevent timer collision flags.

#### [NEW] [animation_curve.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/animation_curve.h) / [animation_curve.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/rendering/animation_curve.cpp) *(Extracted from `animation.cpp`)*
* **Class**: `AnimationCurve`
* **Description**:
  ```cpp
  // AnimationCurve provides mathematical easing curves to drive smooth
  // UI animations in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Implement easing curves (cubic, expo, quadratic, elastic back, etc.).

---

### 3. Loader Package (`loader/`)

Responsible for background decoders and image frame timing.

#### [NEW] [image_loader.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/image_loader.h) / [image_loader.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/image_loader.cpp)
* **Class**: `ImageLoader`
* **Description**:
  ```cpp
  // ImageLoader implements multi-threaded image decoding using
  // Windows Imaging Component (WIC) for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Decode pixel files asynchronously, queuing raw bytes for main thread GPU texture uploading.

#### [NEW] [frame_sequencer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/frame_sequencer.h) / [frame_sequencer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/loader/frame_sequencer.cpp)
* **Class**: `FrameSequencer`
* **Description**:
  ```cpp
  // FrameSequencer manages frame timing, speed adjustments, and repeat counts
  // for animated images in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Track active frame indices and elapsed frame delays for GIF/APNG/WebP animations.

---

### 4. Metadata Package (`metadata/`)

Responsible for tag read/write operations, async search indexing, and comments. This refactors the **1,522-line** `tagging.cpp` file into single-responsibility modules under 400 lines each.

#### [NEW] [xmp_tag_manager.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/xmp_tag_manager.h) / [xmp_tag_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/xmp_tag_manager.cpp)
* **Class**: `XmpTagManager`
* **Description**:
  ```cpp
  // XmpTagManager acts as the primary service interface to query, modify,
  // and search image metadata tags in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Main entry/facade coordinating metadata requests, mapping calls to the cache, reader/writer, and directory scanners.

#### [NEW] [metadata_cache.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/metadata_cache.h) / [metadata_cache.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/metadata_cache.cpp)
* **Class**: `MetadataCache`
* **Description**:
  ```cpp
  // MetadataCache maintains in-memory and on-disk indexed tags to speed up
  // directory searches in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Load, save, and maintain the binary database (`user_data/xmp_tag_cache.bin` using the `KTAG` v1 format) with path normalization.

#### [NEW] [png_chunk_injector.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/png_chunk_injector.h) / [png_chunk_injector.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/png_chunk_injector.cpp)
* **Class**: `PngChunkInjector`
* **Description**:
  ```cpp
  // PngChunkInjector injects raw XMP iTXt metadata chunks directly
  // into PNG streams for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Direct binary manipulation of PNG files to write/replace XMP XML payload packets and compute CRC32 checksums.

#### [NEW] [jpeg_segment_injector.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/jpeg_segment_injector.h) / [jpeg_segment_injector.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/jpeg_segment_injector.cpp)
* **Class**: `JpegSegmentInjector`
* **Description**:
  ```cpp
  // JpegSegmentInjector inserts APP1 metadata segments containing XMP packets
  // into JPEG files for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Split JPEG streams at segments and insert APP1/XMP markers containing custom XML tag payloads.

#### [NEW] [wic_metadata_reader_writer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/wic_metadata_reader_writer.h) / [wic_metadata_reader_writer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/wic_metadata_reader_writer.cpp)
* **Class**: `WicMetadataReaderWriter`
* **Description**:
  ```cpp
  // WicMetadataReaderWriter leverages the WIC API to parse and write XMP
  // Dublin Core tags in standard-compliant files for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Read and write XMP bag elements using Direct WIC query reader interfaces, falling back to full block transcoding if needed.

#### [NEW] [async_directory_scanner.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/async_directory_scanner.h) / [async_directory_scanner.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/async_directory_scanner.cpp)
* **Class**: `AsyncDirectoryScanner`
* **Description**:
  ```cpp
  // AsyncDirectoryScanner manages thread-pool directory scans to locate
  // tagged images asynchronously in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Scan files recursively on a background manager thread and parse cache-missed files in parallel using CPU-bounded threads.

#### [NEW] [comments_manager.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/comments_manager.h) / [comments_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/metadata/comments_manager.cpp) *(Renamed from `commenting.cpp`)*
* **Class**: `CommentsManager`
* **Description**:
  ```cpp
  // CommentsManager parses, updates, and saves spatial annotations/comments
  // associated with images in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Maintain folder comments inside `comments.txt`.

---

### 5. Library Package (`library/`)

Responsible for folder navigation database, browsing statistics, and Library UI grid.

#### [NEW] [folder_data_manager.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/folder_data_manager.h) / [folder_data_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/folder_data_manager.cpp) *(Renamed from `library_data.cpp`)*
* **Class**: `FolderDataManager`
* **Description**:
  ```cpp
  // FolderDataManager maintains the persistence of recent paths, favorites,
  // and session folders browsed in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Read/write `session.txt` and `fav.txt` tracking browsed directories.

#### [NEW] [library_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/library_page.h) / [library_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/library/library_page.cpp)
* **Class**: `LibraryPage`
* **Description**:
  ```cpp
  // LibraryPage implements the panel view for browsing directory collections
  // in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Render left-hand scrollable folder entries and right-hand preview cards.

---

### 6. Board Canvas Package (`board/`)

Responsible for the Miro/PureRef infinite canvas. This breaks down the **1,061-line** `board_page.cpp` file into clean, modular components.

#### [NEW] [board_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.h) / [board_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_page.cpp)
* **Class**: `BoardPage`
* **Description**:
  ```cpp
  // BoardPage coordinates the virtual infinite workspace UI page
  // in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Act as main page handler receiving mouse and keyboard inputs, forwarding events to selection, text editor, canvas, and file loaders.

#### [NEW] [board_canvas.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_canvas.h) / [board_canvas.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_canvas.cpp)
* **Class**: `BoardCanvas`
* **Description**:
  ```cpp
  // BoardCanvas controls the viewport transformations, coordinate projections,
  // and background grid patterns for the board interface of [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Keep zoom and pan states, convert screen/world coordinate spaces, and draw checkerboard or alignment grids.

#### [NEW] [board_item.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_item.h)
* **Struct**: `BoardItem`
* **Description**:
  ```cpp
  // BoardItem represents the structural state and geometry of a single element
  // (image, label, link, or note) on the canvas of [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Data container holding type, bounds, colors, link anchors, texture loaders, and visibility flags.

#### [NEW] [board_item_renderer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_item_renderer.h) / [board_item_renderer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_item_renderer.cpp)
* **Class**: `BoardItemRenderer`
* **Description**:
  ```cpp
  // BoardItemRenderer draws nodes, image frames, and connection links
  // on the virtual canvas of [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Direct2D drawing logic for nodes, connection links, editing status, and textured images.

#### [NEW] [board_selection_manager.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_selection_manager.h) / [board_selection_manager.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_selection_manager.cpp)
* **Class**: `BoardSelectionManager`
* **Description**:
  ```cpp
  // BoardSelectionManager controls picking, rectangular box selection,
  // and translation/scaling of elements on the canvas of [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Manage rubber-band selection, drag translations of selected item sets, handle scaling vectors from TL/TR/BL/BR control boundaries.

#### [NEW] [board_text_editor.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_text_editor.h) / [board_text_editor.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_text_editor.cpp)
* **Class**: `BoardTextEditor`
* **Description**:
  ```cpp
  // BoardTextEditor processes text edits, cursor positions, and string formatting
  // for active notes on the board of [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Manage caret blinking times, text input sequences, character insertion/erasing, and dimension calculations for note items.

#### [NEW] [board_serializer.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_serializer.h) / [board_serializer.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/board/board_serializer.cpp)
* **Class**: `BoardSerializer`
* **Description**:
  ```cpp
  // BoardSerializer handles saving and reading canvas layouts using the custom
  // binary format of [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Serialize/deserialize `BoardItem` structures to `.board` layout files.

---

### 7. Single Image Viewer Package (`viewer/`)

Responsible for main viewer scaling transforms and details overlays.

#### [NEW] [viewer_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.h) / [viewer_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/viewer/viewer_page.cpp)
* **Class**: `ViewerPage`
* **Description**:
  ```cpp
  // ViewerPage implements the single-image display screen with advanced zoom,
  // pan, and visual grids for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Handle fit-to-screen transforms, cyberpunk boundary graphics, thirds divisions, and Ctrl+drag spotlights.

---

### 8. Waterfall Package (`waterfall/`)

Virtual columns layout views.

#### [NEW] [waterfall_page.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/waterfall/waterfall_page.h) / [waterfall_page.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/waterfall/waterfall_page.cpp)
* **Class**: `WaterfallPage`
* **Description**:
  ```cpp
  // WaterfallPage implements the virtual scrolling masonry grid layout
  // for browsing folder contents in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Virtual scroll column coordinate maps and modal image viewers.

---

### 9. Shared UI Overlay Package (`ui/`)

Reusable panels and controls.

#### [NEW] [thumbnail_strip.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/thumbnail_strip.h) / [thumbnail_strip.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/thumbnail_strip.cpp)
* **Class**: `ThumbnailStrip`
* **Description**:
  ```cpp
  // ThumbnailStrip manages the reorderable row of image previews displayed
  // at the bottom of the screen in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Draw selection markers, re-ordering layout guides, and handle thumbnail click events.

#### [NEW] [options_menu.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/options_menu.h) / [options_menu.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/ui/options_menu.cpp)
* **Class**: `OptionsMenu`
* **Description**:
  ```cpp
  // OptionsMenu renders the in-app HUD interface overlay to toggle options
  // and settings for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Render settings categories, checkboxes, sliders, and keybinding panels.

---

### 10. Shell Integration Package (`shell/`)

Low level COM and Explorer interactions.

#### [NEW] [shell_integrator.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/shell/shell_integrator.h) / [shell_integrator.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/shell/shell_integrator.cpp) *(Renamed from `explorer_integration.cpp`)*
* **Class**: `ShellIntegrator`
* **Description**:
  ```cpp
  // ShellIntegrator coordinates Windows shell operations, COM bindings,
  // system tray menus, and Recycle Bin deletes for [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Query explorer processes for paths, handle recycle-bin moves, and system tray menu events.

---

### 11. Internationalization Package (`i18n/`)

Gettext po translations.

#### [NEW] [i18n_engine.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/i18n/i18n_engine.h) / [i18n_engine.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/i18n/i18n_engine.cpp) *(Renamed from `i18n.cpp`)*
* **Class**: `I18nEngine`
* **Description**:
  ```cpp
  // I18nEngine loads and queries gettext-compatible localization files
  // for language switching in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Translate wide strings using `.po` runtime tables.

---

### 12. Utilities Package (`common/`)

Shared structs, precompiled headers.

#### [NEW] [types.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/common/types.h)
* **Description**:
  ```cpp
  // Types defines the core primitives, vectors, and logging macro functions
  // shared across modules in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Colors, geometric rectangles, high-resolution clocks, and file log writers.

#### [NEW] [pch.h](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/common/pch.h) / [pch.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/common/pch.cpp)
* **Description**:
  ```cpp
  // Pch configures precompiled compiler headers to accelerate builds
  // of [IMAGE_VIEWER_APP_NAME].
  ```

---

### 13. Root Source Files (Entry point and Tests)

#### [MODIFY] [main.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/main.cpp)
* **Description**:
  ```cpp
  // Main is the WinMain entry point of the project, which is based on
  // Krumassan's Image Viewer.
  ```
* **Responsibility**: Parse parameters, initialize COM, verify single instances via IPC, and initialize `App`.
* **Important**: Only this class keeps the explicit credit "based on Krumassan's Image Viewer".

#### [MODIFY] [tagging_tests.cpp](file:///E:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/native/src/tagging_tests.cpp)
* **Description**:
  ```cpp
  // tagging_tests implements test assertions to verify metadata reading,
  // writing, and search behaviors in [IMAGE_VIEWER_APP_NAME].
  ```
* **Responsibility**: Unit-test core tags R/W, binary cache loads, and asynchronous cancel search pipelines.

---

## Verification Plan

### Automated Build Checks
Validate compilation using the custom Visual Studio CMake path:
```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S native -B native/build
& $cmake --build native/build --config Release
& $cmake --build native/build --config Release --target tagging_tests
```

### Unit Tests
Verify all reorganized metadata tagging components pass:
```powershell
.\native\build\Release\tagging_tests.exe
```

### Functional Layout Validation
Run the main viewer to check page events, named-pipe IPC instances, and canvas drawing are functional under their new locations:
```powershell
.\native\build\bin\Release\KrumassanViewer.exe
```
