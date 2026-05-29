# Native Windows 11 x64 Image Viewer — Conversion Plan

> **Last Updated**: 2026-05-29 18:35 | **Phase 1**: ✅ COMPLETE | **Phase 2**: 🟨 IN PROGRESS (Viewer, Thumbnail Strip, Library, and Board Pages complete, verified and updated by Antigravity Agent)

## Implementation Status

| Phase | Status | Evidence |
|---|---|---|
| **Phase 1: Core Engine** | ✅ **COMPLETE** | `KrumassanViewer.exe` 331KB — JPEG/AVIF/GIF/WebP all verified |
| **Phase 2: Application Pages** | 🟨 **IN PROGRESS** | `viewer_page` (with conditional default zoom), `thumbnail_strip`, `library_page`, and `board_page` implemented & verified |
| **Phase 3: Advanced Features** | ⬜ Pending | settings, library_data, IPC, gamepad, Explorer integration |
| **Phase 4: Packaging** | ⬜ Pending | Installer, file associations, MSIX |

### Phase 1 Build Evidence
```
Compiler : MSVC 19.43.34809.0
SDK      : Windows SDK 10.0.22621.0
D3D      : Feature Level 11.1 (0xB100)  <- DirectComposition verified active
Output   : native/build/bin/Release/KrumassanViewer.exe (331 KB)
Tested   : Windows 11, 168 DPI (HiDPI per-monitor)
Formats  : JPEG check  PNG check  AVIF check  GIF(100fr) check  WebP(88fr) check  BMP check  TIFF check
```


## Problem Statement

**Krumassan Image Viewer** is a feature-rich image viewer (Picasa 3 clone + PureRef-like board) built in **Python 3.10+ / PyQt5**. The codebase spans **33,419 lines** across **39 Python files** plus a C++ Python extension DLL. While functionally complete, its performance is fundamentally limited by:

1. **QPainter software rendering** — All drawing (images, thumbnails, boards, UI overlays) happens on the CPU via `QPainter`. No GPU acceleration.
2. **Python's GIL** — `QThread`-based parallelism is serialized. Thumbnail generation, image decoding, and UI updates all compete for the single Python thread.
3. **PIL-based image decoding** — Formats like AVIF/HEIC go through Pillow → raw bytes → QImage → QPixmap, with multiple memory copies.
4. **Paint-time data computation** — Per the architecture doc, private data is created/updated *inside* `paintEvent`, lowering FPS.
5. **No incremental rendering** — The entire window is repainted on every frame, even for small changes.

The goal is to produce a **native Windows 11 x64 executable** with **GPU-accelerated rendering** that preserves all current functionality while dramatically improving performance for zoom, pan, board mode, waterfall layout, and animation playback.

---

## Mitigating Optimization Pitfalls (Lessons from Python Optimization)

To ensure the native C++ application delivers a perfect user experience without reproducing the issues encountered during the Python optimization attempt, the native implementation **must** strictly adhere to the following architectural rules:

### 1. Preserving the Translucent Background
*   **The Issue**: The transparent/translucent window effect was lost due to limitations in QOpenGLWidget composition.
*   **C++ Prevention**:
    *   Initialize the Win32 window with the `WS_EX_NOREDIRECTIONBITMAP` extended style (or `WS_EX_LAYERED` for Windows 8.1 compatibility).
    *   Use the Desktop Window Manager (DWM) API to extend the frame into the client area using `DwmExtendFrameIntoClientArea`.
    *   Set the background fill of the DXGI swap chain to transparent (`float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };`) and clear the render target with zero alpha.
    *   This forces Windows to blend the window client area with the desktop background using native hardware composition.

### 2. Eliminating Flickering and Tearing
*   **The Issue**: Context switching between GUI redraws and background events caused parpadeo (flickering).
*   **C++ Prevention**:
    *   Configure the DXGI Swap Chain with **Flip-Model** presentation modes (`DXGI_SWAP_EFFECT_FLIP_DISCARD` or `DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL`). This bypasses GDI completely and eliminates tearing/flickering.
    *   Render all elements (images, boards, grid lines, text overlays) to an off-screen Direct2D render target buffer, and call `IDXGISwapChain::Present` exactly once per frame (VSync-aligned) to update the screen atomically.

