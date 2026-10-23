cmake_minimum_required(VERSION 3.21)

project(lenteuv
    VERSION 0.90.0
    DESCRIPTION "LenteUV Image Viewer — Native C++20"
    LANGUAGES CXX
)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "LenteUV requires Windows and MSVC (Visual Studio 2022 or newer).")
endif()
include(CTest)

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
    _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)

# ─── Source Files ──────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/core/app.cpp
    src/core/window.cpp
    src/core/settings.cpp
    src/core/single_instance_ipc.cpp
    src/rendering/renderer.cpp
    src/rendering/animation_controller.cpp
    src/rendering/animation_curve.cpp
    src/loader/image_loader.cpp
    src/loader/frame_sequencer.cpp
    src/metadata/xmp_tag_manager.cpp
    src/metadata/metadata_cache.cpp
    src/metadata/png_chunk_injector.cpp
    src/metadata/jpeg_segment_injector.cpp
    src/metadata/wic_metadata_reader_writer.cpp
    src/metadata/async_directory_scanner.cpp
    src/core/folder_data_manager.cpp
    src/library/library_page.cpp
    src/board/board_page.cpp
    src/board/board_canvas.cpp
    src/board/board_item_renderer.cpp
    src/board/board_selection_manager.cpp
    src/board/board_text_editor.cpp
    src/board/board_serializer.cpp
    src/board/board_search_manager.cpp
    src/board/board_tag_scanner.cpp
    src/board/board_layout_helper.cpp
    src/viewer/viewer_page.cpp
    src/waterfall/waterfall_page.cpp
    src/ui/thumbnail_strip.cpp
    src/ui/options_menu.cpp
    src/ui/tagging_modal.cpp
    src/shell/shell_integrator.cpp
    src/i18n/i18n_engine.cpp
    src/common/pch.cpp
    src/common/user_data_paths.cpp
)

set(HEADERS
    src/common/pch.h
    src/common/types.h
    src/common/user_data_paths.h
    src/common/image_request.h
    src/core/app.h
    src/core/window.h
    src/core/settings.h
    src/core/single_instance_ipc.h
    src/rendering/renderer.h
    src/rendering/animation_controller.h
    src/rendering/animation_curve.h
    src/loader/image_loader.h
    src/loader/frame_sequencer.h
    src/metadata/xmp_tag_manager.h
    src/metadata/metadata_cache.h
    src/metadata/png_chunk_injector.h
    src/metadata/jpeg_segment_injector.h
    src/metadata/wic_metadata_reader_writer.h
    src/metadata/async_directory_scanner.h
    src/core/folder_data_manager.h
    src/library/library_page.h
    src/board/board_page.h
    src/board/board_canvas.h
    src/board/board_item.h
    src/board/board_item_renderer.h
    src/board/board_selection_manager.h
    src/board/board_text_editor.h
    src/board/board_serializer.h
    src/board/board_search_manager.h
    src/board/board_tag_scanner.h
    src/board/board_layout_helper.h
    src/viewer/viewer_page.h
    src/waterfall/waterfall_page.h
    src/ui/thumbnail_strip.h
    src/ui/options_menu.h
    src/ui/tagging_modal.h
    src/shell/shell_integrator.h
    src/i18n/i18n_engine.h
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
target_precompile_headers(${PROJECT_NAME} PRIVATE src/common/pch.h)

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
    shlwapi.lib
    propsys.lib

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

# Runtime resources are local to the executable: never discover personal data
# from a checkout ancestor, and never overwrite an existing user_data directory.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/locales"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/locales"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/user_data"
    COMMENT "Preparing executable-local translations and data directory..."
)

# ─── Tagging Tests Executable ────────────────────────────────────────────────
if(BUILD_TESTING)
add_executable(tagging_tests
    src/common/user_data_paths.cpp
    src/metadata/xmp_tag_manager.cpp
    src/metadata/metadata_cache.cpp
    src/metadata/png_chunk_injector.cpp
    src/metadata/jpeg_segment_injector.cpp
    src/metadata/wic_metadata_reader_writer.cpp
    src/metadata/async_directory_scanner.cpp
    src/loader/image_loader.cpp
    src/common/pch.cpp
    tests/tagging_tests.cpp
    tests/metadata_cache_tests.cpp
)
target_precompile_headers(tagging_tests PRIVATE src/common/pch.h)
target_include_directories(tagging_tests PRIVATE src)
if(MSVC)
    target_compile_options(tagging_tests PRIVATE
        /W4
        /WX-          # No warnings-as-errors for test harness code
        /permissive-
        /utf-8
        /MP
        /EHsc
        /wd4100       # unreferenced formal parameter (common in test callbacks)
        /wd4189       # local variable initialized but not referenced
    )
