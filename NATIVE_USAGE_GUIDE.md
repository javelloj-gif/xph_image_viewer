# Krumassan Image Viewer — Native C++ Usage and Testing Guide

This guide describes how to build, run, and perform comprehensive functional testing on the native C++ implementation of the **Krumassan Image Viewer** (Win32 + Direct2D/Direct3D 11).

---

## 1. Building and Running

### 📁 Directory Context & Preparation
Before running any compilation commands, you must open your terminal (PowerShell or Command Prompt) and set the working directory to the project's root directory:
```powershell
# Verify your working directory is the project root:
# e:\TEMPORAL\Proyectos\Sergkrumas\image_viewer
```
> [!IMPORTANT]
> All build commands listed below **must** be executed from this root directory (`image_viewer`). Do not run them from inside `native/` or `native/build/`.

---

### Prerequisites
- **Operating System**: Windows 10 (Build 18362+) or Windows 11
- **Compiler**: Visual Studio 2022 (Build Tools, Community, Professional, or Enterprise edition) with the **"Desktop development with C++"** workload checked.
- **Build System**: CMake 3.20+ (can be installed via the Visual Studio Installer as **"C++ CMake tools for Windows"** or standalone).

---

### 🚀 Step-by-Step Compilation Guide

#### Option A: Using CMake & MSBuild on System `PATH` (Recommended)
Use this option if you have registered CMake and MSBuild in your system environment variables.

1. **Navigate to the Project Root**:
   Make sure you are at `e:\TEMPORAL\Proyectos\Sergkrumas\image_viewer`.
2. **Configure & Generate Build Files**:
   This step parses the `CMakeLists.txt` file in `native/` and automatically creates the target output directories under `native/build/`.
   ```powershell
   cmake -S native -B native/build
   ```
3. **Compile the Release Executable**:
   Build the generated Visual Studio solution (`.sln`) in `Release` configuration:
   ```powershell
   cmake --build native/build --config Release
   ```

---

#### Option B: Using Direct Executable Paths (Absolute Paths)
Use this option if CMake or MSBuild are not configured in your environment variables. Run these commands from the root directory `e:\TEMPORAL\Proyectos\Sergkrumas\image_viewer`.

##### Step 1: Configuration and Project Generation
This command runs CMake to generate the Visual Studio project solution inside `native/build/`. Choose the command matching your Visual Studio installation:

*   **Visual Studio Build Tools 2022**:
     ```powershell
     & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S native -B native/build
     ```
*   **Visual Studio Community 2022**:
     ```powershell
     & "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S native -B native/build
     ```
*   **Visual Studio Professional 2022**:
     ```powershell
     & "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S native -B native/build
     ```
*   **Visual Studio Enterprise 2022**:
     ```powershell
     & "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S native -B native/build
     ```

##### Step 2: Compiling the Solution
This builds the C++ code to generate the executable. Run the command matching your Visual Studio installation:

*   **Visual Studio Build Tools 2022**:
     ```powershell
     & "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" native/build/KrumassanViewer.sln /p:Configuration=Release /p:Platform=x64
     ```
*   **Visual Studio Community 2022**:
     ```powershell
     & "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" native/build/KrumassanViewer.sln /p:Configuration=Release /p:Platform=x64
     ```
*   **Visual Studio Professional 2022**:
     ```powershell
     & "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" native/build/KrumassanViewer.sln /p:Configuration=Release /p:Platform=x64
     ```
*   **Visual Studio Enterprise 2022**:
     ```powershell
     & "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" native/build/KrumassanViewer.sln /p:Configuration=Release /p:Platform=x64
     ```

---

### 📂 Compiled Output Location
After compilation completes successfully, the native binary is placed in:
```text
e:\TEMPORAL\Proyectos\Sergkrumas\image_viewer\native\build\bin\Release\KrumassanViewer.exe
```

### 🏃 Running the Application
From the project root directory (`e:\TEMPORAL\Proyectos\Sergkrumas\image_viewer`), execute:
```powershell
./native/build/bin/Release/KrumassanViewer.exe "./test/linux.png"
```
You can replace `./test/linux.png` with any absolute or relative path to a supported image file or directory you want to browse.

---

## 2. Global Shortcuts and Page Navigation

These keys work globally across all screens of the application:
*   **`2`**: Switch to **Viewer Page** (single-image viewer with smooth zoom and pan).
*   **`3`**: Switch to **Board Page** (infinite virtual canvas / PureRef clone).
*   **`4`**: Switch to **Library Page** (folders browser and masonry previews grid).
*   **`5`**: Switch to **Waterfall Page** (masonry layout grid with virtual scrolling and modal viewer).
*   **`F11` / `F`**: Toggle **Fullscreen** mode.
*   **`Escape`**: Exit Fullscreen mode, or close the application if windowed (Note: If *Hide to tray on close* is enabled, it minimizes/hides to system tray).
*   **`Page Up` / `Page Down`**: Increase / decrease window background opacity (translucency level).
*   **`Arrow Left` / `Arrow Up` / `Home`**: Navigate to the previous image in the folder.
*   **`Arrow Right` / `Arrow Down` / `End`**: Navigate to the next image in the folder.
*   **`0` / `Numpad 0`**: Reset active view transform (zoom, pan, and rotation) back to default.
*   **`O` / `~` (Tilde)**: Toggle settings menu modal overlay.