### 3. Guaranteeing High Performance
*   **The Issue**: CPU scaling of heavy images and manual calculation of coordinates inside the paint loop caused frame drops.
*   **C++ Prevention**:
    *   Upload all decoded images to GPU VRAM as `ID2D1Bitmap1` textures immediately after loading. Scaling, translations, and rotations will be computed on the GPU via hardware vertex transforms.
    *   Implement frustum culling on the Board Page: calculate the world coordinates of the viewport and check if each item's pre-computed bounding box intersects it before passing draw commands to Direct2D.

### 4. Robust and Async Thumbnail Loading
*   **The Issue**: Threading and QPixmap instantiation issues on non-GUI threads caused thumbnails to fail to load.
*   **C++ Prevention**:
    *   All file decoding (via WIC) must occur on background worker threads (using a native thread pool).
    *   Once a WIC bitmap is decoded on a background thread, pass the raw pixel data to the main thread via a thread-safe message queue.
    *   The main thread (which holds the D3D11 device and Direct2D context) will instantiate the `ID2D1Bitmap` texture from the raw pixel data. This isolates all GPU texture creation to the render thread, preventing driver context issues and failed image loads.

---

## User Review Required

> **IMPORTANT — Technology Choice**: This plan proposes **C++ with Win32 + Direct2D/Direct3D 11** as the target stack. An alternative would be **Rust + wgpu** or **C++ with Vulkan**. Direct2D was chosen because it's the native Windows 2D GPU API, has excellent image format support via WIC, and is the most mature option for this use case. Please confirm this direction.

> **IMPORTANT — UI Framework**: For dialogs (settings, tags, comments), the plan proposes **Dear ImGui** rendered via Direct3D for in-app UI, plus native Win32 dialogs for file/folder pickers. An alternative is using a lightweight UI library like **Ultralight** or embedding a webview. Please confirm preference.

> **WARNING — Scope**: This is a **complete rewrite**, not a port. The Python codebase (~33K lines) will be replaced with C++ (~estimated 25-40K lines). This is a **multi-month project**. The plan is structured in 4 phases to allow incremental delivery and testing.

> **WARNING — Plugin System**: The Board page has a Python plugin system (`board_plugins/`) that loads `.py` files dynamically. In a native app, plugins would need to be either compiled C++ DLLs, Lua/Python-scripted, or dropped entirely. Please advise on priority.

---

## Open Questions

1. **Target language**: C++ (proposed) vs Rust vs C#/.NET? Each has trade-offs for Windows native development.
2. **Build system**: CMake (proposed) vs MSBuild/Visual Studio project vs Meson?
3. **Plugin system priority**: Should the Board page plugin system be preserved? If yes, via embedded scripting (Lua/Python) or compiled DLLs?
4. **i18n approach**: Current app uses gettext `.pot/.po` files. Keep gettext in C++ or switch to Windows resource strings?
5. **Board file format**: Currently uses CBOR2 serialization. Keep CBOR or migrate to a different format (e.g., FlatBuffers, MessagePack, protobuf)?
6. **Minimum Windows version**: Plan targets Windows 10 1903+ (for Direct2D 1.3 and DXGI 1.5). Is Windows 8.1 support still required?
7. **Animated format support**: Current app supports GIF, animated WebP, and APNG. Should we also add video playback (MP4, etc.) via Media Foundation?

---

## Proposed Technology Stack (Native)

