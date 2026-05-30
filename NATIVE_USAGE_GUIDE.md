# Krumassan Image Viewer — Native C++ Usage and Testing Guide

This guide describes how to build, run, and perform comprehensive functional testing on the native C++ implementation of the **Krumassan Image Viewer** (Win32 + Direct2D/Direct3D 11).

---

## 1. Building and Running

### Prerequisites
- Windows 10 (Build 18362+) or Windows 11
- Visual Studio 2022 with **C++ Build Tools** installed
- CMake 3.20+ (included in Visual Studio C++ workload)

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
*   **`2`**: Switch to **Viewer Page** (single-image viewer with smooth zoom and pan).
*   **`3`**: Switch to **Board Page** (infinite virtual canvas / PureRef clone).
*   **`4`**: Switch to **Library Page** (folders browser and masonry previews grid).
*   **`5`**: Switch to **Waterfall Page** (masonry layout grid with virtual scrolling and modal viewer).
*   **`F11` / `F`**: Toggle **Fullscreen** mode.
*   **`Escape`**: Exit Fullscreen mode, or close the application if windowed.
*   **`Page Up` / `Page Down`**: Increase / decrease window background opacity (translucency level).
*   **`Arrow Left` / `Arrow Right` / `Arrow Up` / `Arrow Down`**: Navigate between images in the current folder (previous/next image).
*   **`0`**: Reset active view transform (zoom, pan, and rotation) back to default.

---

## 3. Page 2: Viewer Page Controls

The Viewer Page acts as the primary single-image viewing screen:
*   **Mouse Drag (Left Click)**: Pan the image.
*   **Mouse Wheel**: Smoothly zoom in/out (centered locked at the screen center).
*   **Ctrl + Mouse Wheel**: Navigate through folder images (loads previous or next image).
*   **Double-click**: Focus/reset image view (toggles between fitting to screen and actual scale).
*   **`1`**: Toggle actual 1:1 pixel scale vs fit-to-screen scale.
*   **`R`**: Rotate image 90° clockwise (hold `Shift` to rotate counter-clockwise).
*   **`Space`**: Pause / Resume animated image playback (GIF/APNG/animated WebP).
*   **Ctrl + Left Drag (Magnifier selection)**: Drag a spotlight box over a region of the image; releasing zooms into that specific region.
*   **Toggle Overlays**:
    *   **`C`**: Toggle Cyberpunk frame corners.
    *   **`T`**: Toggle Rule-of-thirds grid.
    *   **`M`**: Toggle file info/metadata overlay.
    *   **`P`**: Toggle Pixelated (Nearest Neighbor) vs Smooth (High-Quality Cubic) interpolation on zoom.

---

## 4. Page 3: Board Page (Infinite Canvas) Controls

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

The Library Page features a side-by-side layout:
*   **Left Panel (Folder List)**: Displays folders you've browsed in the current session.
    *   **Mouse Wheel**: Scroll through folder cards.
    *   **Left-Click**: Load the clicked folder.
    *   **Folder Thumbnails**: Folder cards display the first image as a thumbnail on the left, and a mini row of up to 5 thumbnails at the bottom.
*   **Right Panel (Masonry Previews)**: Displays a grid of image previews for the active folder.
    *   **Left-Click**: Open the clicked preview card in full view on Page 2 (Viewer).
    *   **Double-Click**: Open the clicked preview card on Page 2 (Viewer).
    *   **Hover scrollbars**: Revealed when the cursor is near them; drag with Left Click to scroll.
*   **Live Preview (Left Panel Overlay)**: Hovering over any preview image in the grid displays a full-size preview on the left side of the window (supporting animated files as well).

---

## 6. Page 5: Waterfall Page Controls

The Waterfall Page displays a dynamic masonry layout grid of image previews with virtual scrolling:
*   **Mouse Wheel**: Scroll through the masonry grid smoothly.
*   **Scrollbars**: Hovering scrollbars reveals them on the left and right margins of the grid; drag with Left Click to scroll.
*   **Ctrl + Mouse Wheel**: Adjust column count dynamically (increases/decreases the grid columns).
*   **Ctrl + Shift + Mouse Wheel**: Adjust grid spacing/gaps dynamically (values range from 0px to 100px).
*   **Alt + Mouse Wheel**: Adjust image preview corner rounding radius dynamically (values range from 0px to 50px).
*   **Hovering Previews**: Enlarges the hovered cell slightly on top of other cells and renders a drop shadow. If the hovered item is an animated image (GIF/APNG/WebP), it plays back in-place inside the card.
*   **Left-Click / Return / F**: Opens the selected preview in a **Modal Viewer** overlay.
*   **Modal Viewer Controls**:
    *   Functions as a full-featured Viewer Page embedded on top of the dimmed grid.
    *   **`Escape` / `Enter`**: Exits the Modal Viewer and returns to the masonry grid.
    *   **All Page 2 shortcuts** (zoom, pan, rotation, space to pause animations) are supported within the modal.
