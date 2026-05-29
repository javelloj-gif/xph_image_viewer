// ============================================================================
// pch.h — Precompiled Header
// Krumassan Image Viewer — Native C++20
//
// All frequently-used Windows SDK, DirectX, and STL headers are included here
// to minimize compile times. This file is included automatically by CMake's
// target_precompile_headers().
// ============================================================================
#pragma once

// ─── Windows Core ────────────────────────────────────────────────────────────
#include <Windows.h>
#include <windowsx.h>         // GET_X_LPARAM, GET_Y_LPARAM
#include <shellapi.h>         // DragAcceptFiles, DragQueryFile
#include <dwmapi.h>           // DwmExtendFrameIntoClientArea
#include <ShObjIdl.h>         // IFileOpenDialog (native file picker)

// ─── Direct3D 11 ─────────────────────────────────────────────────────────────
#include <d3d11_1.h>
#include <dxgi1_3.h>

// ─── Direct2D ────────────────────────────────────────────────────────────────
#include <d2d1_3.h>
#include <d2d1_3helper.h>
#include <d2d1effects_2.h>

// ─── DirectWrite ─────────────────────────────────────────────────────────────
#include <dwrite_3.h>

// ─── DirectComposition (per-pixel alpha transparency) ────────────────────────
#include <dcomp.h>

// ─── Windows Imaging Component (image decoding) ─────────────────────────────
#include <wincodec.h>

// ─── COM Smart Pointers (RAII) ───────────────────────────────────────────────
#include <wrl/client.h>

// ─── C++ Standard Library ────────────────────────────────────────────────────
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <numbers>
#include <numeric>
#include <optional>
#include <queue>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>