| Layer | Technology | Rationale |
|---|---|---|
| **Language** | C++20 | Maximum control, mature Windows ecosystem, interop with Win32 |
| **Build** | CMake + MSVC (Visual Studio 2022) | Standard for C++ on Windows |
| **Windowing** | Win32 API | Frameless windows, translucent background, multi-monitor, tray icon |
| **2D Rendering** | Direct2D 1.3 | GPU-accelerated 2D, hardware bitmap caching, transform stacks |
| **3D Backend** | Direct3D 11 | Required by Direct2D, also useful for custom shaders |
| **Image Loading** | WIC (Windows Imaging Component) | Native GPU-ready decoding for JPEG, PNG, TIFF, BMP, GIF, WEBP, AVIF, HEIF, SVG |
| **Text Rendering** | DirectWrite | Subpixel antialiased text, font fallback, layout |
| **Animation** | WIC + custom frame cache | GIF/APNG/WebP animation with GPU texture atlas |
| **UI Widgets** | Dear ImGui (Direct3D backend) | Settings window, color picker, tag editor |
| **Serialization** | nlohmann/json + custom binary | Settings (JSON), boards (binary) |
| **Gamepad** | XInput 1.4 + Raw HID | PS4/PS3 gamepad support |
| **IPC** | Named pipes | Single-instance enforcement, open-file requests |
| **i18n** | ICU or gettext for C++ | Preserve existing .po translations |
| **Installer** | WiX Toolset or NSIS | Windows installer with file associations |

---

## Python → Native Dependency Mapping

| Python Package | Purpose | Native Replacement |
|---|---|---|
| **PyQt5** | UI framework, rendering, events | Win32 + Direct2D |
| **Pillow** | AVIF/HEIC/TGA decoding, PIL→QPixmap | WIC (Windows Imaging Component) |
| **pillow-avif-plugin** | AVIF format support | WIC AVIF codec (Windows 10+) |
| **psutil** | Process info, memory monitoring | Win32 API (GetProcessMemoryInfo) |
| **pywin32** | Shell operations, win32 API | Direct Win32 API |
| **cbor2** | Board file serialization | tinycbor or custom binary format |
| **hidapi** | Gamepad/HID device input | XInput + Raw HID API |
| **apng** | Animated PNG reading | WIC + custom APNG parser |
| **noise** | Perlin noise (optional visual effect) | stb_perlin or custom |

---

## Phased Implementation

### Phase 1: Core Engine (Foundation) — ✅ COMPLETE

Build the minimal rendering engine that can display a single image with zoom/pan/rotate.

#### `src/main.cpp` ✅ DONE
Application entry point: `wWinMain`, COM init (STA), DPI awareness, command-line parsing, App creation and main loop.

#### `src/pch.h` / `src/pch.cpp` ✅ DONE
Precompiled header with all Windows SDK, DirectX, WIC, DirectComposition, and STL includes.

#### `src/types.h` ✅ DONE
Core types: `ComPtr<T>`, `PointF/SizeF/SizeU/RectF/Color`, `KV_HR_RETURN` HRESULT macros, `ScopedComInit`, `Timer`, file-based logging (`kv_log.txt`).

#### `src/window.h` / `src/window.cpp` ✅ DONE
Win32 frameless window:
- `WS_POPUP` + `WS_EX_NOREDIRECTIONBITMAP` + DirectComposition for per-pixel alpha
- `DwmExtendFrameIntoClientArea` for correct DWM blending
- Per-monitor DPI awareness V2 (`WM_DPICHANGED`)
- Custom NCHITTEST for frameless resize/move (6px border)
- Fullscreen toggle (saves/restores window placement)
- File drag-and-drop (`WM_DROPFILES`)
- Event callback system (lambda-based, no inheritance)

#### `src/renderer.h` / `src/renderer.cpp` ✅ DONE
Direct2D/Direct3D 11 rendering backend:
- D3D11 device → DXGI Flip-Model swap chain (`FLIP_SEQUENTIAL`, premultiplied alpha) → D2D1 device context
- DirectComposition binding for true per-pixel transparency
- Transform stack: `PushTranslation/Scale/Rotation` + `PopTransform`
- Clip stack: `PushClip/PopClip`
- Primitives: `DrawBitmap`, `FillRect/DrawRect`, `FillRoundedRect`, `DrawEllipse`, `DrawLine`, `DrawText`
- Resource creation: `CreateBitmapFromWIC`, `CreateBitmapFromMemory` (render thread only)
- Device-lost recovery, swap chain resize

