cmake_minimum_required(VERSION 3.20)

project(KrumassanViewer
    VERSION 0.90.0
    DESCRIPTION "Krumassan Image Viewer — Native C++20"
    LANGUAGES CXX
)

# ─── C++20 Standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Target Windows 10 1903+ (build 18362) ────────────────────────────────────
add_compile_definitions(
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    NTDDI_VERSION=0x0A000007   # NTDDI_WIN10_19H1
)

# ─── Source Files ──────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/window.cpp
    src/renderer.cpp
    src/image_loader.cpp
    src/animation.cpp
    src/viewer_page.cpp
    src/thumbnail_strip.cpp
    src/library_page.cpp
    src/board_page.cpp
    src/waterfall_page.cpp
    src/settings.cpp
    src/library_data.cpp
    src/pch.cpp
)

set(HEADERS
    src/pch.h
    src/types.h
    src/app.h
    src/window.h
    src/renderer.h
    src/image_loader.h
    src/animation.h
    src/viewer_page.h
    src/thumbnail_strip.h
    src/library_page.h
    src/board_page.h
    src/waterfall_page.h
    src/settings.h
    src/library_data.h
)

# ─── Resource Files ───────────────────────────────────────────────────────────
set(RESOURCES
    src/resources/app.rc
)

# ─── Executable ───────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# ─── Precompiled Header ──────────────────────────────────────────────────────
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# ─── Include Directories ─────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE src)

# ─── Compiler Options (MSVC) ─────────────────────────────────────────────────
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4             # High warning level
        /WX             # Warnings as errors
        /permissive-    # Strict conformance
        /Zc:__cplusplus # Correct __cplusplus macro
        /utf-8          # Source and execution charset UTF-8
        /MP             # Multi-processor compilation
        /EHsc           # Standard exception handling
        # Release optimizations are set by CMAKE_BUILD_TYPE
    )

    # Disable specific warnings that are noisy but harmless
    target_compile_options(${PROJECT_NAME} PRIVATE
        /wd4100   # unreferenced formal parameter (common with Win32 callbacks)
    )
endif()

# ─── Link Libraries (Windows SDK) ────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    # Core Windows
    user32.lib
    gdi32.lib
    shell32.lib
    ole32.lib
    advapi32.lib

    # Direct3D 11 & DXGI
    d3d11.lib
    dxgi.lib

    # Direct2D & DirectWrite
    d2d1.lib
    dwrite.lib

    # DirectComposition (per-pixel alpha transparency)
    dcomp.lib

    # Desktop Window Manager
    dwmapi.lib

    # Windows Imaging Component (image decoding)
    windowscodecs.lib

    # Multimedia timer (high-resolution timing)
    winmm.lib
)

# ─── Output Directory ────────────────────────────────────────────────────────
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

# ─── Copy test images to build output (for easy testing) ─────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/../test"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/test"
    COMMENT "Copying test images..."
)