endif()
target_link_libraries(tagging_tests PRIVATE
    ole32.lib
    windowscodecs.lib
    user32.lib
    gdi32.lib
    d2d1.lib
    dwrite.lib
    d3d11.lib
    dxgi.lib
    dcomp.lib
    dwmapi.lib
    winmm.lib
    shell32.lib
    advapi32.lib
    shlwapi.lib
    propsys.lib
)
find_program(POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
add_test(NAME metadata.tagging
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/scripts/run_tagging_tests.ps1"
        -Executable "$<TARGET_FILE:tagging_tests>"
        -Fixtures "${CMAKE_SOURCE_DIR}/tests/fixtures"
        -RunRoot "${CMAKE_BINARY_DIR}/test-runs"
)
set_tests_properties(metadata.tagging PROPERTIES TIMEOUT 90)

# Filesystem selection tests own temporary trees and do not launch the application.
add_executable(user_data_path_tests
    tests/user_data_path_tests.cpp
    src/common/user_data_paths.cpp
)
target_include_directories(user_data_path_tests PRIVATE src)
target_compile_options(user_data_path_tests PRIVATE /W4 /WX /permissive- /utf-8 /EHsc)
add_test(NAME common.user_data_paths COMMAND user_data_path_tests)
set_tests_properties(common.user_data_paths PROPERTIES TIMEOUT 30)

# Link actual consumers and loader; delivery is controlled without workers or app renderer initialization.
set(ASYNC_PREVIEW_TEST_SOURCES ${SOURCES})
list(REMOVE_ITEM ASYNC_PREVIEW_TEST_SOURCES src/main.cpp)
add_executable(async_preview_tests tests/async_preview_tests.cpp tests/ipc_diagnostic_tests.cpp tests/image_decode_tests.cpp tests/shutdown_tests.cpp ${ASYNC_PREVIEW_TEST_SOURCES})
target_precompile_headers(async_preview_tests PRIVATE src/common/pch.h)
target_include_directories(async_preview_tests PRIVATE src)
target_compile_options(async_preview_tests PRIVATE /W4 /WX /permissive- /utf-8 /MP /EHsc /wd4100)
get_target_property(LENTEUV_TEST_LINK_LIBRARIES ${PROJECT_NAME} LINK_LIBRARIES)
target_link_libraries(async_preview_tests PRIVATE ${LENTEUV_TEST_LINK_LIBRARIES})
add_test(NAME ui.async_previews COMMAND async_preview_tests)
set_tests_properties(ui.async_previews PROPERTIES TIMEOUT 60)
add_test(NAME lifecycle.shutdown COMMAND async_preview_tests --shutdown-tests)
set_tests_properties(lifecycle.shutdown PROPERTIES TIMEOUT 60)
add_test(NAME ipc.delivery_diagnostics
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/scripts/run_ipc_diagnostics.ps1"
        -Executable "$<TARGET_FILE:async_preview_tests>"
        -Fixtures "${CMAKE_SOURCE_DIR}/tests/fixtures"
        -RunRoot "${CMAKE_BINARY_DIR}/test-runs"
)
set_tests_properties(ipc.delivery_diagnostics PROPERTIES TIMEOUT 60)
add_test(NAME loader.viewer_decode
    COMMAND "${POWERSHELL_EXECUTABLE}" -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/scripts/run_decode_tests.ps1"
        -Executable "$<TARGET_FILE:async_preview_tests>"
        -Fixtures "${CMAKE_SOURCE_DIR}/tests/fixtures"
        -RunRoot "${CMAKE_BINARY_DIR}/test-runs"
)
set_tests_properties(loader.viewer_decode PROPERTIES TIMEOUT 90)
endif()
