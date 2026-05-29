# Krumassan Image Viewer — Native C++ Usage Guide

This guide describes how to build, run, and use the native C++ implementation of the **Krumassan Image Viewer** (Win32 + Direct2D/Direct3D 11).

---

## 1. Building and Running

### Prerequisites
- Windows 10 (Build 18362+) or Windows 11
- Visual Studio 2022 with **C++ Build Tools** installed

### Build Commands
To compile the project in **Release** configuration:
1. Open PowerShell inside the project directory.
2. Configure CMake:
   ```powershell
   & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -B native/build native
   ```
3. Compile with MSBuild:
   ```powershell
   & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" native/build/KrumassanViewer.sln /p:Configuration=Release
   ```
The compiled executable will be written to:
`native/build/bin/Release/KrumassanViewer.exe`

### Run Command
You can run the viewer from command line by passing an initial image file or folder path:
```powershell
./native/build/bin/Release/KrumassanViewer.exe "./test/linux.png"
```

---

## 2. Global Shortcuts and Page Navigation

These keys work globally across all screens of the application:
*   **`2`**: Switch to **Viewer Page** (visores de imágenes individuales con zoom/pan).
*   **`3`**: Switch to **Board Page** (pizarra infinita estilo PureRef/Miro).
*   **`4`**: Switch to **Library Page** (navegador de carpetas y rejilla de previsualizaciones).
*   **`F11` / `F`**: Toggle **Fullscreen** mode.
*   **`Escape`**: Exit Fullscreen mode, or close the application if windowed.
*   **`Page Up` / `Page Down`**: Increase / decrease window background opacity (translucency).
*   **`Arrow Left` / `Arrow Right` / `Arrow Up` / `Arrow Down`**: Paging between images in the current folder (moves to previous/next image).

---

## 3. Page 2: Viewer Page Controls

The Viewer Page acts as the primary image viewing screen:
*   **Mouse Drag (Left Click)**: Pan the image.
*   **Mouse Wheel**: Smoothly zoom in/out (centered locked at the screen center).
*   **Ctrl + Mouse Wheel**: Paging folder images (scroll to load previous or next image).
*   **Double-click**: Focus/reset image view (toggles between fitting to screen and actual scale).
*   **`1`**: Toggle actual 1:1 scale vs fit-to-screen scale.
*   **`0`**: Reset image zoom, pan, and rotation back to default.
*   **`R`**: Rotate image 90° clockwise (hold `Shift` to rotate counter-clockwise).
*   **`Space`**: Pause / Resume animated image playback (GIF/APNG/animated WebP).
*   **Ctrl + Left Drag (Magnifier selection)**: Drag a spotlight box over a region of the image; releasing zooms into that specific region.
*   **Toggle Overlays**:
    *   **`C`**: Toggle Cyberpunk frame corners.
    *   **`T`**: Toggle Rule-of-thirds grid.
    *   **`M`**: Toggle file info/metadata overlay.
    *   **`P`**: Toggle Pixelated/Smooth interpolation on zoom.

---

## 4. Page 3: Board Page (Pizarra Infinite) Controls

The Board Page provides an infinite virtual canvas where you can arrange items:

### Canvas Camera Navigation
*   **Middle-Mouse Drag**: Pan the camera view (moves around the canvas).
*   **Mouse Wheel**: Zoom in and out. The point directly under your cursor remains stationary during zoom.
*   **Double-Click empty space**: Resets camera zoom to 100% and centers view.

### Creating Items
*   **Drag & Drop Files/Folders**: Drag any supported image files or folders from Windows Explorer and drop them onto the canvas. They will be instantiated as new items at the dropped cursor location.
*   **Right-Click**: Opens a pop-up menu allowing you to:
  1. Add a new text note (`ITEM_NOTE`).
  2. Add a new node circle (`ITEM_NODE`).

### Item Transforms & Selection
*   **Left-Click**: Select an item. Click empty space to deselect all.
*   **Shift + Left Click**: Select multiple items or toggle selection of a single item.
*   **Left-Click Drag**:
    *   If dragging empty space: Draws a **rubber-band selection rectangle** to select multiple items inside it.
    *   If dragging a selected item: Translates (moves) all selected items together across the canvas.
    *   If dragging corner handles of the selection box: Scales all selected items. Hold `Shift` while scaling to preserve aspect ratio.
*   **`Delete`**: Remove all selected items.

### Creating Connection Links
*   **Ctrl + Left Click Drag**: Hold `Ctrl` and drag from any item (like a note or image) to another item. A directional connection link line with an arrowhead will be created between them.

### Editing Text Notes (`ITEM_NOTE`)
*   **Double-click Note**: Enter text editing mode for the note card. A blinking cursor `|` will appear at the end of the text.
*   **Keyboard Input**: Type text characters directly (letters, digits, space, punctuation).
*   **`Backspace`**: Delete characters.
*   **`Escape` / `Enter`**: Commit text changes and exit editing mode.

### Layout Persistence
*   **`Ctrl + S`**: Saves the current board state to a binary file named `!board.kbrd` inside the active folder.
*   **Autoloading**: When opening a folder containing a `!board.kbrd` file, the board layout is automatically restored.

---

## 5. Page 4: Library Page Controls

The Library Page lists folders and previews:
*   **Left Panel (Folder List)**: Displays folders you've browsed. Use the mouse wheel to scroll folder cards. Clicking a folder card loads it.
*   **Right Panel (Masonry Previews)**: Displays a grid of image previews for the active folder. Click any preview card to open it in full view on Page 2 (Viewer).
*   **Scrollbars**: Hovering scrollbars reveals them. Drag scrollbars with the left mouse button to scroll long lists.
*   **Live Preview**: Hovering over any preview image in the grid displays a full-size preview on the left side of the window (supporting animated files as well).
