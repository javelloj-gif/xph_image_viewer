# Krumassan Image Viewer — Python/PyQt5 Optimization Plan

This document serves as a detailed, step-by-step roadmap for optimizing the performance of the Python/PyQt5 codebase of **Krumassan Image Viewer**. It is structured in 5 logical phases, each containing explicit instructions, target files, and verification steps. This plan is designed so that a low-consumption independent agent can execute it sequentially.

---

## Phase 1: Decoupling Computations from `paintEvent`

### Goal
Ensure `paintEvent` and `_paintEvent` act strictly as readers. Move all allocations, filesystem queries, layout calculations, and heavy mathematical operations out of the draw pass.

### Step 1.1: Pre-cache Checkerboard Brush
*   **Target File**: [_viewer.pyw](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/_viewer.pyw)
*   **Location**: `draw_viewer_content(self, painter)` (around line 5040)
*   **Problem**: Every single frame, a new `QPixmap(40, 40)` is allocated, a temporary `QPainter` is started to draw checkerboard squares, and a `QBrush` is created and assigned.
*   **Action**:
    1. In `MainWindow.__init__`, define `self._checkerboard_brush = None`.
    2. Implement a caching helper:
        ```python
        def get_checkerboard_brush(self):
            if self._checkerboard_brush is None:
                pixmap = QPixmap(40, 40)
                p = QPainter(pixmap)
                white_c = QColor(Qt.gray).darker(600)
                black_c = QColor(Qt.black)
                p.fillRect(QRect(0, 0, 40, 40), QBrush(white_c))
                p.setPen(Qt.NoPen)
                p.setBrush(QBrush(black_c))
                p.drawRect(QRect(0, 0, 20, 20))
                p.drawRect(QRect(20, 20, 20, 20))
                p.end()
                self._checkerboard_brush = QBrush(pixmap)
            return self._checkerboard_brush
        ```
    3. Modify `draw_viewer_content` to use `painter.setBrush(self.get_checkerboard_brush())` and remove the local pixel drawing block.

### Step 1.2: Cache Perlin Noise Calculations
*   **Target File**: [_viewer.pyw](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/_viewer.pyw)
*   **Location**: `draw_noise_cells(self, painter)` (around line 4362)
*   **Problem**: Iterates through a grid on every single frame, calling `noise.snoise3(x, y, self.noise_time)` on the CPU.
*   **Action**:
    1. Cache the noise grid values in a list/matrix in the main window or a helper thread.
    2. Only recalculate the noise grid when the timer ticks, not inside `paintEvent`.
    3. If `STNG.show_noise_cells` is disabled, ensure this function immediately returns.

### Step 1.3: Audit Board Layout Prep
*   **Target File**: [board.py](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/board.py)
*   **Location**: `board_draw_main_default(self, painter, event)`
*   **Problem**: Calls `self.board_prepare_items_layout_and_viewport(cf)` during rendering if the board isn't marked as ready.
*   **Action**: Ensure `board_prepare_items_layout_and_viewport` is called strictly when loading a new folder/board, resizing the window, or performing zoom/pan actions. Set a boolean dirty flag (e.g. `self.board_layout_dirty`) to trigger it, rather than reading state on every frame.

---

## Phase 2: GPU-Accelerated OpenGL Rendering

### Goal
Direct all `QPainter` drawing operations to the GPU by replacing the software back-end with an OpenGL widget viewport.

### Step 2.1: Convert MainWindow to QOpenGLWidget
*   **Target File**: [_viewer.pyw](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/_viewer.pyw)
*   **Action**:
    1. Import `QOpenGLWidget` from `PyQt5.QtWidgets`.
    2. Modify the class declaration of `MainWindow`:
        ```python
        # BEFORE
        class MainWindow(QMainWindow, ...):
        # AFTER
        class MainWindow(QOpenGLWidget, ...):
        ```
    3. Ensure `super().__init__(*args, **kwargs)` is called properly.
    4. Since `MainWindow` inherits from `QOpenGLWidget` instead of `QMainWindow`, check and resolve any calls to `QMainWindow` specific functions (if any exist).

