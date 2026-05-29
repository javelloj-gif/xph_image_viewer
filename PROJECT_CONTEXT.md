# Krumassan Image Viewer — Complete Project Context

> **Purpose**: This document provides full context for any agent or developer needing to understand, modify, or rewrite the Krumassan Image Viewer codebase.

---

## 1. Project Identity

| Field | Value |
|---|---|
| **Name** | Krumassan Image Viewer |
| **Version** | v0.90 Alpha |
| **Author** | Sergei Krumas ([github.com/sergkrumas](https://github.com/sergkrumas)) |
| **License** | GPL v2+ |
| **Language** | Python 3.10+ (original) + C++20 (native rewrite in progress) |
| **UI Framework** | PyQt5 (5.15.7) |
| **Platform** | Windows 8.1/10/11 |
| **Status** | Prototype (Python) + Native C++20 rewrite in progress (Phase 1 complete; Phase 2 in progress: board, library, thumbnail strip, and viewer pages implemented; waterfall_page and start_page are pending, 2026-05-29 18:41) |
| **Inspiration** | Google Picasa Photo Viewer (discontinued Feb 2016) + PureRef |

---

## 2. Codebase Metrics

### File Sizes (sorted descending by lines)

| File | Lines | Bytes | Role |
|---|---|---|---|
| `board.py` | 9,128 | 392KB | Board page (PureRef clone) — canvas, items, serialization |
| `_viewer.pyw` | 6,299 | 265KB | Main window — all pages, events, rendering orchestrator |
| `library_data.py` | 2,625 | 101KB | Data layer — folders, files, sessions, favorites |
| `control_panel.py` | 1,678 | 71KB | Bottom panel — thumbnails, buttons, context menus |
| `settings_handling.py` | 1,243 | 50KB | Settings system — JSON persistence, UI window |
| `board_plugins/elastiques_nodes.py` | 1,107 | 35KB | Board plugin — elastic node system |
| `_utils.py` | 996 | 35KB | Shared utilities — image loading, math, drawing helpers |
| `hidapi_adapter.py` | 949 | 38KB | Gamepad input — PS4 DualShock, ShanWan |
| `tagging.py` | 943 | 35KB | Image tagging system |
| `board_plugins/front_tracker.py` | 890 | 32KB | Board plugin — front tracking |
| `colorpicker.py` | 887 | 34KB | Color picker widget |
| `board_note_item.py` | 799 | 34KB | Board text note items |
| `line_eyedropper_tool.py` | 685 | 29KB | Line eyedropper tool |
| `board_align_distribution_ui.py` | 580 | 22KB | Board alignment tools |
| `commenting.py` | 578 | 20KB | Image commenting |
| `pixmaps_generation.py` | 516 | 16KB | Runtime pixmap generation |
| `ipc_utils.py` | 495 | 18KB | IPC dialog utilities |
| `autoscroll.py` | 452 | 21KB | Auto-scroll feature |
| `i18n_l10n_tools.py` | 427 | 14KB | i18n tooling |
| `board_plugins/tangents_chain_plugin.py` | 409 | 14KB | Board plugin — tangent circles |
| `help_text.py` | 342 | 11KB | F1 help overlay |
| `app_copy_prevention.py` | 182 | 9KB | Single-instance enforcement via sockets |
| **Other files (17)** | ~800 | — | Minor utilities, plugins, scripts |
| `VIEWER_DLL_PROJECT/viewer_dll.cc` | 269 | 9KB | C++ extension — Explorer window file list |
| **TOTAL** | **33,419** | — | **39 Python files** |

---

## 3. Entry Points

| Entry File | Mode | Description |
|---|---|---|
| `viewer.pyw` | Default | Sets `DEBUG=False`, calls `_viewer.main()` |
| `viewer_standard.pyw` | Standard | Sets `SUPER_LITE=False`, `SUPRESS_RERUN=True` |
| `_viewer.pyw` directly | Debug | Sets `DEBUG=True` automatically |

All entry points call `_viewer.main()` → `AppMixin.APP_main()`.

### Startup Flow
1. Parse command-line arguments (`-lite`, `-standard`, `-board`, `-viewer`, `-library`, `-waterfall`, `-startpage`, `-frame`, `-rerun`, `-aftercrash`, `path`)
2. If not `SUPER_LITE`: handle Windows double-process bug via `-rerun`
3. Load settings from `user_data/settings.json`
4. Create `QApplication`
5. If not lite mode: run IPC server/client check via sockets (1 second timeout)
6. Load library data (session, favorites, comments, tags, boards)
7. Generate runtime pixmaps (icons, buttons)
8. Create `MainWindow` (frameless or windowed)
9. Create system tray icon (standard mode only)
10. Prepare initial page based on args/settings
11. Handle input data (file/folder path)
12. Enter Qt event loop

---

## 4. Application Modes

| Mode | Flag | Description |
|---|---|---|
| **Lite** | `-lite` or `SUPER_LITE=True` | No IPC server, no tray icon, no session loading, different icon. For "open in copy" feature. |
| **Standard** | `-standard` | Full features: tray, metadata, comments, history, tags |
| **Debug** | `DEBUG=True` | Auto-detected from filename `_viewer.pyw`, opens debug page |

---

## 5. Page Architecture

### Page System

Pages are defined in `MainWindow.pages` (inner class):

```python
class pages():
    START_PAGE = 1
    VIEWER_PAGE = 2
    BOARD_PAGE = 3
    LIBRARY_PAGE = 4
    WATERFALL_PAGE = 5
```

Each page has:
- A **draw callback** (e.g., `viewerpage_draw_callback`) set in `set_page_transparency_and_draw_callback()`
- A **transparency value** from settings
- Custom behavior in all event handlers: `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`, `wheelEvent`, `mouseDoubleClickEvent`, `keyPressEvent`, `keyReleaseEvent`, `contextMenuEvent`

Page switching is handled by `change_page()`, which also manages control panel visibility.

### Rendering Flow per Frame

```
paintEvent(event)
├── If BW filter active:
│   ├── Render to QImage via _paintEvent()
│   ├── Convert to Format_Grayscale16
│   └── Draw grayscale to screen
├── Else:
│   └── _paintEvent(event, painter)
│       ├── Set render hints (Antialiasing, SmoothPixmapTransform)
│       ├── Draw background (black, with opacity for frameless mode)
│       ├── current_page_draw_callback(painter, event)
│       │   ├── Viewer: draw_viewer_content() + region_zoom_in_draw()
│       │   ├── Library: draw_library()
│       │   ├── Board: board_draw()
│       │   ├── Waterfall: draw_waterfall()
│       │   └── Start: draw_startpage()
│       ├── Draw autoscroll indicator
│       ├── Draw eyedropper tool info
│       ├── Draw center label
│       ├── Draw corner menus (top-left = page, top-right = close/minimize)
│       ├── Draw thread info (thumbnail generation progress)
│       ├── Draw 32-bit warning
│       ├── Draw console output
│       ├── Draw noise cells (optional effect)
│       ├── Draw gamepad status
│       └── Draw setting notification
└── Draw FPS indicator
```

---

## 6. Key Classes & Mixins

### MainWindow (line 905 of `_viewer.pyw`)

```python
class MainWindow(QMainWindow,
    UtilsMixin,      # Window style, cursor helpers
    AppMixin,        # App lifecycle, IPC, restart, tray
    BoardMixin,      # Board page (9128 lines in board.py)
    HelpWidgetMixin, # F1 help overlay
    CommentingMixin, # Image region comments
    TaggingMixing,   # Image tagging
    LineEyedropperToolMixin,  # Color picker line tool
    XYZMixin,        # XYZ dispatcher (39 lines)
    AutoscrollMixin, # Browser-style autoscroll
):
```

### LibraryData (line 77 of `library_data.py`)

Singleton pattern via `__new__`. Manages:
- Folder list with current selection
- Favorites (virtual folder)
- Comments (virtual folder)
- Tags (virtual folders)
- Session persistence
- Thumbnail/preview generation threads
- Board data save/load
- View history

### BoardItem (line 256 of `board.py`)

Represents any item on the Board page canvas:

| Type Constant | Description |
|---|---|
| `ITEM_IMAGE` | Image from a file |
| `ITEM_AV` | Audio/video file |
| `ITEM_FOLDER` | Folder reference |
| `ITEM_GROUP` | Group of items |
| `ITEM_NODE` | Graph node |
| `ITEM_LINK` | Connection between nodes |
| `ITEM_FRAME` | Visual frame/border |
| `ITEM_NOTE` | Text note |
| `ITEM_EDITING_TABLE` | Editing table |

Each item has: `position`, `scale_x/y`, `rotation`, `width`, `height`, `visible`, `_selected`, `_touched`, `_transform_stack`, `_crop_data_stack`.

### Settings (in `settings_handling.py`)

- Stored as JSON in `user_data/settings.json`
- ~100+ settings covering all pages, transparency, animation, gamepad, etc.
- Settings window rendered via custom `QWidget` with `paintEvent`
- Postponed-save mechanism (debounced timers)
- Page-specific settings matrix

---

## 7. Module Dependency Map

```
viewer.pyw / viewer_standard.pyw
  └── _viewer.pyw (main orchestrator)
        ├── _utils.py (shared utilities)
        │     ├── PyQt5 (QtWidgets, QtGui, QtCore, QtSvg)
        │     ├── PIL / Pillow + pillow_avif
        │     ├── apng (APNG reader)
        │     ├── psutil (process info)
        │     ├── win32con, win32api, win32com.shell
        │     └── hashlib, ctypes, itertools, etc.
        ├── library_data.py (data management)
        │     ├── settings_handling.py (Settings class)
        │     ├── commenting.py (CommentingLibraryDataMixin)
        │     ├── board.py (BoardLibraryDataMixin)
        │     └── tagging.py (TaggingLibraryDataMixin)
        ├── board.py (BoardMixin - 9128 lines)
        │     ├── board_note_item.py (BoardTextEditItemMixin)
        │     ├── board_align_distribution_ui.py (ToolWindow, AlignType)
        │     ├── hidapi_adapter.py (gamepad input)
        │     ├── cbor2 (board serialization)
        │     └── board_plugins/ (7 dynamic plugins)
        ├── control_panel.py (ControlPanel widget)
        ├── help_text.py (HelpWidgetMixin)
        ├── commenting.py (CommentingMixin)
        ├── tagging.py (TaggingMixing)
        ├── line_eyedropper_tool.py (LineEyedropperToolMixin)
        ├── autoscroll.py (AutoscrollMixin)
        ├── xyz_dispatcher.py (XYZMixin)
        ├── colorpicker.py (ColorPicker widget)
        ├── pixmaps_generation.py (runtime icons)
        ├── app_copy_prevention.py (IPC single-instance)
        ├── ipc_utils.py (IPC dialogs)
        ├── i18n_l10n_tools.py (translations)
        └── viewer_dll.pyd (C++ Explorer integration)
```

---

## 8. Supported Image Formats

| Format | Extension(s) | Loader | Animation |
|---|---|---|---|
| JPEG | .jpg, .jpeg | QImageReader | No |
| JFIF | .jfif | QImageReader | No |
| PNG | .png | QImageReader | No |
| APNG | .apng, .png | `apng` library → QPixmap | Yes |
| GIF | .gif | QMovie (Qt) | Yes |
| WebP (static) | .webp | QImageReader | No |
| WebP (animated) | .webp | QMovie (Qt) | Yes |
| SVG | .svg, .svgz | QSvgRenderer → rasterize | No |
| TIFF | .tif, .tiff | QImageReader | No |
| TGA | .tga | QImageReader | No |
| BMP | .bmp | QImageReader | No |
| ICO | .ico | QImageReader | No |
| AVIF | .avif, .avf | Pillow + pillow-avif | No |
| HEIF/HEIC | .heif, .heic | Pillow + pillow-avif | No |

Format detection is done by file extension. Animation detection for APNG and WebP uses binary header inspection (`is_apng_file_animated()`, `is_webp_file_animated()` in `_utils.py`).

---

## 9. Threading Model

### GIL Limitation (Critical)

From ARCHITECTURE.md:
> Due to the GIL in the C implementation of Python, QThread threads do not work as theoretically intended — they are executed sequentially, not in parallel. Data processing code has been written assuming threads are sequential. Running in true multithreaded mode may cause crashes.

### Threads

| Thread | Class | Purpose |
|---|---|---|
| Main Thread | Qt Event Loop | All UI, rendering, event handling |
| Thumbnail Thread | `ThumbnailsPreviewsThread(QThread)` | Generate thumbnails + previews for a folder |

`ThumbnailsPreviewsThread` maintains a class-level `threads_pool` to prevent duplicate threads for the same folder. It signals the main window via `update_signal` (pyqtSignal).

---

## 10. IPC / Single Instance

Implemented in `app_copy_prevention.py` via TCP sockets:

1. On startup, try to connect to a localhost port (1 second timeout)
2. If connection succeeds → another instance is running → send the file path → exit
3. If connection fails → become the server → listen for future requests
4. Server receives file paths and calls `handle_input_data()` to open them

Lite mode skips IPC entirely.

---

## 11. Serialization Formats

| Data | Format | File |
|---|---|---|
| Settings | JSON | `user_data/settings.json` |
| Session (open folders) | Line-delimited text | `user_data/session.txt` |
| Favorites | Line-delimited text | `user_data/fav.txt` |
| Comments | Line-delimited text | `user_data/comments.txt` |
| Tags | Folder-based | `user_data/tags/` |
| Board data | CBOR2 binary | `user_data/boards/*.board` |
| Image rotations | INI-like | `viewer.ini` (per-folder) |
| View history | In-memory only | Not persisted |

---

## 12. Plugin System (Board Page)

Plugins are Python files in `board_plugins/`:

| Plugin | Lines | Description |
|---|---|---|
| `template_plugin.py` | 99 | Template for new plugins |
| `tangents_chain_plugin.py` | 409 | Interactive tangent circles |
| `elastiques_nodes.py` | 1,107 | Elastic node graph |
| `front_tracker.py` | 890 | Front tracking visualization |
| `watch_tower.py` | 141 | Watch tower |
| `deprecated_bookmarks.py` | 159 | Deprecated bookmarks |
| `ideas_forge.py` | 31 | Ideas forge (stub) |

Plugins are loaded dynamically via `importlib`. Each plugin exposes callbacks matching `BoardCallbacksNames` (mousePressEvent, paintEvent, etc.) and is wrapped in a `PluginInfo` object.

---

## 13. C++ Extension (`viewer_dll.pyd`)

Source: `VIEWER_DLL_PROJECT/viewer_dll.cc` (269 lines)

**Single function**: `getFileListFromExplorerWindow(fullpaths=True)` → Returns a Python list of strings.

Uses COM interfaces (`IShellWindows`, `IWebBrowserApp`, `IShellBrowser`, `IShellView`, `IFolderView`, `IShellFolder`) to enumerate files in the currently active Windows Explorer tab. This is used at startup to determine what files are visible in the Explorer window that launched the viewer.

Built with `setup.py build_ext --inplace` using distutils (Python â‰¤3.11) or the `prepare_pyd.py` script.

---

## 14. i18n Infrastructure

| Component | Details |
|---|---|
| Translation framework | Python `gettext` |
| Base template | `locales/base.pot` |
| Languages | English (default), Russian, German (WIP), French (WIP), Italian (WIP), Spanish (WIP) |
| String marking | `_("string")` via `__import__('builtins').__dict__['_'] = __import__('gettext').gettext` |
| Tools | `i18n_l10n_tools.py` (427 lines) |

---

## 15. Python Dependencies

| Package | Purpose | Install Command |
|---|---|---|
| **PyQt5** | UI framework, rendering, events | `pip install PyQt5` |
| **Pillow** | AVIF/HEIC/TGA decoding | `pip install Pillow` |
| **pillow-avif-plugin** | AVIF format support | `pip install pillow-avif-plugin` |
| **psutil** | Process info, memory monitoring | `pip install psutil` |
| **pywin32** | Shell operations, win32 API | `pip install pywin32` |
| **cbor2** | Board file serialization | `pip install cbor2` |
| **hidapi** | Gamepad/HID device input | `pip install hidapi` |
| **apng** | Animated PNG reading | `pip install apng` |
| **noise** | Perlin noise (optional visual effect) | `pip install noise` |

---

## 16. Known Performance Issues

1. **Paint-time data creation**: Per ARCHITECTURE.md, private data is created/updated inside `paintEvent` instead of in event handlers. This adds overhead to every frame.
2. **Full window repaint**: No dirty-rect optimization — the entire window is redrawn each frame.
3. **CPU-only rendering**: All `QPainter` operations are software-rasterized, including image scaling with `SmoothPixmapTransform`.
4. **GIL serialization**: Thumbnail generation thread is effectively sequential due to GIL.
5. **Multiple format conversions**: AVIF/HEIC: PIL Image → raw bytes → QImage → QPixmap (3 copies).
6. **No frustum culling**: Board page draws all items regardless of viewport visibility.
7. **Animation timer conflicts**: Multiple `QTimer` instances can interfere (documented in MANUAL_TESTING.md).

---

## 17. Test Assets

Located in `test/`:

| File | Format | Size |
|---|---|---|
| animated.webp | WebP (animated) | 7.4 MB |
| apng.apng | APNG | 258 KB |
| apng_png.png | APNG as .png | 258 KB |
| copyleft.svg | SVG | 212 B |
| icon.ico | ICO | 17 KB |
| kimono.avif | AVIF | 85 KB |
| linux.png | PNG | 818 KB |
| noname.bmp | BMP | 4.0 MB |
| noname.jpeg | JPEG | 36 KB |
| noname.jpg | JPG | 45 KB |
| noname.tif | TIFF | 22 KB |
| noname.tiff | TIFF | 22 KB |
| rubik.gif | GIF (animated) | 763 KB |
| sample.jfif | JFIF | 81 KB |
| sample.tga | TGA | 818 KB |
| static.webp | WebP (static) | 83 KB |

---

## 18. File/Folder Layout

```
image_viewer/
├── viewer.pyw                    # Entry point (default)
├── viewer_standard.pyw           # Entry point (standard mode)
├── _viewer.pyw                   # Main application (6299 lines)
├── _utils.py                     # Shared utilities
├── board.py                      # Board page (PureRef clone)
├── board_note_item.py            # Board text items
├── board_align_distribution_ui.py # Board alignment tools
├── library_data.py               # Data management layer
├── control_panel.py              # Bottom thumbnail/button panel
├── settings_handling.py          # Settings system + UI
├── help_text.py                  # F1 help overlay
├── commenting.py                 # Image commenting
├── tagging.py                    # Image tagging
├── autoscroll.py                 # Auto-scroll feature
├── line_eyedropper_tool.py       # Eyedropper tool
├── colorpicker.py                # Color picker widget
├── hidapi_adapter.py             # Gamepad input
├── pixmaps_generation.py         # Runtime pixmap generation
├── app_copy_prevention.py        # Single-instance IPC
├── ipc_utils.py                  # IPC dialog utilities
├── i18n_l10n_tools.py            # i18n tooling
├── on_windows_startup.py         # Windows startup registration
├── xyz_dispatcher.py             # XYZ mixin (39 lines)
├── get_winreg_file.pyw           # Registry helper
├── viewer_dll.pyd                # Compiled C++ extension
├── image_viewer.ico              # App icon (standard)
├── image_viewer_lite.ico         # App icon (lite mode)
├── install_dependencies.bat      # pip install script
├── crash.log                     # Crash log output
│
├── board_plugins/                # Board page plugins
│   ├── template_plugin.py
│   ├── tangents_chain_plugin.py
│   ├── elastiques_nodes.py
│   ├── front_tracker.py
│   ├── watch_tower.py
│   ├── deprecated_bookmarks.py
│   └── ideas_forge.py
│
├── VIEWER_DLL_PROJECT/           # C++ extension source
│   ├── viewer_dll.cc             # C++ source (269 lines)
│   ├── setup.py                  # Build script
│   └── prepare_pyd.py            # Build helper
│
├── locales/                      # Translations
│   ├── base.pot                  # Template
│   ├── ru/                       # Russian
│   ├── de/                       # German (WIP)
│   ├── fr/                       # French (WIP)
│   ├── it/                       # Italian (WIP)
│   └── es/                       # Spanish (WIP)
│
├── resources/                    # UI resources (switch buttons)
├── cursors/                      # Custom cursor files
├── icon_source/                  # Icon source files (Blender)
├── docs/                         # Screenshots for README
├── test/                         # Test image files (16 formats)
├── user_data/                    # Runtime data
│   ├── settings.json
│   ├── history.log
│   └── boards/
│
├── ARCHITECTURE.md               # Architecture notes
├── MANUAL_TESTING.md             # Manual test checklist
├── CHANGELOG.md                  # Changelog (286KB)
├── README.md                     # English readme
├── README_ru.md                  # Russian readme
└── LICENSE                       # GPL v2+
```

---

## 12. Native C++ Rewrite Status (2026-05-29)

The project is being rewritten as a native C++20 application. See `NATIVE_CONVERSION_PLAN.md` for the full plan.

### Conversion Progress

| Phase | Status | Path |
|---|---|---|
| Phase 1: Core Engine | **COMPLETE** | `native/` |
| Phase 2: Application Pages | **IN PROGRESS** (Viewer, Thumbnail Strip, Library, and Board Pages complete; Waterfall and Start Pages pending) | `native/src/viewer_page.*`, `native/src/library_page.*`, `native/src/board_page.*` (Done), `native/src/waterfall_page.*`, `native/src/start_page.*` (Pending) |
| Phase 3: Advanced Features | Pending | — |
| Phase 4: Packaging | Pending | — |

### Native Project Structure (`native/`)

```
native/
├── CMakeLists.txt                  # CMake build (MSVC x64, C++20, static DirectX)
├── build/
│   └── bin/Release/
│       ├── KrumassanViewer.exe     # 331 KB (D3D11 FL 11.1)
│       ├── kv_log.txt              # Runtime diagnostic log
│       └── test/                  # 16 test images copied here
└── src/
    ├── pch.h / pch.cpp             # Precompiled header
    ├── types.h                     # ComPtr, PointF/RectF/Color, logging, Timer
    ├── window.h / window.cpp       # Win32 frameless + DirectComposition transparency
    ├── renderer.h / renderer.cpp   # D3D11 + D2D1 + DirectWrite + DirectComposition
    ├── image_loader.h / .cpp       # WIC async loader, thread-safe GPU queue
    ├── animation.h / animation.cpp # 12 easing curves, AnimationManager, FrameSequencer
    ├── app.h / app.cpp             # Application orchestrator, game-loop render
    ├── main.cpp                    # WinMain entry point
    └── resources/
        ├── app.manifest            # DPI awareness, Windows 10/11 compat
        └── app.rc                  # Version info
```

### Key Architectural Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Transparency | DirectComposition + `WS_EX_NOREDIRECTIONBITMAP` | Avoids QOpenGLWidget flicker from Python attempt |
| Presentation | Flip-Model DXGI (`FLIP_SEQUENTIAL`) | Eliminates tearing; replaces GDI/PyQt rendering |
| Thread model | Decode on workers, GPU upload on render thread only | Prevents D3D context crashes from Python QThread attempt |
| GPU bitmaps | `ID2D1Bitmap1` (VRAM) | One-time upload, zero-cost scaling per frame |
| Build | CMake + MSVC 2022, no external dependencies (Phase 1) | Simplest possible build chain |

### Runtime Performance (Phase 1, single image)
- Working set: ~57 MB (GPU resources loaded)
- Frame rate: game-loop (up to 60fps, idles when static)
- Init time: < 1 second including image load