*   **Keyboard Navigation (Grid Mode)**:
    *   **`Home` / `End`**: Scroll instantly to the top or bottom of the grid.
    *   **`Page Up` / `Page Down`**: Scroll up or down by one screen height.

---

## 7. System Verification (Phase 3 Subsystems)

### A. Settings System (`user_data/settings.json`)
The settings system manages JSON configuration using a high-performance custom parser and writer.
*   **Storage Location**: Look for `user_data/settings.json` relative to the executable path (searches up to 6 parent directories or defaults to next to the `.exe`).
*   **Manual Testing**:
    1. Close the application.
    2. Open `user_data/settings.json` and change values (e.g., `"animated_zoom": [true, ...]` or `"show_thirds": [true, ...]`).
    3. Start the application and verify the modified options are active (e.g., rule-of-thirds grid is drawn by default).
*   **Postponed/Debounced Writing**: Settings modified during runtime are written with debounced timers to avoid disk thrashing. Verify that updates persist correctly after closing the app.

### B. Library Data System (`user_data/session.txt` & `user_data/fav.txt`)
LibraryData handles session, favorites, history, and natural folder sorting.
*   **Session Persistence**:
    *   Stored in `user_data/session.txt`.
    *   Verify that when you add folders or navigate folders, the state is persisted inside this file.
    *   Restarting the app should automatically restore the list of folders in the Library Page (Page 4).
*   **Favorites System**:
    *   Stored in `user_data/fav.txt`.
    *   Verify that files added to favorites are written correctly in a line-delimited list format.
*   **Folder Sorting Modes**:
    *   Supports Name, Date, and Size sorting in both ascending and descending directions.
    *   Sorting updates the active folder file paths dynamically.

---

## 8. Step-by-Step Functional Testing Checklist

Execute the following testing checklist to verify the correct behavior of all implemented subsystems.

### Test Block 1: Windowing & Core Engine (Phase 1)
- [ ] **Startup Path**: Launch the application from PowerShell with a path argument. Verify the image loads instantly.
- [ ] **DPI Scaling**: Drag the application between monitors with different DPI settings (e.g., 100% to 150%). Verify the window scales correctly without resizing artifacts.
- [ ] **Frameless Window Custom Resize**: Hover the cursor over the window edges. Verify the resize cursor appeared and dragging resizing works on all 4 corners and borders (6px border zones).
- [ ] **Translucency Opacity**: Press `Page Up` and `Page Down`. Verify that the background gets darker or more translucent.
- [ ] **Fullscreen Mode**: Press `F11` or `F`. Verify that the window enters borderless fullscreen, hiding the taskbar. Press `Escape` or `F` again to restore window placement.
- [ ] **Drag & Drop Loading**: Drag a file from Explorer and drop it onto Page 2. Verify that the image loaded instantly. Drag a folder and verify that it scans the folder and updates the file list.

### Test Block 2: Image & Animation Loading (Phase 1 & 2)
- [ ] **Static Format Support**: Open static images in the `test/` directory. Verify successful loading of: `.jpg`, `.jpeg`, `.jfif`, `.png`, `.bmp`, `.tif`, `.tiff`, `.avif`, `.static.webp`, `.tga`.
- [ ] **Animated Format Support**: Open animated files in `test/`. Verify smooth playback of: `.gif` (e.g., `rubik.gif`), `.apng` (e.g., `apng.apng`), `.webp` (e.g., `animated.webp`).
- [ ] **Animation Playback Control**: Open `rubik.gif`. Press `Space` to pause playback. Press `Space` again to resume.
- [ ] **Folder Navigation**: Use `Arrow Right` and `Arrow Left` to step through the images. Verify that loop-around works (going from the last image to the first and vice versa).