### Step 2.2: Adjust Window Transparency Flags for OpenGL
*   **Target File**: [_viewer.pyw](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/_viewer.pyw)
*   **Action**:
    1. `QOpenGLWidget` handles translucency differently than a standard `QWidget`.
    2. In `MainWindow.__init__`, set the OpenGL format to support alpha buffers:
        ```python
        from PyQt5.QtGui import QSurfaceFormat
        fmt = QSurfaceFormat()
        fmt.setAlphaBufferSize(8)
        QSurfaceFormat.setDefaultFormat(fmt)
        ```
    3. Set attribute `self.setAttribute(Qt.WA_AlwaysStackOnTop)` if translucent window blending with the desktop background shows black borders.

---

## Phase 3: Optimizing PIL-Based Decoders & Zero-Copy

### Goal
Minimize memory copies when converting decoded image bytes from Pillow to PyQt5's rendering pipeline.

### Step 3.1: Zero-Copy QImage Construction
*   **Target File**: [_utils.py](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/_utils.py)
*   **Action**:
    1. Audit PIL-to-Qt image conversion helpers (e.g., loading `.avif`, `.heic`).
    2. Instead of copying image buffers via `QImage.fromData` or converting to temporary bytes, instantiate `QImage` directly using the underlying memory buffer:
        ```python
        # Get raw RGBA buffer from PIL Image
        img = img.convert("RGBA")
        rgba_bytes = img.tobytes("raw", "RGBA") 
        # Create QImage pointing to raw buffer
        qimage = QImage(rgba_bytes, img.width, img.height, QImage.Format_RGBA8888)
        # Keep buffer alive on QImage object to prevent GC crash
        qimage.pil_buffer_ref = rgba_bytes
        ```

### Step 3.2: Configure Qt Image Format Plugins
*   **Action**:
    1. Place `qavif.dll` and `qheif.dll` under the `imageformats/` plugins directory.
    2. When these DLLs are present, `QImageReader` will decode AVIF and HEIC files natively in C++, skipping Pillow entirely.

---

## Phase 4: Parallel Image Decoding & GIL Bypass

### Goal
Utilize a separate Python process to decode images and generate thumbnails, freeing the main thread's GUI responsiveness.

### Step 4.1: Convert Thumbnail Thread to Multiprocessing
*   **Target File**: [library_data.py](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/library_data.py)
*   **Location**: `ThumbnailsPreviewsThread(QThread)`
*   **Action**:
    1. Replace the class `ThumbnailsPreviewsThread` with a pool-based approach using `concurrent.futures.ProcessPoolExecutor` or `multiprocessing.Pool`.
    2. Write a helper function `decode_and_scale_worker(filepath, target_size)` that runs in the child process. It decodes the image, resizes it, and saves it to a cache folder, or returns the path to the cached thumbnail.
    3. The main Qt thread listens to progress notifications via a custom socket, pipe, or timer, and loads the cached thumbnail directly into memory.

---

## Phase 5: Dirty-Rect Updates & Canvas Caching

### Goal
Only repaint portions of the screen that actually change, and cache static canvas layers.

### Step 5.1: Offscreen Grid and Background Cache
*   **Target File**: [board.py](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/board.py)
*   **Action**:
    1. Create a class member `self._static_board_cache = None` (a `QPixmap`).
    2. In `board_draw_grid(self, painter)`, instead of drawing lines directly to the main painter, draw them to the `_static_board_cache` once when zooming or resizing.
    3. In `paintEvent`, simply paint the cached `QPixmap` onto the screen.

### Step 5.2: Target Repaints with `update(QRect)`
*   **Target Files**: [board.py](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/board.py), [control_panel.py](file:///e:/TEMPORAL/Proyectos/Sergkrumas/image_viewer/control_panel.py)
*   **Action**:
    1. Find calls to `self.update()` that trigger when hovering over specific elements (e.g. note resizing, button hover).
    2. Replace them with `self.update(dirty_rect)` specifying the bounding box of the modified element.