#### `src/image_loader.h` / `src/image_loader.cpp` ✅ DONE
WIC-based image loading:
- 4-thread pool for async background decoding
- Thread-safe `PendingTexture` queue — GPU upload on render thread only
- Formats: JPEG, PNG, APNG, GIF, WebP, TIFF, BMP, TGA, ICO, AVIF, HEIF, JFIF, JXR
- Animated image frame extraction (GIF frame delays, loop count from metadata)
- `ProcessPendingTextures()` — call once per frame on render thread

#### `src/animation.h` / `src/animation.cpp` ✅ DONE
Animation framework:
- 12 easing functions: linear, quad, cubic, expo, back, elastic (in/out/inout variants)
- `AnimationManager` with named animations — new animation with same name cancels old (prevents timer conflicts from MANUAL_TESTING.md)
- `FrameSequencer` — GIF/WebP/APNG frame timing with speed multiplier, pause/resume, loop counting

#### `src/app.h` / `src/app.cpp` ✅ DONE
Application orchestrator:
- Game-loop render at up to 60fps; idles via `MsgWaitForMultipleObjects` when static
- Image display: fit-to-screen with zoom/pan/rotate
- Checkerboard under transparent images (cached GPU bitmap)
- FPS overlay, file info bar, D2D-drawn close button
- Folder navigation (arrow keys, Ctrl+wheel)
- Animated GIF/WebP/APNG playback (Space=pause, R=rotate, 0=reset)
- Drag-and-drop file/folder loading

#### `src/resources/app.manifest` + `src/resources/app.rc` ✅ DONE
DPI awareness manifest (PerMonitorV2), version info, Windows 10/11 compatibility declaration.

---

### Phase 2: Application Pages

#### `src/viewer_page.h / viewer_page.cpp` ✅ DONE
Viewer page (Picasa clone):
- GPU-rendered image with zoom (1%–10,000%), pan, rotation
- Zoom anchor locked to window center (ignores cursor)
- Double-click behavior: toggles fullscreen mode in main viewer
- Key '1' toggle between actual 1:1 scale and fit-to-screen
- Rectangle selection magnifier (Ctrl+drag) with spotlight animation
- Animated content playback with progress bar
- Rule-of-thirds overlay
- Cyberpunk corner decorations with exclude path geometries
- Checkerboard under transparent images
- WASD + arrow key movement
- Conditional default zoom: fit-to-screen if the image size is superior to screen size, otherwise 1:1 pixel scale

#### `src/thumbnail_strip.h / thumbnail_strip.cpp` ✅ DONE
Bottom thumbnail panel (replaces `control_panel.py`):
- GPU-rendered thumbnail row with smooth scrolling animation
- Centered-on-current logic
- Click to select, Ctrl+click for multi-select reordering
- Red insertion cursor
- Hover transparency
- Context menu

#### `src/library_page.h / library_page.cpp` ✅ DONE
Library page:
- Left panel: scrollable folder list with selection highlight
- Right panel: dynamic-column grid of previews
- Hover-to-preview in left panel
- Folder management (add, remove, update)

#### `src/board_page.h / board_page.cpp` ✅ DONE
Board page (PureRef/Miro clone) — **largest component**:
- 2D canvas with GPU-accelerated zoom/pan
- Board items: images, folders, groups, nodes, links, frames, text notes
- Selection system (click, box-select, lasso)
- Transform tools (move, scale, rotate) with undo stack
- Copy/paste with hierarchy
- Serialization to/from binary format (.board files)
- Auto-scroll (middle mouse button)
- Progressive layout
- Fixed double-click note/node editing, auto-scaling node sizes based on text and scaling node text proportionally during resize, double-click image to view in main viewer, uniform aspect-ratio image/node scaling, and deletable link connections