---

## 3. Page 2: Viewer Page Controls

The Viewer Page acts as the primary single-image viewing screen:

### Mouse Interactions
*   **Mouse Drag (Left Click)**: Pan the image.
*   **Mouse Wheel (Default)**: Smoothly zoom in/out (centered locked at the screen center).
*   **Ctrl + Mouse Wheel (Default)**: Navigate through folder images (loads previous or next image).
*   **Mouse Wheel over Thumbnail Strip**: Scrolls the bottom thumbnail row horizontally.
*   **Double-click**: Focus/reset image view (toggles between fitting to screen and actual 1:1 scale).
*   **Right-Click**: Opens a context menu with options:
    - **Settings...**: Opens the settings modal.
    - **Toggle Fullscreen (F11)**: Toggles window fullscreen mode.
    - **Exit**: Closes the application.
*   **Ctrl + Left Drag (Magnifier selection)**: Drag a spotlight box over a region of the image; releasing zooms into that specific region.
*   **Ctrl + Left Click on Thumbnail**: Toggle thumbnail selection (for multi-selection).
*   **Drag & Drop Thumbnails**: Select and drag thumbnails horizontally to reorder them in the folder list. A red insertion cursor will guide placement.

### Keyboard Interactions
*   **`1`**: Toggle actual 1:1 pixel scale vs fit-to-screen scale.
*   **`R`**: Rotate image 90° clockwise.
*   **`Shift + R`**: Rotate image 90° counter-clockwise.
*   **`W` / `A` / `S` / `D`**: Pan the image (Up / Left / Down / Right). Hold `Shift` to pan faster.
*   **`Up Arrow` / `+` / `Numpad +`**: Zoom in (centered locked at the screen center).
*   **`Down Arrow` / `-` / `Numpad -`**: Zoom out (centered locked at the screen center).
*   **`Space`**: Pause / Resume animated image playback (GIF/APNG/animated WebP).
*   **`C`**: Toggle Cyberpunk frame corners.
*   **`T`**: Toggle Rule-of-thirds grid.
*   **`M`**: Toggle file info/metadata overlay.
*   **`I`**: Toggle Invert colors filter.
*   **`P`**: Toggle Pixelated (Nearest Neighbor) vs Smooth (High-Quality Cubic) interpolation on zoom.

### Swapped Mouse Wheel Behavior
If the setting **"Enable mouse wheel to zoom and Ctrl+mouse wheel to navigate through image list"** is unchecked/disabled, the wheel inputs are swapped:
*   **Mouse Wheel**: Navigate through folder images (loads previous or next image).
*   **Ctrl + Mouse Wheel**: Smoothly zoom in/out.

---

## 4. Page 3: Board Page (Infinite Canvas) Controls

The Board Page provides an infinite virtual canvas where you can arrange items:

### Canvas Camera Navigation
*   **Middle-Mouse Drag**: Pan the camera view (moves around the canvas).
*   **Mouse Wheel**: Zoom in and out. The point directly under your cursor remains stationary during zoom.
*   **Double-Click empty space**: Resets camera zoom to 100% and centers view.

### Creating Items
*   **Drag & Drop Files/Folders**: Drag any supported image files or folders from Windows Explorer and drop them onto the canvas. They will be instantiated as new items at the dropped cursor location.
*   **Right-Click (Empty space)**: Opens a pop-up menu allowing you to:
  1. **Add Note**: Add a new text note card (`ITEM_NOTE`).
  2. **Add Node**: Add a new node circle (`ITEM_NODE`).
  3. **Settings...**: Opens the settings modal.

### Item Transforms & Selection
*   **Left-Click**: Select an item. Click empty space to deselect all.
*   **Shift + Left Click**: Select multiple items or toggle selection of a single item.
*   **Left-Click Drag**:
    *   If dragging empty space: Draws a **rubber-band selection rectangle** to select multiple items inside it.
    *   If dragging a selected item: Translates (moves) all selected items together across the canvas.
    *   If dragging corner handles of the selection box: Scales all selected items. Hold `Shift` while scaling to preserve aspect ratio. Note: Images and Node circles preserve aspect ratio by default.
*   **`Delete`**: Remove all selected items.
*   **Right-Click (On item)**: Opens a context menu for the item:
    - **Link this item** (Starts directional connecting link)
    - **Delete item**
    - **Delete Link** (If clicked over a connection link segment)

### Creating Connection Links
*   **Ctrl + Left Click Drag**: Hold `Ctrl` and drag from any item (like a note or image) to another item. A directional connection link line with an arrowhead will be created between them.