### Test Block 3: Viewer Page & Interactions (Phase 2)
- [ ] **Target Zooming**: Use the Mouse Wheel to zoom in and out. Verify that the zoom center is locked under the cursor.
- [ ] **Actual Scale Toggle**: Press `1` or Double-Click the image. Verify it toggles between fit-to-screen and 1:1 scale.
- [ ] **Rotation**: Press `R` to rotate 90° clockwise. Hold `Shift` and press `R` to rotate 90° counter-clockwise.
- [ ] **Nearest Neighbor Toggle**: Zoom in close (> 400%). Press `P` to toggle between pixelated zoom and smooth zoom.
- [ ] **Metadata Display**: Press `M`. Verify that the metadata overlay bar containing file path, dimensions, zoom percentage, and index list appears at the bottom.
- [ ] **Overlay Features**: Press `C` to draw cyberpunk corners. Press `T` to draw the rule-of-thirds grid. Verify rendering is aligned with the image boundary.
- [ ] **Spotlight selection**: Hold `Ctrl` and drag a rectangle over a region. Release the mouse. Verify that the view zooms and centers smoothly on that region using a spotlight animation.

### Test Block 4: Thumbnail Strip (Phase 2)
- [ ] **Control Panel Backplate**: Verify the thumbnail row appears at the bottom on Page 2.
- [ ] **Selection Synchronization**: Use `Arrow Right` to navigate images. Verify that the active thumbnail indicator updates its border and scroll-focuses automatically.
- [ ] **Scroll Navigation**: Hover over the thumbnail strip and scroll the wheel. Verify the thumbnails row scrolls horizontally.
- [ ] **Hover Transparency**: Hover the cursor over the thumbnail strip. Verify that the strip becomes fully opaque and desaturates/saturates depending on settings.

### Test Block 5: Library Page (Phase 2)
- [ ] **Dual-Panel Rendering**: Press `4`. Verify the screen splits into a Left Panel (Folder list) and Right Panel (Masonry grid of previews).
- [ ] **Folder List Thumbnails**: Verify that folder cards on the left panel display their cover thumbnail and a mini row of up to 5 small thumbnails.
- [ ] **Folder Selection**: Click a folder card. Verify that the right panel instantly updates with the previews for the newly selected folder.
- [ ] **Live Full-Size Preview**: Hover over any preview image in the grid. Verify that the left panel displays a full-size preview overlay, including animation playback for GIFs.
- [ ] **Image Selection**: Double-click or single-click a preview card. Verify that the application switches back to Page 2 (Viewer) showing the clicked image.

### Test Block 6: Board Page (Phase 2)
- [ ] **Infinite Navigation**: Press `3`. Use Middle-Mouse Drag to pan around empty space. Scroll the wheel to zoom in and out of the board canvas.
- [ ] **Right-Click Context Menu**: Right-click empty canvas space. Choose "Add Note" or "Add Node". Verify the item is created at the cursor coordinates.
- [ ] **Note Text Editing**: Double-click a text note. Verify a blinking cursor appears. Type text and use Backspace to modify it. Press `Enter` or `Escape` to commit.
- [ ] **Rubber-band Selection**: Click and drag on empty canvas space to draw a selection frame. Verify multiple items are selected.
- [ ] **Transform Handles**: Click an item. Drag the corner handles to scale it. Hold `Shift` while dragging to scale uniformly. Drag the item center to move it.
- [ ] **Creating Connection Links**: Hold `Ctrl` and drag from an image item to a note item. Verify a line with an arrow is created connecting both items.
- [ ] **Persistent Serialization**: Press `Ctrl + S`. Verify that `!board.kbrd` is written inside the folder. Delete some items, close the page, and reopen Page 3. Verify it restores the saved board configuration.

### Test Block 7: Waterfall Page (Phase 2)
- [ ] **Masonry Grid Rendering**: Press `5`. Verify the previews are arranged in columns of variable heights.
- [ ] **Dynamic Columns**: Press `Ctrl + Mouse Wheel`. Verify that the columns count increases or decreases dynamically.
- [ ] **Dynamic Gaps Spacing**: Press `Ctrl + Shift + Mouse Wheel`. Verify the gaps spacing between grid items grows or shrinks.
- [ ] **Rounded Corners**: Press `Alt + Mouse Wheel`. Verify that the corners rounding of previews changes dynamically.
- [ ] **Hover Zoom & Playback**: Hover the cursor over a card. Verify the card enlarges slightly, renders a drop shadow, and plays animations if it's a GIF.
- [ ] **Modal Viewer trigger**: Click on any preview card. Verify that the Modal Viewer overlay opens on top of a dimmed grid.
- [ ] **Modal Dismissal**: While in the Modal Viewer, press `Escape` or `Enter`. Verify that you return to the waterfall grid view smoothly.
- [ ] **Grid Navigation**: Use `Page Up` / `Page Down` and `Home` / `End` keys. Verify the grid scrolls accordingly.