#### `src/waterfall_page.h / waterfall_page.cpp` [NEW]
Waterfall/masonry layout page:
- Dynamic column count (Ctrl+wheel)
- Configurable gaps (Ctrl+Shift+wheel)
- Corner rounding (Alt+wheel)
- Modal viewer overlay
- Virtual scrolling (only render visible items)

#### `src/start_page.h / start_page.cpp` [NEW]
Start page:
- Desktop wallpaper as background
- Language selector
- Quick-action buttons

---

### Phase 3: Advanced Features

#### `src/settings.h / settings.cpp` [NEW]
Settings system:
- JSON storage (replaces `settings_handling.py`)
- Dear ImGui settings window
- All ~100+ settings from the current app
- Postponed save (debounced writes)

#### `src/library_data.h / library_data.cpp` [NEW]
Data management (replaces `library_data.py`):
- Folder/file/board data structures
- Session save/load
- Favorites system
- Viewing history
- Sort modes (name, date, both directions)

#### `src/ipc.h / ipc.cpp` [NEW]
Inter-process communication (replaces `app_copy_prevention.py` + `ipc_utils.py`):
- Named pipe server/client for single-instance
- Open-file requests from Explorer

#### `src/gamepad.h / gamepad.cpp` [NEW]
Gamepad input (replaces `hidapi_adapter.py`):
- XInput for standard gamepads
- Raw HID for PS4 DualShock / ShanWan
- Configurable button mapping

#### `src/explorer_integration.h / explorer_integration.cpp` [NEW]
Windows Explorer integration (replaces `viewer_dll.cc`):
- Get file list from active Explorer window
- Shell file operations (delete to recycle bin)
- File associations registration
- System tray icon

#### `src/commenting.h / commenting.cpp` [NEW]
Image commenting system (replaces `commenting.py`).

#### `src/tagging.h / tagging.cpp` [NEW]
Image tagging system (replaces `tagging.py`).

#### `src/i18n.h / i18n.cpp` [NEW]
Internationalization (replaces `i18n_l10n_tools.py`):
- Load .po/.mo files
- Runtime language switching

---

### Phase 4: Packaging & Distribution

#### `installer/` [NEW]
- WiX or NSIS installer
- File association registration (.jpg, .png, .gif, .webp, etc.)
- Start menu shortcut
- Uninstaller
- Optional Windows Store package (MSIX)

#### `CMakeLists.txt` [NEW]
Build system:
- Debug/Release configurations
- Static linking of dependencies
- Single .exe output with embedded resources

---

## Rendering Architecture (Native — GPU-accelerated)

```
WM_PAINT / VSync
  └── BeginDraw()
        ├── Clear background
        ├── Draw current page
        │   ├── Viewer → Draw cached image bitmap with GPU transform
        │   ├── Board → Frustum cull items → Draw visible only
        │   ├── Library → Draw grid from cached thumbnail atlas
        │   └── Waterfall → Virtual scroll → Draw visible tiles
        ├── Draw overlays (labels, menus, panels)
        └── EndDraw()
              └── Present() (DXGI swap chain)
```

### Key GPU Optimizations

1. **Hardware Bitmap Caching**: Decoded images stored as `ID2D1Bitmap1` on GPU VRAM — no CPU→GPU upload per frame
2. **Thumbnail Atlas**: All folder thumbnails packed into a single GPU texture atlas — one draw call for entire grid
3. **Frustum Culling**: Board page only renders items whose bounding boxes intersect the viewport
4. **Transform Batching**: Multiple items with the same transform concatenated into a single GPU draw call
5. **Incremental Rendering**: Dirty-rect tracking — only repaint changed regions
6. **Async Image Decoding**: WIC decoder runs on background threads (true parallelism, no GIL)
7. **Mipmap-like multi-resolution**: Pre-generate thumbnail, preview, and full-res bitmaps for each image
8. **Animation Frame Cache**: Pre-decode animation frames to GPU textures for instant frame switching