### Editing Text Notes (`ITEM_NOTE`) & Nodes
*   **Double-click Note / Node**: Enter text editing mode. A blinking cursor `|` will appear at the end of the text.
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
    *   **Right-Click**: Opens a context menu (Settings..., Toggle Fullscreen, Exit).
*   **Right Panel (Masonry Previews)**: Displays a grid of image previews for the active folder.
    *   **Left-Click**: Open the clicked preview card in full view on Page 2 (Viewer).
    *   **Double-Click**: Open the clicked preview card on Page 2 (Viewer).
    *   **Hover scrollbars**: Revealed when the cursor is near them; drag with Left Click to scroll.
    *   **Right-Click**: Opens a context menu (Settings..., Toggle Fullscreen, Exit).
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
*   **Right-Click**: Opens a context menu (Settings..., Toggle Fullscreen, Exit).
*   **Modal Viewer Controls**:
    - Functions as a full-featured Viewer Page embedded on top of the dimmed grid.
    - **`Escape` / `Enter`**: Exits the Modal Viewer and returns to the masonry grid.
    - **All Page 2 shortcuts** (zoom, pan, rotation, space to pause animations) are supported within the modal.
*   **Keyboard Navigation (Grid Mode)**:
    - **`Home` / `End`**: Scroll instantly to the top or bottom of the grid.
    - **`Page Up` / `Page Down`**: Scroll up or down by one screen height.

---

## 7. Options Menu Modal Controls

The Options Menu is a custom HUD dialog that overlays on top of the dimmed application screen.
*   **`O` / `~` (Tilde) / Right-Click Menu -> Settings**: Toggle options menu visibility.
*   **Left-Click Tabs (General, Viewer Page, Advanced, Language / About)**: Navigate settings sections.
*   **Left-Click Checkboxes**: Toggle settings values.
*   **Left-Click Drag Sliders**: Adjust values dynamically (Thumbnails size, Settings menu opacity, Application window opacity).
*   **Left-Click Buttons**: Cycle options (Launch Page, Drag & Drop Page, Debug Page) or set UI language (English, Russian).
*   **Click Outside / Escape**: Dismiss settings menu.

---

## 8. Tagging System Implementation Detail

The image tagging subsystem in the native C++ implementation uses standard-compliant **XMP (Extensible Metadata Platform)** keywords written directly into the image files, replacing the legacy file-based MD5 system.

### 💾 Storage Design
Tags/keywords are stored within the image file headers using the standard Dublin Core namespace (`dc:subject` Bag property). To optimize access and prevent repeating heavy file scans:
1. **Direct File Storage**: The image files themselves act as the single source of truth.
2. **Metadata Cache (`xmp_tag_cache.bin`)**: A binary serialization cache file (using a fast binary format starting with the signature `KTAG` version 1) is saved under the `user_data/` directory. It maps normalized absolute file paths to their last write timestamp and their associated tags array.

### ⚙️ Logical Workflows & Optimizations
- **Direct Reading (`ReadTags()`)**:
  - Queries the metadata query reader via WIC (`/iTXt/{str=XML:com.adobe.xmp}` for PNG, `/app1/xmp/` for JPEG/other formats) and extracts the items from the `<rdf:Bag> -> <rdf:li>` block.
- **Direct Writing (`WriteTags()`)**:
  - **PNG Raw Chunk Injection**: Since WIC's PNG encoder silences or discards metadata writer blocks during serialization, the manager parses the PNG structure manually, removes any existing `iTXt` XMP chunk, and injects a newly computed `iTXt` chunk with the UTF-8 XMP XML packet and a corrected CRC32.
  - **JPEG Raw Segment Injection**: If a JPEG lacks an XMP block, WIC's `AddWriter` fails. The manager manually parses the JPEG APP markers, removes old APP1/XMP blocks, and injects a new APP1 segment containing the XMP packet payload.
  - **Atomic Transactions**: Both PNG and JPEG raw injections are written to a `.tmp` file first and then atomically renamed to prevent file corruption.
  - **Fast Encoding / Transcoding Fallback**: For other formats (TIFF, etc.), the system uses WIC Fast Metadata Encoding, falling back to full frame transcoding if padding is insufficient.
- **Asynchronous Search (`SearchTagsAsync()`)**:
  - Initiates a multithreaded directory traversal using a parallel worker thread pool.
  - Checks discovered image files against the cache: if the file path and last write timestamp match the cache entry, it is counted as a cache hit (retrieved instantly).
  - Cache misses are distributed across background threads, each initializing COM and parsing the metadata via `ReadTags()`, then updating the cache in memory.
  - The binary cache file is saved back to disk once the search is complete.
- **Search Cancellation (`CancelSearch()`)**:
  - Safely flags cancellation via atomics and joins the worker/search threads to prevent resources from being leaked or causing destructor exceptions.