## Scaling Strategy for Large Directories (20,000+ Images)

To handle directories with massive numbers of files (e.g., 20,000+ images) without blocking the UI thread or exceeding system memory constraints, the native C++ architecture implements the following multi-layered virtualization and caching system:

### 1. Asynchronous Incremental Directory Scanning
* **Problem**: Synchronous directory iteration over 20,000+ files blocks the Win32 message pump, causing the window to freeze ("Not Responding") for several seconds.
* **C++ Solution**: 
  - Execute `std::filesystem::directory_iterator` scanning inside a background worker thread.
  - Discover file paths incrementally and post them in batches (e.g., 100 paths per batch) back to the main UI thread via a thread-safe thread-pool callback to update `m_fileList`.
  - Sort the resulting file list on the background thread once scanning is complete, rather than on the UI thread.

### 2. UI Virtualization (DIP Viewport Mapping)
* **Problem**: Rendering or calculating layouts for 20,000 thumbnail containers wastes CPU cycles and GPU vertices.
* **C++ Solution**:
  - **Thumbnail Strip & Waterfall Grids**: Compute layout boundaries using arithmetic virtualization (e.g., cell index calculations based on current scroll offsets: `first_visible = scroll_pos / cell_width`). Only create vertices and run rendering calls for items currently within the viewport bounds.
  - **Memory Limits**: The vectors containing folder item indexes are thin (representing offsets/integers rather than GPU objects), keeping structural footprint under 1MB for 20k items.

### 3. Priority-Based Bounded Task Queue
* **Problem**: Fast scrolling triggers thousands of decoding threads, building up backlogs and causing long latency delays when the user finally stops.
* **C++ Solution**:
  - The `ImageLoader` worker queue is bounded and prioritizes tasks dynamically.
  - When scrolling triggers a task request for an item that leaves the viewport before loading starts, the task is automatically cancelled or deprioritized, ensuring the decoder only works on images currently visible to the user.

### 4. LRU GPU Texture Cache (VRAM Management)
* **Problem**: Loading 20,000 textures into GPU VRAM (even small 100x100 thumbnails) will cause high memory consumption and drivers to crash.
* **C++ Solution**:
  - Impose a strict Least Recently Used (LRU) limit on `ID2D1Bitmap1` textures.
  - Bound thumbnail cache memory (e.g., limit of 1000 thumbnail textures active at once) and full-size image VRAM cache (e.g., limit of 5-10 high-resolution textures).
  - Automatically release texture handles of items that haven't been visible/selected recently when the cache limit is exceeded.

### 5. Metadata Database & Binary File Indices
* **Problem**: Repeatedly querying OS filesystem metadata (last write time, dimensions, file size) for 20,000 files during sorting operations is slow.
* **C++ Solution**:
  - Cache file properties (timestamps, sizes, dimensions) in a fast in-memory SQLite database or a compact binary indices map.
  - File sorting actions (sort by name, date) query the memory-cache directly, dropping sorting time to under 10 milliseconds.

---

## Verification Plan

### Automated Tests
- Unit tests for data structures (FolderData, FileData, BoardItem serialization)
- Integration tests for image loading (all 15 supported formats)
- Rendering correctness tests (screenshot comparison)
- Build: `cmake --build . --config Release` must succeed with zero warnings

### Manual Verification
- **Zoom performance**: Open a 50MP image, zoom from 1% to 10,000% — should be 60 FPS
- **Board performance**: Load a board with 500+ items — pan/zoom should be 60 FPS
- **Waterfall performance**: Open a folder with 1000+ images — scroll should be 60 FPS
- **Animation playback**: Play a 100-frame GIF at 2x speed — should be smooth
- **Multi-monitor**: Test on 2+ monitors with different DPI scales
- **Memory usage**: Should use less memory than the Python version for equivalent workloads
- **All format support**: Open each of the 16 test files in `test/` directory
- **Feature parity checklist**: Walk through every item in MANUAL_TESTING.md

