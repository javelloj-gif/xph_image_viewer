#include "common/pch.h"
#include "core/app.h"
#include "common/performance_trace.h"
#include "board/board_tag_scanner.h"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <limits>
#include <random>
#include <cstring>

namespace kv {

// Drives production callbacks on one thread, with real WIC/queues but no workers or visible UI.
class AsyncPreviewTests {
public:
    // Uses an isolated message-only window so redraw never invalidates the desktop.
    AsyncPreviewTests() {
        app.m_window.m_hwnd = CreateWindowExW(0, L"STATIC", L"async-preview-tests", 0,
            0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
        Check(app.m_window.m_hwnd != nullptr, "message-only window");
        Check(app.m_imageLoader.Initialize(0), "WIC without workers");
        app.m_thumbnailStrip.Initialize(&app);
        app.m_waterfallPage.Initialize(&app);
    }

    // Checks failures in all configurations, including Release.
    void Check(bool condition, const char* label) {
        if (!condition) throw std::runtime_error(label);
        ++checks;
    }

    // Runs one real queued decoder and retains its real callback for controlled completion.
    ImageLoadCallback TakeRequest() {
        auto& loader = app.m_imageLoader;
        Check(loader.m_taskQueue.size() == 1, "one request queued");
        auto task = std::move(loader.m_taskQueue.front());
        loader.m_taskQueue.pop();
        task();
        Check(loader.m_pendingTextures.size() == 1, "one decoded completion queued");
        auto callback = std::move(loader.m_pendingTextures.front().callback);
        loader.m_pendingTextures.clear();
        return callback;
    }

    // Delivers via ProcessPendingTextures; invalid pixel data skips GPU but keeps preview success semantics.
    void Deliver(const ImageLoadCallback& callback, std::shared_ptr<ImageData> image = nullptr) {
        app.m_imageLoader.m_pendingTextures.push_back({std::move(image), callback});
        app.m_imageLoader.ProcessPendingTextures(app.m_renderer);
    }

    // Reproduces the original same-size replacement bug without provoking an out-of-bounds crash.
    void StripReplacement() {
        auto& strip = app.m_thumbnailStrip;
        strip.SetFileList({L"missing-a.png"});
        strip.LoadThumbnail(0);
        auto stale = TakeRequest();
        strip.SetFileList({L"missing-b.png"});
        Deliver(stale);
        Check(!strip.m_thumbnails[0].loadFailed && !strip.m_thumbnails[0].isLoading,
            "stale A completion must not mark replacement B failed");
    }

    // Covers empty/shorter lists, reorder, retention, removal/re-add, duplicate delivery and path occurrences.
    void StripMutations() {
        auto& strip = app.m_thumbnailStrip;
        strip.SetFileList({L"missing-a.png"});
        strip.LoadThumbnail(0);
        auto empty = TakeRequest();
        strip.SetFileList({});
        Deliver(empty);
        Check(strip.m_thumbnails.empty(), "empty list ignores completion");

        strip.SetFileList({L"missing-a.png", L"missing-b.png"});
        strip.LoadThumbnail(1);
        auto moved = TakeRequest();
        strip.SetFileList({L"missing-b.png"});
        Check(strip.m_thumbnails[0].isLoading, "retained request stays pending");
        Deliver(moved);
        Check(strip.m_thumbnails[0].loadFailed && !strip.m_thumbnails[0].isLoading, "shorter list resolves retained item");

        strip.SetFileList({});
        strip.SetFileList({L"missing-a.png", L"missing-b.png"});
        strip.LoadThumbnail(0);
        auto reordered = TakeRequest();
        strip.SetFileList({L"missing-b.png", L"missing-a.png"});
        Deliver(reordered);
        Check(!strip.m_thumbnails[0].loadFailed && strip.m_thumbnails[1].loadFailed, "failure follows reordered path");

        strip.SetFileList({});
        strip.SetFileList({L"missing-a.png"});
        strip.LoadThumbnail(0);
        auto old = TakeRequest();
        strip.SetFileList({L"missing-b.png"});
        strip.SetFileList({L"missing-a.png"});
        strip.LoadThumbnail(0);
        auto current = TakeRequest();
        Deliver(old);
        Check(strip.m_thumbnails[0].isLoading && !strip.m_thumbnails[0].loadFailed, "old same-path generation rejected");
        Deliver(current);
        Check(!strip.m_thumbnails[0].isLoading && strip.m_thumbnails[0].loadFailed, "current same-path generation completes");
        strip.m_thumbnails[0].loadFailed = false;
        Deliver(current);
        Check(!strip.m_thumbnails[0].loadFailed, "duplicate completion consumed once");

        strip.SetFileList({});
        strip.SetFileList({L"missing-a.png"});
        strip.LoadThumbnail(0);
        auto duplicatePath = TakeRequest();
        strip.SetFileList({L"missing-a.png", L"missing-a.png"});
        Deliver(duplicatePath);
        Check(strip.m_thumbnails[0].loadFailed && !strip.m_thumbnails[1].isLoading,
            "same-path occurrences do not share pending state");
        strip.LoadThumbnail(1);
        Deliver(TakeRequest());
        Check(strip.m_thumbnails[1].loadFailed, "second occurrence can load independently");
    }

    // Creates a real 1x1 D2D bitmap on this thread with the software WARP backend, without a swapchain.
    ComPtr<ID2D1Bitmap1> SoftwareBitmap() {
        ComPtr<ID3D11Device> d3d;
        Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
            &d3d, nullptr, nullptr)), "software D3D device");
        ComPtr<IDXGIDevice> dxgi;
        Check(SUCCEEDED(d3d.As(&dxgi)), "DXGI interface");
        ComPtr<ID2D1Factory1> factory;
        Check(SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            IID_PPV_ARGS(&factory))), "D2D factory");
        ComPtr<ID2D1Device> device;
        Check(SUCCEEDED(factory->CreateDevice(dxgi.Get(), &device)), "D2D software device");
        ComPtr<ID2D1DeviceContext> context;
        Check(SUCCEEDED(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context)), "D2D context");
        const uint32_t pixel = 0xffffffff;
        const auto properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        ComPtr<ID2D1Bitmap1> bitmap;
        Check(SUCCEEDED(context->CreateBitmap(D2D1::SizeU(1, 1), &pixel, 4, properties, &bitmap)), "software bitmap");
        return bitmap;
    }

    // Exercises the real drag reorder path and texture-success branch without an application renderer.
    void StripSuccess() {
        auto& strip = app.m_thumbnailStrip;
        strip.SetFileList({});
        app.m_fileList = {L"missing-a.png", L"missing-b.png"};
        strip.SetFileList(app.m_fileList);
        strip.LoadThumbnail(0);
        auto callback = TakeRequest();
        strip.m_selected[0] = true;
        strip.Rearrange(1);
        auto image = std::make_shared<ImageData>();
        image->gpuBitmap = SoftwareBitmap();
        Deliver(callback, image);
        Check(!strip.m_thumbnails[0].texture && strip.m_thumbnails[1].texture == image->gpuBitmap,
            "bitmap follows actual strip reorder");
        Check(!strip.m_thumbnails[1].isLoading && !strip.m_thumbnails[1].loadFailed, "bitmap success ends loading");
        strip.SetFileList(app.m_fileList);
        Check(strip.m_thumbnails[1].texture == image->gpuBitmap, "loaded bitmap cache retained");
        strip.SetFileList({L"missing-b.png", L"missing-a.png", L"missing-a.png"});
        Check(strip.m_thumbnails[2].texture == image->gpuBitmap && !strip.m_thumbnails[2].isLoading,
            "duplicate paths share completed bitmap cache only");
        app.m_fileList.clear();
    }

    // Builds shared preview state in memory only; never scans or writes image folders.
    void Folders() {
        app.m_folders.clear();
        for (int i = 0; i < 2; ++i) {
            FolderInfo folder;
            folder.folderPath = L"synthetic-folder-" + std::to_wstring(i);
            folder.filePaths = {L"missing-a.png", L"missing-b.png"};
            for (const auto& path : folder.filePaths) {
                PreviewItem item;
                item.filePath = path;
                folder.previews.push_back(std::move(item));
            }
            folder.previewsInitialized = true;
            app.m_folders.push_back(std::move(folder));
        }
        app.m_folderData.NotifyPreviewLayoutChanged();
        app.m_currentFolderIdx = 0;
    }

    // Uses Waterfall's real entry point for shared preview completion tests.
    ImageLoadCallback PreviewRequest(int folder, int preview) {
        app.m_waterfallPage.LoadPreviewTexture(folder, preview);
        return TakeRequest();
    }

    // Checks sorting, folder shifts, deletion/recreation, changed path and failure for Waterfall.
    void PreviewMutations() {
        Folders();
        auto sorted = PreviewRequest(0, 0);
        app.m_folderData.SortFolder(0, FolderDataManager::SortMode::NameDesc);
        auto image = std::make_shared<ImageData>();
        Deliver(sorted, image);
        Check(!app.m_folders[0].previews[0].texture && app.m_folders[0].previews[1].texture == image,
            "preview success follows sorting");
        Check(!app.m_folders[0].previews[1].isLoading, "success ends loading");
        app.m_folders[0].previews[1].texture.reset();
        Deliver(sorted, image);
        Check(!app.m_folders[0].previews[1].texture, "preview completion consumed once");

        Folders();
        auto shiftedFolder = PreviewRequest(1, 0);
        app.m_folders.erase(app.m_folders.begin());
        Deliver(shiftedFolder, image);
        Check(app.m_folders[0].previews[0].texture == image, "request follows shifted folder");

        Folders();
        auto removedFolder = PreviewRequest(0, 0);
        app.m_folders.erase(app.m_folders.begin());
        Deliver(removedFolder, image);
        Check(!app.m_folders[0].previews[0].texture, "same paths in replacement folder untouched");

        Folders();
        auto shiftedItem = PreviewRequest(0, 1);
        app.m_folders[0].previews.erase(app.m_folders[0].previews.begin());
        Deliver(shiftedItem);
        Check(app.m_folders[0].previews[0].loadFailed && !app.m_folders[0].previews[0].isLoading,
            "failure follows shifted preview");

        Folders();
        auto recreated = PreviewRequest(0, 0);
        Folders();
        auto newRequest = PreviewRequest(0, 0);
        Deliver(recreated, image);
        Check(app.m_folders[0].previews[0].isLoading && !app.m_folders[0].previews[0].texture,
            "recreated same-path preview rejects old result");
        Deliver(newRequest);
        Check(app.m_folders[0].previews[0].loadFailed && !app.m_folders[0].previews[0].isLoading,
            "current preview failure completes");

        Folders();
        auto changedPath = PreviewRequest(0, 0);
        app.m_folders[0].previews[0].filePath = L"missing-c.png";
        Deliver(changedPath, image);
        Check(!app.m_folders[0].previews[0].texture, "token also requires matching path");
        app.m_folders.clear();
        Deliver(changedPath, image);
        Check(app.m_folders.empty(), "empty folders ignore delivery");
    }

    // Confirms one shared in-flight request and uses App's in-memory deletion path (no filesystem deletion).
    void SharedPreviews() {
        Folders();
        auto shared = PreviewRequest(0, 0);
        app.m_waterfallPage.LoadPreviewTexture(0, 0);
        Check(app.m_imageLoader.m_taskQueue.empty(), "repeated Waterfall request shares in-flight preview");
        Deliver(shared);
        Check(app.m_folders[0].previews[0].loadFailed && !app.m_folders[0].previews[0].isLoading,
            "shared failure releases loading");

        Folders();
        auto removed = PreviewRequest(0, 0);
        auto kept = PreviewRequest(0, 1);
        app.DeleteImage(L"missing-a.png");
        Deliver(removed);
        Check(app.m_folders[0].previews.size() == 1 && app.m_folders[0].previews[0].isLoading &&
            !app.m_folders[0].previews[0].loadFailed, "deleted image result cannot clear survivor loading");
        auto image = std::make_shared<ImageData>();
        Deliver(kept, image);
        Check(app.m_folders[0].previews[0].texture == image && !app.m_folders[0].previews[0].isLoading,
            "surviving request completes after App deletion");
    }

    // Retains the pre-006 rebuild algorithm as a differential oracle and optional timing baseline.
    static std::vector<ThumbnailInfo> ReferenceRebuild(std::vector<ThumbnailInfo> previous,
        const std::vector<std::wstring>& paths) {
        std::vector<ThumbnailInfo> result;
        result.reserve(paths.size());
        for (const auto& path : paths) {
            ThumbnailInfo info;
            info.path = path;
            auto old = std::find_if(previous.begin(), previous.end(),
                [&path](const ThumbnailInfo& item) { return item.path == path; });
            if (old != previous.end()) {
                info = std::move(*old);
                previous.erase(old);
            } else {
                auto cached = std::find_if(result.begin(), result.end(),
                    [&path](const ThumbnailInfo& item) { return item.path == path; });
                if (cached != result.end()) {
                    info.texture = cached->texture;
                    info.loadFailed = cached->loadFailed;
                }
            }
            result.push_back(std::move(info));
        }
        return result;
    }

    // Compares every retained field/token to the old algorithm across mixed states and duplicate occurrences.
    void StripRebuildEquivalence() {
        std::mt19937 random(6004);
        const auto bitmap = SoftwareBitmap();
        auto& strip = app.m_thumbnailStrip;
        for (int trial = 0; trial < 250; ++trial) {
            std::vector<ThumbnailInfo> original;
            const size_t count = random() % 80;
            for (size_t i = 0; i < count; ++i) {
                ThumbnailInfo item;
                item.path = L"synthetic/Ω " + std::to_wstring(random() % 16) + L".png";
                const auto state = random() % 4;
                if (state == 1) item.texture = bitmap;
                if (state == 2) item.loadFailed = true;
                if (state == 3) {
                    item.isLoading = true;
                    item.pendingRequest = std::make_shared<const std::wstring>(item.path);
                }
                original.push_back(std::move(item));
            }
            std::vector<std::wstring> paths;
            const size_t newCount = random() % 100;
            for (size_t i = 0; i < newCount; ++i)
                paths.push_back(L"synthetic/Ω " + std::to_wstring(random() % 20) + L".png");
            const auto expected = ReferenceRebuild(original, paths);
            strip.m_thumbnails = std::move(original);
            strip.m_selected.assign(count, true);
            strip.SetFileList(paths);
            Check(strip.m_thumbnails.size() == expected.size(), "rebuild size matches reference");
            Check(strip.m_selected.size() == paths.size() &&
                std::none_of(strip.m_selected.begin(), strip.m_selected.end(), [](bool value) { return value; }),
                "rebuild resets selection as before");
            for (size_t i = 0; i < expected.size(); ++i) {
                const auto& actual = strip.m_thumbnails[i];
                const auto& reference = expected[i];
                Check(actual.path == reference.path && actual.texture == reference.texture &&
                    actual.isLoading == reference.isLoading && actual.loadFailed == reference.loadFailed &&
                    actual.pendingRequest == reference.pendingRequest, "rebuild matches all occurrence state");
            }
        }
        strip.SetFileList({});
    }

    // Checks both production edge predicates against a full scan, including fractional/extreme float geometry.
    void StripVisibilityEquivalence() {
        auto& strip = app.m_thumbnailStrip;
        const float infinity = std::numeric_limits<float>::infinity();
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float largest = std::numeric_limits<float>::max();
        size_t cases = 0;
        size_t fullVisits = 0;
        size_t candidateVisits = 0;
        for (const size_t count : {0u, 1u, 2u, 17u, 100u, 10000u}) {
            strip.m_thumbnails.resize(count);
            for (const float thumbWidth : {0.0f, -1.0f, 0.01f, 60.0f, 73.375f, 200.0f, largest, infinity, nan}) {
                app.m_settings.thumbnail_width = thumbWidth;
                for (const float windowWidth : {0.0f, 1.0f, 60.0f, 801.25f, -1.0f, infinity, nan}) {
                    for (const float scroll : {0.0f, -60.0f, -300.125f, 0.00001f, -600000.0f, -largest, infinity, -infinity, nan}) {
                        strip.m_scrollX = scroll;
                        const auto [first, end] = strip.GetVisibleRange(windowWidth);
                        Check(first <= end && end <= count, "visible interval bounded");
                        std::vector<size_t> fullUpdate, fullDraw, boundedUpdate, boundedDraw;
                        for (size_t i = 0; i < count; ++i) {
                            const auto rect = strip.GetThumbnailRect(static_cast<int>(i), windowWidth, 480);
                            if (rect.right >= 0 && rect.left <= windowWidth) fullUpdate.push_back(i);
                            if (!(rect.right < 0 || rect.left > windowWidth)) fullDraw.push_back(i);
                        }
                        for (size_t i = first; i < end; ++i) {
                            const auto rect = strip.GetThumbnailRect(static_cast<int>(i), windowWidth, 480);
                            if (rect.right >= 0 && rect.left <= windowWidth) boundedUpdate.push_back(i);
                            if (!(rect.right < 0 || rect.left > windowWidth)) boundedDraw.push_back(i);
                        }
                        Check(fullUpdate == boundedUpdate && fullDraw == boundedDraw, "visible sets/order match full scans");
                        if (thumbWidth == 60 && windowWidth == 801.25f && std::isfinite(scroll)) {
                            fullVisits += count;
                            candidateVisits += end - first;
                        }
                        ++cases;
                    }
                }
            }
        }
        std::cout << "Visibility: " << cases << " geometries; normal subset full/candidate visits "
            << fullVisits << '/' << candidateVisits << " (excludes bisection probes)\n";
        strip.SetFileList({});
        strip.m_scrollX = 0;
        app.m_settings.thumbnail_width = 60;
    }

    // Reports medians of matched isolated rebuilds; never asserts wall-clock thresholds or application speedups.
    int OptimizationBenchmark() {
        auto& strip = app.m_thumbnailStrip;
        std::cout << "count,scenario,reference_ms,indexed_ms\n";
        for (const size_t count : {100u, 1000u, 10000u}) {
            for (int scenario = 0; scenario < 3; ++scenario) {
                std::vector<ThumbnailInfo> original;
                std::vector<std::wstring> paths;
                for (size_t i = 0; i < count; ++i) {
                    ThumbnailInfo item;
                    item.path = L"synthetic/imagen Ω " + std::to_wstring(scenario == 2 ? i % 32 : i) + L".png";
                    original.push_back(std::move(item));
                    paths.push_back(original.back().path);
                }
                if (scenario == 0) original.clear();
                if (scenario == 1) std::reverse(paths.begin(), paths.end());
                std::vector<double> referenceTimes, indexedTimes;
                std::vector<bool> referenceSelection;
                for (int repetition = 0; repetition < 6; ++repetition) {
                    // Both paths receive identical prebuilt input, outside the measured interval.
                    strip.m_thumbnails = original;
                    auto referenceInput = original;
                    std::vector<ThumbnailInfo> reference;
                    double referenceMs = 0, indexedMs = 0;
                    auto runReference = [&]() {
                        const auto begin = std::chrono::steady_clock::now();
                        reference = ReferenceRebuild(std::move(referenceInput), paths);
                        referenceSelection.assign(reference.size(), false);
                        referenceMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
                    };
                    auto runIndexed = [&]() {
                        const auto begin = std::chrono::steady_clock::now();
                        strip.SetFileList(paths);
                        indexedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
                    };
                    if (repetition % 2 == 0) { runReference(); runIndexed(); }
                    else { runIndexed(); runReference(); }
                    Check(strip.m_thumbnails.size() == reference.size() && referenceSelection.size() == paths.size(),
                        "benchmark materializes both outputs");
                    if (repetition > 0) {
                        referenceTimes.push_back(referenceMs);
                        indexedTimes.push_back(indexedMs);
                    }
                }
                std::sort(referenceTimes.begin(), referenceTimes.end());
                std::sort(indexedTimes.begin(), indexedTimes.end());
                std::cout << count << ',' << (scenario == 0 ? "cold" : scenario == 1 ? "reverse" : "duplicates")
                    << ',' << referenceTimes[2] << ',' << indexedTimes[2] << '\n';
            }
        }
        return 0;
    }

    // Preserves the pre-007 greedy formulas and tie-breaking, writing only caller-owned reference rects.
    void ReferenceLayout(std::vector<RectF>& result) {
        result.clear();
        if (app.m_currentFolderIdx < 0 || app.m_currentFolderIdx >= static_cast<int>(app.m_folders.size())) return;
        const auto& previews = app.m_folders[app.m_currentFolderIdx].previews;
        const float width = app.m_renderer.GetTargetSize().ToFloat().width;
        const float itemWidth = 200.0f;
        const float gap = app.m_waterfallPage.m_gridSpacing;
        float available = width - 100.0f;
        if (available < 100.0f) available = width;
        int columns = static_cast<int>((available + gap) / (itemWidth + gap));
        columns = std::max(1, columns);
        if (app.m_waterfallPage.m_columnsNumber > 0)
            columns = std::min(columns, app.m_waterfallPage.m_columnsNumber);
        std::vector<float> heights(columns, 90.0f);
        const float layoutWidth = columns * itemWidth + (columns - 1) * gap;
        const float startX = (width - layoutWidth) / 2.0f;
        result.resize(previews.size());
        for (size_t i = 0; i < previews.size(); ++i) {
            int column = 0;
            for (int c = 1; c < columns; ++c) if (heights[c] < heights[column]) column = c;
            float height = itemWidth;
            const auto& item = previews[i];
            if (item.texture && item.texture->width > 0) height = (itemWidth / item.texture->width) * item.texture->height;
            result[i] = RectF::FromXYWH(startX + column * (itemWidth + gap), heights[column], itemWidth, height);
            heights[column] += height + gap;
        }
    }

    // Executes the Waterfall cache and compares all rect bits against the original greedy formulas.
    void CheckLayouts() {
        if (app.m_currentFolderIdx < 0 || app.m_currentFolderIdx >= static_cast<int>(app.m_folders.size())) {
            app.m_waterfallPage.CalculateLayout();
            Check(!app.m_waterfallPage.m_layoutValid &&
                app.m_waterfallPage.m_previewRects.empty(), "invalid selection clears cache validity");
            return;
        }
        std::vector<RectF> expected;
        {
            ReferenceLayout(expected);
            app.m_waterfallPage.CalculateLayout();
            Check(app.m_waterfallPage.m_previewRects.size() == expected.size(), "Waterfall rect count matches");
            for (size_t i = 0; i < expected.size(); ++i) {
                const auto& rect = app.m_waterfallPage.m_previewRects[i];
                Check(std::memcmp(&rect, &expected[i], sizeof(RectF)) == 0, "layout rect bits match reference");
            }
        }
    }

    // Reuses a sentinel output on a stable frame, then restores it through the normal invalidation contract.
    void CheckLayoutHit() {
        const RectF marker = RectF::FromXYWH(-123, -456, 7, 8);
        app.m_waterfallPage.m_previewRects[0] = marker;
        app.m_waterfallPage.CalculateLayout();
        Check(std::memcmp(&app.m_waterfallPage.m_previewRects[0], &marker, sizeof(RectF)) == 0,
            "stable keys do not reconstruct geometry");
        app.m_folderData.NotifyPreviewLayoutChanged();
        CheckLayouts();
    }

    // Covers input mutations, shared callbacks, stale/failure delivery, resize and parameter-only changes.
    void LayoutCacheRegressions() {
        std::mt19937 random(7002);
        for (int trial = 0; trial < 140; ++trial) {
            Folders();
            auto& folder = app.m_folders[0];
            folder.filePaths.clear();
            folder.previews.clear();
            const size_t count = trial == 0 ? 0 : random() % 120;
            for (size_t i = 0; i < count; ++i) {
                PreviewItem item;
                item.filePath = L"synthetic/Ω " + std::to_wstring(i) + L".png";
                if (i % 3 != 0) {
                    item.texture = std::make_shared<ImageData>();
                    item.texture->width = random() % 700;
                    item.texture->height = random() % 1200;
                }
                folder.filePaths.push_back(item.filePath);
                folder.previews.push_back(std::move(item));
            }
            app.m_folderData.NotifyPreviewLayoutChanged();
            app.m_renderer.m_width = 100 + random() % 3000;
            app.m_waterfallPage.m_columnsNumber = static_cast<int>(random() % 9);
            app.m_waterfallPage.m_gridSpacing = static_cast<float>(random() % 201) / 2.0f;
            CheckLayouts();
            if (count > 0) {
                CheckLayoutHit();
                std::reverse(folder.previews.begin(), folder.previews.end());
                app.m_folderData.NotifyPreviewLayoutChanged();
                CheckLayouts();
            }
            app.m_renderer.m_width += 230;
            CheckLayouts();
            ++app.m_waterfallPage.m_columnsNumber;
            CheckLayouts();
            app.m_waterfallPage.m_gridSpacing += 0.125f;
            CheckLayouts();
            folder.scrollOffset = -98.75f;
            app.m_waterfallPage.m_scrollOffset = -98.75f;
            app.m_renderer.m_height += 15; // Height and scroll must not invalidate base geometry.
            if (count > 0) CheckLayoutHit();
            app.m_currentFolderIdx = 1;
            CheckLayouts();
            app.m_currentFolderIdx = 0;
            CheckLayouts();
        }
        // Real LoadPreviewTexture/ProcessPendingTextures callbacks from Waterfall invalidate the Waterfall cache.
        {
            Folders();
            CheckLayouts();
            const auto callback = PreviewRequest(0, 0);
            const auto before = app.m_folderData.GetPreviewLayoutRevision();
            auto image = std::make_shared<ImageData>();
            image->width = 100;
            image->height = 333;
            Deliver(callback, image);
            Check(app.m_folderData.GetPreviewLayoutRevision() != before, "Waterfall completion invalidates shared geometry");
            CheckLayouts();
            const auto applied = app.m_folderData.GetPreviewLayoutRevision();
            Deliver(callback, image);
            Check(app.m_folderData.GetPreviewLayoutRevision() == applied, "duplicate completion does not invalidate");
            Deliver(PreviewRequest(0, 1));
            Check(app.m_folderData.GetPreviewLayoutRevision() == applied, "failed placeholder keeps geometry revision");
        }
        Folders();
        CheckLayouts();
        auto revision = app.m_folderData.GetPreviewLayoutRevision();
        app.m_folderData.SortFolder(0, FolderDataManager::SortMode::NameDesc);
        Check(app.m_folderData.GetPreviewLayoutRevision() != revision, "sort notifies even with same item count");
        CheckLayouts();
        app.m_currentFolderIdx = 999;
        CheckLayouts();
        app.m_currentFolderIdx = 0;
        CheckLayouts();
        revision = app.m_folderData.GetPreviewLayoutRevision();
        app.m_fileList.clear();
        app.DeleteImage(L"missing-a.png");
        Check(app.m_folderData.GetPreviewLayoutRevision() != revision, "App deletion invalidates Waterfall layout");
        CheckLayouts();
        // Same slot/count replacement and folder-vector relocation use the shared mutation contract.
        app.m_folders.erase(app.m_folders.begin());
        app.m_folderData.NotifyPreviewLayoutChanged();
        CheckLayouts();
        app.m_folders.push_back(app.m_folders[0]);
        app.m_folderData.NotifyPreviewLayoutChanged();
        CheckLayouts();
        app.m_currentFolderIdx = -1;
        app.m_waterfallPage.CalculateLayout();
        CheckLayouts();
        revision = app.m_folderData.GetPreviewLayoutRevision();
        app.m_folderData.Initialize(nullptr);
        Check(app.m_folderData.GetPreviewLayoutRevision() != revision, "model reinitialize advances epoch");
        CheckLayouts();
        app.m_renderer.m_width = app.m_renderer.m_height = 0;
        app.m_waterfallPage.m_columnsNumber = 0;
        app.m_waterfallPage.m_gridSpacing = 15;
    }

    // Measures the actual unchanged production calculation forced uncached, versus stable or revision-invalidated calls.
    int LayoutBenchmark() {
        std::cout << "count,page,mode,recompute_us,cached_us\n";
        for (const size_t count : {100u, 1000u, 10000u}) {
            Folders();
            auto& folder = app.m_folders[0];
            folder.previews.resize(count);
            for (size_t i = 0; i < count; ++i) {
                folder.previews[i].texture = std::make_shared<ImageData>();
                folder.previews[i].texture->width = 100;
                folder.previews[i].texture->height = 30 + static_cast<uint32_t>(i % 230);
            }
            app.m_folderData.NotifyPreviewLayoutChanged();
            app.m_renderer.m_width = 1601;
            {
                for (const bool forced : {false, true}) {
                    std::vector<double> referenceTimes, cachedTimes;
                    for (int sample = 0; sample < 6; ++sample) {
                        auto measure = [&](bool old) {
                            const auto begin = std::chrono::steady_clock::now();
                            for (int repeat = 0; repeat < 100; ++repeat) {
                                if (old) {
                                    app.m_waterfallPage.m_layoutValid = false;
                                } else if (forced) {
                                    app.m_folderData.NotifyPreviewLayoutChanged();
                                }
                                app.m_waterfallPage.CalculateLayout();
                            }
                            return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - begin).count() / 100;
                        };
                        double oldUs, cachedUs;
                        if (sample % 2 == 0) { oldUs = measure(true); cachedUs = measure(false); }
                        else { cachedUs = measure(false); oldUs = measure(true); }
                        if (sample > 0) { referenceTimes.push_back(oldUs); cachedTimes.push_back(cachedUs); }
                    }
                    std::sort(referenceTimes.begin(), referenceTimes.end());
                    std::sort(cachedTimes.begin(), cachedTimes.end());
                    std::cout << count << ',' << "Waterfall" << ',' << (forced ? "forced" : "stable")
                        << ',' << referenceTimes[2] << ',' << cachedTimes[2] << '\n';
                }
            }
        }
        return 0;
    }

    // Creates an isolated DirectWrite factory; no D2D device or native window is needed for format checks.
    void TextFactory() {
        Check(SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory3),
            reinterpret_cast<IUnknown**>(app.m_renderer.m_dwriteFactory.ReleaseAndGetAddressOf()))), "isolated text factory");
    }

    // Compares all currently effective format properties, including default wrapping/direction/spacing/trimming.
    void CheckFormatProperties(IDWriteTextFormat* actual, IDWriteTextFormat* expected) {
        Check(actual && expected, "both text formats exist");
        Check(actual->GetFontSize() == expected->GetFontSize() && actual->GetFontWeight() == expected->GetFontWeight() &&
            actual->GetFontStyle() == expected->GetFontStyle() && actual->GetFontStretch() == expected->GetFontStretch(), "font scalar properties identical");
        Check(actual->GetTextAlignment() == expected->GetTextAlignment() &&
            actual->GetParagraphAlignment() == expected->GetParagraphAlignment() &&
            actual->GetWordWrapping() == expected->GetWordWrapping() &&
            actual->GetReadingDirection() == expected->GetReadingDirection() &&
            actual->GetFlowDirection() == expected->GetFlowDirection() &&
            actual->GetIncrementalTabStop() == expected->GetIncrementalTabStop(), "alignment/wrapping/direction identical");
        std::wstring actualName(actual->GetFontFamilyNameLength() + 1, L'\0');
        std::wstring expectedName(expected->GetFontFamilyNameLength() + 1, L'\0');
        Check(SUCCEEDED(actual->GetFontFamilyName(actualName.data(), static_cast<UINT32>(actualName.size()))) &&
            SUCCEEDED(expected->GetFontFamilyName(expectedName.data(), static_cast<UINT32>(expectedName.size()))) && actualName == expectedName,
            "font family identical");
        std::wstring actualLocale(actual->GetLocaleNameLength() + 1, L'\0');
        std::wstring expectedLocale(expected->GetLocaleNameLength() + 1, L'\0');
        Check(SUCCEEDED(actual->GetLocaleName(actualLocale.data(), static_cast<UINT32>(actualLocale.size()))) &&
            SUCCEEDED(expected->GetLocaleName(expectedLocale.data(), static_cast<UINT32>(expectedLocale.size()))) && actualLocale == expectedLocale,
            "locale identical");
        DWRITE_LINE_SPACING_METHOD am, em;
        float ah, ab, eh, eb;
        Check(SUCCEEDED(actual->GetLineSpacing(&am, &ah, &ab)) && SUCCEEDED(expected->GetLineSpacing(&em, &eh, &eb)) &&
            am == em && ah == eh && ab == eb, "line spacing identical");
        DWRITE_TRIMMING at = {}, et = {};
        ComPtr<IDWriteInlineObject> as, es;
        Check(SUCCEEDED(actual->GetTrimming(&at, &as)) && SUCCEEDED(expected->GetTrimming(&et, &es)) &&
            at.granularity == et.granularity && at.delimiter == et.delimiter && at.delimiterCount == et.delimiterCount && as == es,
            "trimming identical");
        ComPtr<IDWriteFontCollection> ac, ec;
        Check(SUCCEEDED(actual->GetFontCollection(&ac)) && SUCCEEDED(expected->GetFontCollection(&ec)) &&
            ac->GetFontFamilyCount() == ec->GetFontFamilyCount(), "font collection matches");
    }

    // Verifies key isolation, equivalence, bounded retention, public mutability and factory lifecycle.
    void TextFormatRegressions() {
        TextFactory();
        auto& renderer = app.m_renderer;
        for (const auto* family : {L"Segoe UI", L"Arial", L"Consolas"}) {
            for (const float size : {8.0f, 13.0f, 13.5f, 24.0f}) {
                for (int h = 0; h < 3; ++h) {
                    for (int v = 0; v < 3; ++v) {
                        auto actual = renderer.GetInternalTextFormat(family, size, static_cast<TextAlign>(h), static_cast<TextVAlign>(v));
                        auto expected = renderer.CreateTextFormat(family, size);
                        Check(expected != nullptr, "fresh reference format");
                        expected->SetTextAlignment(h == 0 ? DWRITE_TEXT_ALIGNMENT_LEADING : h == 1 ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_TRAILING);
                        expected->SetParagraphAlignment(v == 0 ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR : v == 1 ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER : DWRITE_PARAGRAPH_ALIGNMENT_FAR);
                        CheckFormatProperties(actual.Get(), expected.Get());
                        Check(actual == renderer.GetInternalTextFormat(family, size, static_cast<TextAlign>(h), static_cast<TextVAlign>(v)), "same key reuses format");
                    }
                }
                for (const auto text : {L"", L"   a b  ", L"Á Ω Ж 中文 🙂", L"línea 1\nlínea 2\tend"}) {
                    const auto measured = renderer.MeasureText(text, size, family);
                    const auto reference = ReferenceMeasureText(renderer, text, size, family);
                    Check(std::memcmp(&measured, &reference, sizeof(PointF)) == 0, "measurement bits identical to pre-cache algorithm");
                }
            }
        }
        auto internal = renderer.GetInternalTextFormat(L"Segoe UI", 13, TextAlign::Center, TextVAlign::Center);
        auto leading = renderer.GetInternalTextFormat(L"Segoe UI", 13);
        auto trailing = renderer.GetInternalTextFormat(L"Segoe UI", 13, TextAlign::Right, TextVAlign::Bottom);
        Check(internal != leading && internal != trailing && leading != trailing &&
            internal->GetTextAlignment() == DWRITE_TEXT_ALIGNMENT_CENTER && internal->GetParagraphAlignment() == DWRITE_PARAGRAPH_ALIGNMENT_CENTER &&
            leading->GetTextAlignment() == DWRITE_TEXT_ALIGNMENT_LEADING && trailing->GetTextAlignment() == DWRITE_TEXT_ALIGNMENT_TRAILING,
            "alignment keys retain independent immutable objects");
        auto publicA = renderer.CreateTextFormat(L"Segoe UI", 13);
        auto publicB = renderer.CreateTextFormat(L"Segoe UI", 13);
        Check(publicA != publicB && publicA != internal && publicB != internal, "public formats are separate objects");
        publicA->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        publicA->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        publicA->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        Check(publicB->GetTextAlignment() == DWRITE_TEXT_ALIGNMENT_LEADING &&
            internal->GetTextAlignment() == DWRITE_TEXT_ALIGNMENT_CENTER && internal->GetWordWrapping() == DWRITE_WORD_WRAPPING_WRAP,
            "public mutation cannot contaminate internal or other public formats");
        auto styled = renderer.CreateTextFormat(L"Segoe UI", 13, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_ITALIC);
        Check(styled->GetFontWeight() == DWRITE_FONT_WEIGHT_BOLD && styled->GetFontStyle() == DWRITE_FONT_STYLE_ITALIC &&
            internal->GetFontWeight() == DWRITE_FONT_WEIGHT_REGULAR && internal->GetFontStyle() == DWRITE_FONT_STYLE_NORMAL,
            "public weight/style independent of fixed private contract");
        auto unknown = renderer.GetInternalTextFormat(L"Segoe UI", 13, static_cast<TextAlign>(99), static_cast<TextVAlign>(99));
        Check(unknown == renderer.GetInternalTextFormat(L"Segoe UI", 13), "unknown enum keeps original default alignment");

        renderer.ClearTextFormatCache();
        auto oldest = renderer.GetInternalTextFormat(L"Segoe UI", 10);
        for (int i = 1; i < 32; ++i) (void)renderer.GetInternalTextFormat(L"Segoe UI", 10.0f + i);
        Check(renderer.m_textFormats.size() == 32, "cache fills to fixed capacity");
        (void)renderer.GetInternalTextFormat(L"Segoe UI", 100);
        Check(renderer.m_textFormats.size() == 32 && renderer.GetInternalTextFormat(L"Segoe UI", 10) != oldest,
            "FIFO eviction removes oldest key while borrowed object stays alive");
        for (int i = 0; i < 200; ++i) (void)renderer.GetInternalTextFormat(L"Segoe UI", 5.0f + i * 0.25f);
        Check(renderer.m_textFormats.size() == 32, "size churn never grows entry count");
        const std::wstring boundaryFamily(128, L'x');
        auto boundary = renderer.GetInternalTextFormat(boundaryFamily.c_str(), 12);
        auto boundaryExpected = renderer.CreateTextFormat(boundaryFamily.c_str(), 12);
        Check(static_cast<bool>(boundary) == static_cast<bool>(boundaryExpected), "family boundary keeps factory result");
        if (boundary) {
            CheckFormatProperties(boundary.Get(), boundaryExpected.Get());
            Check(std::any_of(renderer.m_textFormats.begin(), renderer.m_textFormats.end(),
                [&](const auto& entry) { return entry.family == boundaryFamily; }), "128-character key is retained intact");
        }
        const std::wstring longFamily(129, L'x');
        const auto before = renderer.m_textFormats.size();
        const auto fifoBefore = renderer.m_nextTextFormat;
        auto longActual = renderer.GetInternalTextFormat(longFamily.c_str(), 12);
        auto longExpected = renderer.CreateTextFormat(longFamily.c_str(), 12);
        Check(static_cast<bool>(longActual) == static_cast<bool>(longExpected) && renderer.m_textFormats.size() == before &&
            renderer.m_nextTextFormat == fifoBefore,
            "oversized family preserves API outcome without insertion");
        if (longActual) CheckFormatProperties(longActual.Get(), longExpected.Get());
        const bool wasLogging = detail::g_enableFileLog;
        detail::g_enableFileLog = false;
        for (const float invalid : {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
            auto actual = renderer.GetInternalTextFormat(L"Segoe UI", invalid);
            auto expected = renderer.CreateTextFormat(L"Segoe UI", invalid);
            Check(static_cast<bool>(actual) == static_cast<bool>(expected) && renderer.m_textFormats.size() == before &&
                renderer.m_nextTextFormat == fifoBefore,
                "invalid size keeps factory result and cannot poison cache");
        }
        detail::g_enableFileLog = wasLogging;
        Check(std::all_of(renderer.m_textFormats.begin(), renderer.m_textFormats.end(), [](const auto& entry) {
            return entry.family.size() <= 128 && std::isfinite(entry.size) && entry.size > 0;
        }), "all retained keys satisfy size and length bounds");
        auto oldFactory = renderer.m_dwriteFactory;
        auto oldFormat = renderer.GetInternalTextFormat(L"Segoe UI", 13);
        TextFactory();
        Check(renderer.m_dwriteFactory != oldFactory, "isolated replacement has different identity");
        auto replacement = renderer.GetInternalTextFormat(L"Segoe UI", 13);
        Check(replacement != oldFormat && renderer.m_textFormats.size() == 1 && renderer.m_textFormatFactory == renderer.m_dwriteFactory,
            "replacement factory discards all old entries");
        renderer.DiscardDeviceResources();
        Check(renderer.m_textFormats.empty() && !renderer.m_textFormatFactory && !renderer.m_dwriteFactory && renderer.m_nextTextFormat == 0,
            "resource discard releases cached formats and identity");
    }

    // Reports acquisition and full measurement separately; same inputs, alternate order and no timing assertions.
    int TextBenchmark() {
        TextFactory();
        auto& renderer = app.m_renderer;
        std::cout << "operation,fresh_us,cached_us\n";
        for (const bool measureText : {false, true}) {
            std::vector<double> oldTimes, newTimes;
            for (int sample = 0; sample < 6; ++sample) {
                auto measure = [&](bool cached) {
                    const auto begin = std::chrono::steady_clock::now();
                    double sink = 0;
                    for (int i = 0; i < 1000; ++i) {
                        const float size = 12.0f + i % 4;
                        if (measureText) {
                            const auto value = cached ? renderer.MeasureText(L"Imagen Ω 123 | Zoom 137%", size) :
                                ReferenceMeasureText(renderer, L"Imagen Ω 123 | Zoom 137%", size, L"Segoe UI");
                            sink += value.x;
                        } else {
                            auto format = cached ? renderer.GetInternalTextFormat(L"Segoe UI", size, TextAlign::Center) : renderer.CreateTextFormat(L"Segoe UI", size);
                            if (!cached) {
                                format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                                format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
                            }
                            sink += format->GetFontSize();
                        }
                    }
                    const auto end = std::chrono::steady_clock::now();
                    Check(sink > 0, "benchmark consumes output");
                    return std::chrono::duration<double, std::micro>(end - begin).count() / 1000;
                };
                double fresh, cached;
                if (sample % 2 == 0) { fresh = measure(false); cached = measure(true); }
                else { cached = measure(true); fresh = measure(false); }
                if (sample > 0) { oldTimes.push_back(fresh); newTimes.push_back(cached); }
            }
            std::sort(oldTimes.begin(), oldTimes.end());
            std::sort(newTimes.begin(), newTimes.end());
            std::cout << (measureText ? "MeasureText" : "format") << ',' << oldTimes[2] << ',' << newTimes[2] << '\n';
        }
        return 0;
    }

    // Preserves pre-008 measurement through an independent public format and fresh layout.
    PointF ReferenceMeasureText(Renderer& renderer, std::wstring_view text, float size, const wchar_t* family) {
        auto format = renderer.CreateTextFormat(family, size);
        if (!format) return {};
        std::wstring converted(text);
        for (auto& c : converted) if (c == L' ') c = L'\u00A0';
        ComPtr<IDWriteTextLayout> layout;
        Check(SUCCEEDED(renderer.m_dwriteFactory->CreateTextLayout(converted.data(), static_cast<UINT32>(converted.size()),
            format.Get(), 10000, 1000, &layout)), "reference text layout");
        DWRITE_TEXT_METRICS metrics = {};
        Check(SUCCEEDED(layout->GetMetrics(&metrics)), "reference text metrics");
        return {metrics.width, metrics.height};
    }

    // Preserves pre-008 drawing with a fresh format, for differential pixels in the same executable.
    void ReferenceDrawText(Renderer& renderer, std::wstring_view text, const RectF& rect, const Color& color,
        float size, TextAlign horizontal, TextVAlign vertical, const wchar_t* family) {
        auto format = renderer.CreateTextFormat(family, size);
        Check(format != nullptr, "reference draw format");
        switch (horizontal) {
            case TextAlign::Left: format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING); break;
            case TextAlign::Center: format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER); break;
            case TextAlign::Right: format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); break;
        }
        switch (vertical) {
            case TextVAlign::Top: format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR); break;
            case TextVAlign::Center: format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER); break;
            case TextVAlign::Bottom: format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR); break;
        }
        renderer.m_d2dContext->DrawText(text.data(), static_cast<UINT32>(text.size()), format.Get(), rect.ToD2D(), renderer.GetBrush(color));
    }

    // Exercises real text drawing/measurement across fonts, alignment, Unicode, opacity and cache churn.
    void DrawTextProbe(Renderer& renderer, int state, bool reference) {
        const auto size = renderer.GetTargetSize().ToFloat();
        for (int i = 0; i < 40; ++i) {
            if (reference) (void)ReferenceMeasureText(renderer, L"", 5.0f + i, L"Segoe UI");
            else (void)renderer.MeasureText(L"", 5.0f + i);
        }
        const std::array<const wchar_t*, 3> fonts{L"Segoe UI", L"Arial", L"Consolas"};
        const std::array<std::wstring_view, 3> texts{L"Á Ω Ж 中文 🙂", L"  a b  \nsegunda línea", L"Zoom 137% | [7/1000]"};
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                const auto rect = RectF::FromXYWH(col * size.width / 3 + 3, row * 120.0f + 3,
                    size.width / 3 - 6, 112);
                renderer.DrawRect(rect, Color::White(0.2f));
                const auto color = Color::White(state == 1 ? 0.4f : 1.0f);
                const float fontSize = 12.0f + state * 2 + row * 0.5f;
                if (reference) ReferenceDrawText(renderer, texts[state], rect, color, fontSize,
                    static_cast<TextAlign>(col), static_cast<TextVAlign>(row), fonts[state]);
                else renderer.DrawText(texts[state], rect, color, fontSize,
                    static_cast<TextAlign>(col), static_cast<TextVAlign>(row), fonts[state]);
            }
        }
        const auto measured = reference ? ReferenceMeasureText(renderer, texts[state], 13.5f, fonts[state]) :
            renderer.MeasureText(texts[state], 13.5f, fonts[state]);
        const auto label = std::format(L"{:.3f} x {:.3f}", measured.x, measured.y);
        const auto labelRect = RectF::FromXYWH(0, 370, size.width, 75);
        if (reference) ReferenceDrawText(renderer, label, labelRect, Color::White(), 13.5f,
            TextAlign::Center, TextVAlign::Center, L"Segoe UI");
        else renderer.DrawText(label, labelRect, Color::White(), 13.5f, TextAlign::Center, TextVAlign::Center);
    }

    // Writes production text, strip/overlay, dock/icon or grid pixels on WARP; checks reference geometry/text contracts.
    int VisualSnapshot(const std::filesystem::path& output, bool grids = false, bool textOnly = false, bool reference = false, bool dock = false) {
        auto& renderer = app.m_renderer;
        ComPtr<ID3D11Device> d3d;
        Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
            &d3d, nullptr, nullptr)), "snapshot WARP device");
        ComPtr<IDXGIDevice> dxgi;
        Check(SUCCEEDED(d3d.As(&dxgi)), "snapshot DXGI");
        ComPtr<ID2D1Factory1> factory;
        Check(SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            IID_PPV_ARGS(&factory))), "snapshot D2D factory");
        ComPtr<ID2D1Device> device;
        Check(SUCCEEDED(factory->CreateDevice(dxgi.Get(), &device)), "snapshot D2D device");
        ComPtr<ID2D1DeviceContext> context;
        Check(SUCCEEDED(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context)), "snapshot context");
        Check(SUCCEEDED(context.As(&renderer.m_d2dContext)), "snapshot context2");
        Check(SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3),
            reinterpret_cast<IUnknown**>(renderer.m_dwriteFactory.GetAddressOf()))), "snapshot DirectWrite");
        Check(SUCCEEDED(context->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1), &renderer.m_brush)), "snapshot brush");
        context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        app.m_viewerPage.Initialize(&app);
        app.m_currentImage = std::make_shared<ImageData>();
        app.m_currentImage->filePath = L"synthetic/Imagen Ω con espacios.png";
        app.m_currentImage->width = 640;
        app.m_currentImage->height = 320;
        app.m_settings.show_metadata = true;
        app.m_settings.draw_default_thumbnail = true;
        app.m_currentIndex = 3;
        app.m_fileList.clear();
        for (int i = 0; i < 120; ++i) app.m_fileList.push_back(L"synthetic/Imagen Ω " + std::to_wstring(i) + L".png");
        app.m_currentImage->filePath = app.m_fileList[3];
        app.m_thumbnailStrip.SetFileList(app.m_fileList);
        std::vector<uint32_t> colors(64 * 32);
        for (size_t i = 0; i < colors.size(); ++i)
            colors[i] = 0xff000000u | (static_cast<uint32_t>(i * 17) & 0x00ffffffu);
        auto bitmap = renderer.CreateBitmapFromMemory(reinterpret_cast<const uint8_t*>(colors.data()), 64, 32);
        Check(bitmap != nullptr, "snapshot thumbnail bitmap");
        if (grids) {
            app.m_folders.clear();
            FolderInfo folder;
            folder.folderPath = L"synthetic/Ω";
            for (int i = 0; i < 32; ++i) {
                PreviewItem preview;
                preview.filePath = L"synthetic/Ω " + std::to_wstring(i) + L".png";
                preview.loadFailed = true; // Keep placeholders without queuing disk I/O.
                if (i % 3 == 0) {
                    preview.texture = std::make_shared<ImageData>();
                    preview.texture->width = 64;
                    preview.texture->height = 20 + (i % 5) * 24;
                    preview.texture->hasAlpha = true;
                    preview.texture->gpuBitmap = bitmap;
                }
                folder.previews.push_back(std::move(preview));
            }
            app.m_folders.push_back(std::move(folder));
            app.m_currentFolderIdx = 0;
        }
        for (size_t i = 0; i < app.m_thumbnailStrip.m_thumbnails.size(); ++i) {
            auto& item = app.m_thumbnailStrip.m_thumbnails[i];
            if (i % 3 == 0) item.texture = bitmap;
            else if (i % 3 == 1) item.isLoading = true;
            else item.loadFailed = true;
        }
        std::ofstream stream(output, std::ios::binary);
        Check(stream.good(), "snapshot output");
        int frames = 0;
        for (const uint32_t width : (grids ? std::array{801u, 1601u} : std::array{320u, 801u})) {
            for (const float dpi : {96.0f, 144.0f}) {
                renderer.m_width = width;
                renderer.m_height = 480;
                const auto properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
                renderer.m_renderTarget.Reset();
                Check(SUCCEEDED(context->CreateBitmap(D2D1::SizeU(width, 480), nullptr, 0,
                    properties, &renderer.m_renderTarget)), "snapshot target");
                context->SetTarget(renderer.m_renderTarget.Get());
                context->SetDpi(96, 96);
                app.m_window.m_dpi = static_cast<uint32_t>(dpi);
                auto readProperties = properties;
                readProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
                ComPtr<ID2D1Bitmap1> readable;
                Check(SUCCEEDED(context->CreateBitmap(D2D1::SizeU(width, 480), nullptr, 0,
                    readProperties, &readable)), "snapshot readback");
                for (int state = 0; state < 3; ++state) {
                    auto& strip = app.m_thumbnailStrip;
                    strip.m_opacity = state == 0 ? 0.0f : (state == 1 ? 0.45f : 1.0f);
                    strip.m_scrollX = state == 1 ? -273.125f : -300.0f;
                    strip.m_hoveredIndex = state == 1 ? 7 : 4;
                    strip.m_selected.assign(strip.m_thumbnails.size(), false);
                    strip.m_selected[5] = true;
                    strip.m_draggingThumbnails = state == 1;
                    strip.m_insertIndex = 6;
                    app.m_viewerPage.m_metadataOpacity = state == 0 ? 0.0f : (state == 1 ? 0.35f : 1.0f);
                    app.m_currentPage = state == 2 ? Page::Waterfall : Page::Viewer;
                    app.m_waterfallPage.m_viewerModal = state == 2;
                    if (grids && state == 1) {
                        auto& preview = app.m_folders[0].previews[1];
                        preview.pendingRequest = std::make_shared<const std::wstring>(preview.filePath);
                        auto progressive = std::make_shared<ImageData>();
                        progressive->width = 64;
                        progressive->height = 100;
                        progressive->gpuBitmap = bitmap;
                        Check(app.m_folderData.CompletePreview(preview.pendingRequest, progressive), "snapshot progressive completion");
                    }
                    renderer.BeginFrame();
                    renderer.Clear(dock ? Color::FromHex(0x10171b) : Color::Transparent());
                    if (textOnly) {
                        DrawTextProbe(renderer, state, reference);
                    } else if (grids) {
                        app.m_folders[0].scrollOffset = state == 0 ? 0.0f : -135.25f * state;
                        app.m_waterfallPage.m_scrollOffset = app.m_folders[0].scrollOffset;
                        app.m_waterfallPage.m_hoveredPreviewIdx = state == 0 ? -1 : 3;
                        const auto rect = RectF::FromXYWH(0, 0, static_cast<float>(width), 480);
                        app.m_waterfallPage.DrawPreviewsGrid(renderer, rect);
                    } else {
                        strip.Draw(renderer);
                        if (dock) {
                            renderer.m_textFormats.clear();
                            app.m_viewerPage.m_metadataOpacity = 1;
                        }
                        app.m_viewerPage.DrawOverlayInfo(renderer);
                        if (dock) {
                            Check(std::any_of(renderer.m_textFormats.begin(), renderer.m_textFormats.end(), [](const auto& entry) { return entry.size == 13.0f; }), "metadata retains original 13-unit font at every window DPI");
                            Check(std::all_of(renderer.m_textFormats.begin(), renderer.m_textFormats.end(), [](const auto& entry) { return entry.size <= 13.0f; }), "metadata text is not enlarged by window DPI");
                            app.m_dockOpacity = 1;
                            app.m_dockHovered = state == 1 ? 1 : (state == 2 ? 2 : -1);
                            app.m_dockFocus = -1;
                            app.DrawDock();
                            auto retained = app.m_dockIcons.m_outlines[0];
                            D2D1_MATRIX_3X2_F before, after;
                            context->GetTransform(&before);
                            Check(app.m_dockIcons.Draw(renderer, 0, RectF::FromXYWH(0, 100, 64, 64), Color::White()), "reference icon draws at inspection scale");
                            context->GetTransform(&after);
                            Check(std::memcmp(&before, &after, sizeof(before)) == 0 && app.m_dockIcons.m_outlines[0] == retained, "icon draws preserve transforms and reuse factory geometry");
                            for (const auto& geometry : app.m_dockIcons.m_outlines) {
                                D2D1_RECT_F bounds{};
                                Check(SUCCEEDED(geometry->GetBounds(nullptr, &bounds)) && bounds.left >= 0 && bounds.top >= 0 && bounds.right <= 32 && bounds.bottom <= 32, "all reference outlines fit their design canvas");
                            }
                        }
                    }
                    Check(SUCCEEDED(context->EndDraw()), "snapshot drawing");
                    Check(SUCCEEDED(readable->CopyFromBitmap(nullptr, renderer.m_renderTarget.Get(), nullptr)), "snapshot copy");
                    D2D1_MAPPED_RECT mapped = {};
                    Check(SUCCEEDED(readable->Map(D2D1_MAP_OPTIONS_READ, &mapped)), "snapshot map");
                    for (uint32_t row = 0; row < 480; ++row)
                        stream.write(reinterpret_cast<const char*>(mapped.bits + row * mapped.pitch), width * 4);
                    Check(SUCCEEDED(readable->Unmap()), "snapshot unmap");
                    ++frames;
                }
            }
        }
        Check(stream.good(), "snapshot complete output");
        std::cout << "Wrote " << frames << " offscreen production frames\n";
        return 0;
    }

    // Runs a production decode task on a COM worker and joins before retaining its UI functions for controlled delivery.
    ImageLoader::PendingTexture TakeUploadRequest() {
        auto& loader = app.m_imageLoader;
        Check(loader.m_taskQueue.size() == 1, "one guarded task");
        auto task = std::move(loader.m_taskQueue.front());
        loader.m_taskQueue.pop();
        std::thread worker([task = std::move(task)]() {
            ScopedComInit com;
            task();
        });
        worker.join();
        Check(loader.m_pendingTextures.size() == 1, "one guarded completion");
        auto pending = std::move(loader.m_pendingTextures.front());
        loader.m_pendingTextures.clear();
        return pending;
    }

    // Supplies valid CPU pixels so a missing bitmap proves skipped upload with a working WARP context.
    std::shared_ptr<ImageData> UploadPixels() {
        auto image = std::make_shared<ImageData>();
        image->width = 2;
        image->height = 1;
        image->pixels.assign(8, 255);
        return image;
    }

    // Exercises the optional loader contract and actual token consumers with real D2D uploads, no visible window.
    void UploadValidityRegressions() {
        ComPtr<ID3D11Device> d3d;
        Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
            &d3d, nullptr, nullptr)), "upload WARP device");
        ComPtr<IDXGIDevice> dxgi;
        Check(SUCCEEDED(d3d.As(&dxgi)), "upload DXGI");
        ComPtr<ID2D1Factory1> factory;
        Check(SUCCEEDED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            IID_PPV_ARGS(&factory))), "upload D2D factory");
        ComPtr<ID2D1Device> device;
        Check(SUCCEEDED(factory->CreateDevice(dxgi.Get(), &device)), "upload D2D device");
        ComPtr<ID2D1DeviceContext> context;
        Check(SUCCEEDED(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context)), "upload context");
        Check(SUCCEEDED(context.As(&app.m_renderer.m_d2dContext)), "upload renderer context");
        auto& loader = app.m_imageLoader;
        const auto uiThread = GetCurrentThreadId();
        for (int mode = 0; mode < 3; ++mode) {
            for (bool failure : {false, true}) {
                int predicates = 0;
                int completions = 0;
                const auto pixels = failure ? nullptr : UploadPixels();
                ImageUploadPredicate predicate;
                if (mode != 0) predicate = [&]() {
                    Check(GetCurrentThreadId() == uiThread, "predicate on UI");
                    ++predicates;
                    return mode == 1;
                };
                loader.LoadAsync(L"missing-contract.png", [&](std::shared_ptr<ImageData> image) {
                    Check(GetCurrentThreadId() == uiThread, "completion on UI");
                    Check(image == pixels, "original CPU result including null");
                    Check(predicates == (mode == 0 ? 0 : 1), "predicate before callback");
                    ++completions;
                    loader.LoadAsync(L"missing-reentrant.png", [](std::shared_ptr<ImageData>) {});
                }, 0, 0, predicate);
                auto pending = TakeUploadRequest();
                Check(predicates == 0 && completions == 0, "decode does not invoke UI functions");
                pending.image = pixels;
                loader.m_pendingTextures.push_back(std::move(pending));
                loader.ProcessPendingTextures(app.m_renderer);
                Check(completions == 1, "exactly one completion even when stale or failed");
                if (pixels) Check(pixels->HasGpuBitmap() == (mode != 2), "optional upload guard");
                auto followup = TakeUploadRequest();
                loader.m_pendingTextures.push_back(std::move(followup));
                loader.ProcessPendingTextures(app.m_renderer);
                Check(loader.m_pendingTextures.empty(), "callback can enqueue next load");
            }
        }

        // 0 retained/reordered, 1 removed, 2 recreated, 3 changed path, 4 duplicate, 5 failure, 6 replacement.
        for (int consumer = 0; consumer < 2; ++consumer) {
            for (int mutation = 0; mutation < 7; ++mutation) {
                auto& strip = app.m_thumbnailStrip;
                strip.SetFileList({});
                Folders();
                if (consumer == 0) {
                    strip.SetFileList({L"missing-a.png", L"missing-b.png"});
                    strip.LoadThumbnail(0);
                } else app.m_waterfallPage.LoadPreviewTexture(0, 0);
                auto pending = TakeUploadRequest();
                Check(static_cast<bool>(pending.isCurrent), "consumer opts into guard");
                if (consumer == 0) {
                    if (mutation == 0) strip.SetFileList({L"missing-b.png", L"missing-a.png"});
                    if (mutation == 1 || mutation == 2) strip.SetFileList({});
                    if (mutation == 2) strip.SetFileList({L"missing-a.png"});
                    if (mutation == 3) strip.m_thumbnails[0].path = L"changed.png";
                    if (mutation == 6) strip.SetFileList({L"replacement.png"});
                } else {
                    if (mutation == 0) {
                        std::swap(app.m_folders[0].previews[0], app.m_folders[0].previews[1]);
                        std::swap(app.m_folders[0], app.m_folders[1]);
                    }
                    if (mutation == 1 || mutation == 2) app.m_folders.clear();
                    if (mutation == 2) Folders();
                    if (mutation == 3) app.m_folders[0].previews[0].filePath = L"changed.png";
                    if (mutation == 6) app.m_folders[0].previews[0] = PreviewItem{};
                    app.m_folderData.NotifyPreviewLayoutChanged();
                }
                const bool current = mutation == 0 || mutation == 4 || mutation == 5;
                const auto revision = app.m_folderData.GetPreviewLayoutRevision();
                Check(pending.isCurrent() == current, "token/path validity after mutation");
                Check(app.m_folderData.GetPreviewLayoutRevision() == revision, "predicate is read only");
                const auto pixels = mutation == 5 ? nullptr : UploadPixels();
                pending.image = pixels;
                auto duplicate = pending;
                loader.m_pendingTextures.push_back(std::move(pending));
                loader.ProcessPendingTextures(app.m_renderer);
                if (pixels) Check(pixels->HasGpuBitmap() == current, "consumer upload follows identity");
                Check(!duplicate.isCurrent(), "completion consumes current token or remains stale");
                Check(app.m_folderData.GetPreviewLayoutRevision() == revision +
                    ((consumer != 0 && current && pixels) ? 1 : 0), "007 revision only on current success");
                if (current) {
                    if (consumer == 0) {
                        const auto& item = strip.m_thumbnails[mutation == 0 ? 1 : 0];
                        Check(!item.isLoading && item.loadFailed == !pixels, "strip completes success/failure");
                        Check(item.texture == (pixels ? pixels->gpuBitmap : nullptr), "strip keeps current bitmap");
                    } else {
                        const auto& item = app.m_folders[mutation == 0 ? 1 : 0].previews[mutation == 0 ? 1 : 0];
                        Check(!item.isLoading && item.loadFailed == !pixels, "preview completes success/failure");
                        Check(item.texture == pixels, "preview keeps CPU result");
                    }
                }
                const auto after = app.m_folderData.GetPreviewLayoutRevision();
                duplicate.image = UploadPixels();
                const auto repeated = duplicate.image;
                loader.m_pendingTextures.push_back(std::move(duplicate));
                loader.ProcessPendingTextures(app.m_renderer);
                Check(!repeated->HasGpuBitmap(), "repeated completion skips upload");
                Check(app.m_folderData.GetPreviewLayoutRevision() == after, "repeat does not invalidate layout");
            }
        }
        // Removal/re-add with a new outstanding request must not let the old completion end loading.
        for (int consumer = 0; consumer < 2; ++consumer) {
            auto request = [&]() {
                if (consumer == 0) app.m_thumbnailStrip.LoadThumbnail(0);
                else app.m_waterfallPage.LoadPreviewTexture(0, 0);
                return TakeUploadRequest();
            };
            auto recreate = [&]() {
                app.m_thumbnailStrip.SetFileList({});
                app.m_thumbnailStrip.SetFileList({L"missing-a.png", L"missing-a.png"});
                Folders();
            };
            recreate();
            auto old = request();
            recreate();
            auto fresh = request();
            old.image = UploadPixels();
            fresh.image = UploadPixels();
            const auto oldPixels = old.image;
            const auto freshPixels = fresh.image;
            loader.m_pendingTextures.push_back(std::move(old));
            loader.ProcessPendingTextures(app.m_renderer);
            Check(!oldPixels->HasGpuBitmap() && fresh.isCurrent(), "old generation leaves fresh request pending");
            loader.m_pendingTextures.push_back(std::move(fresh));
            loader.ProcessPendingTextures(app.m_renderer);
            Check(freshPixels->HasGpuBitmap(), "fresh same-path generation uploads");
            if (consumer == 0) {
                app.m_thumbnailStrip.LoadThumbnail(1);
                auto second = TakeUploadRequest();
                second.image = UploadPixels();
                const auto pixels = second.image;
                loader.m_pendingTextures.push_back(std::move(second));
                loader.ProcessPendingTextures(app.m_renderer);
                Check(pixels->HasGpuBitmap(), "second occurrence uploads independently");
                Check(app.m_thumbnailStrip.m_thumbnails[0].texture == freshPixels->gpuBitmap &&
                    app.m_thumbnailStrip.m_thumbnails[1].texture == pixels->gpuBitmap, "distinct occurrence resources");
            }
        }
        // Validity is evaluated per entry, after preceding callbacks, not once for the whole swapped batch.
        bool current = true;
        int delivered = 0;
        auto first = UploadPixels();
        auto second = UploadPixels();
        loader.m_pendingTextures.push_back({first, [&](std::shared_ptr<ImageData>) { current = false; ++delivered; }, {}});
        loader.m_pendingTextures.push_back({second, [&](std::shared_ptr<ImageData>) { ++delivered; }, [&]() { return current; }});
        loader.ProcessPendingTextures(app.m_renderer);
        Check(first->HasGpuBitmap() && !second->HasGpuBitmap() && delivered == 2, "earlier callback invalidates later upload only");
        app.m_renderer.m_d2dContext.Reset();
    }

    // Holds both producers across App destruction and verifies loader admission stays alive until join.
    void ProducerShutdownOrder() {
        auto owner = std::make_unique<App>();
        App* live = owner.get();
        Check(live->m_imageLoader.Initialize(0), "producer test WIC");
        HANDLE entered = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE release = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        Check(entered && release, "producer test events");
        std::atomic<bool> boardAlive{false}, searchAlive{false};
        live->m_boardPage.m_tagScanner->m_scanThread = std::thread([&] {
            WaitForSingleObject(release, 3000);
            boardAlive = !live->m_imageLoader.m_shutdown.load();
        });
        // Missing root needs no WIC factory and cannot touch files; finished callback remains a producer.
        live->m_tagManager.SearchTagsAsync(L"", {}, {}, [&](size_t, size_t) {
            SetEvent(entered);
            WaitForSingleObject(release, 3000);
            searchAlive = !live->m_imageLoader.m_shutdown.load();
        });
        const bool ready = WaitForSingleObject(entered, 3000) == WAIT_OBJECT_0;
        std::jthread releaser([&] { Sleep(30); SetEvent(release); });
        owner.reset();
        releaser.join();
        CloseHandle(entered);
        CloseHandle(release);
        Check(ready && boardAlive && searchAlive, "App joins both producers before loader shutdown");
    }

    // Exercises real consumers before tasks run, independent cancellation, same-path retry and delivery cancellation.
    void ConsumerCancellation() {
        auto& loader = app.m_imageLoader;
        auto& strip = app.m_thumbnailStrip;
        strip.SetFileList({L"missing-a.png", L"missing-b.png"});
        strip.LoadThumbnail(0);
        strip.LoadThumbnail(1);
        std::weak_ptr<const bool> removed = strip.m_thumbnails[0].loadLifetime;
        strip.SetFileList({L"missing-b.png"});
        loader.CancelObsoleteRequests();
        Check(removed.expired() && loader.m_taskQueue.size() == 1, "removed strip task physically pruned, retained task alive");
        Deliver(TakeRequest());
        Check(strip.m_thumbnails[0].loadFailed, "retained strip task completes");
        strip.SetFileList({L"missing-a.png"});
        strip.LoadThumbnail(0);
        strip.CancelPendingLoads();
        loader.CancelObsoleteRequests();
        Check(loader.m_taskQueue.empty() && !strip.m_thumbnails[0].isLoading, "page exit cancels strip and permits retry");
        strip.LoadThumbnail(0);
        Deliver(TakeRequest());
        Check(strip.m_thumbnails[0].loadFailed, "same path can be requested after cancellation");

        Folders();
        app.m_waterfallPage.LoadPreviewTexture(0, 0);
        app.m_waterfallPage.LoadPreviewTexture(0, 1);
        app.m_folders[0].previews.erase(app.m_folders[0].previews.begin());
        loader.CancelObsoleteRequests();
        Check(loader.m_taskQueue.size() == 1, "removed preview cancels only its own lease");
        Deliver(TakeRequest());
        Check(app.m_folders[0].previews[0].loadFailed, "other preview survives removal");
        Folders();
        app.m_waterfallPage.LoadPreviewTexture(0, 0);
        app.m_folderData.SetCurrentFolderIndex(1, false);
        loader.CancelObsoleteRequests();
        Check(loader.m_taskQueue.empty() && !app.m_folders[0].previews[0].isLoading, "folder change cancels previous shared previews");

        int callbacks = 0, predicates = 0;
        auto owner = std::make_shared<const bool>(true);
        loader.LoadAsync(L"missing-cancel.png", [&](std::shared_ptr<ImageData>) { ++callbacks; }, 0, 0,
            [&] { ++predicates; return true; }, owner);
        auto task = std::move(loader.m_taskQueue.front()); loader.m_taskQueue.pop(); task();
        Check(loader.m_pendingTextures.size() == 1, "static completion awaiting UI");
        owner.reset();
        loader.ProcessPendingTextures(app.m_renderer);
        Check(callbacks == 0 && predicates == 0, "canceled completion invokes neither UI predicate, upload nor callback");
        owner = std::make_shared<const bool>(true);
        loader.LoadAnimatedAsync(L"missing-cancel.gif", [&](std::shared_ptr<AnimatedImageData>) { ++callbacks; }, owner);
        task = std::move(loader.m_taskQueue.front()); loader.m_taskQueue.pop(); task();
        owner.reset();
        loader.ProcessPendingTextures(app.m_renderer);
        Check(callbacks == 0 && loader.m_pendingAnimatedTextures.empty(), "animation delivery cancellation");

        // Exercise Waterfall hover cancellation with fixed geometry before decode.
        Folders();
        app.m_renderer.m_width = 1000;
        app.m_renderer.m_height = 700;
        app.m_waterfallPage.m_previewRects = {RectF::FromXYWH(100, 100, 100, 100), RectF::FromXYWH(300, 100, 100, 100)};
        app.m_waterfallPage.OnMouseMove(150, 150, {});
        std::weak_ptr<const bool> oldHover = app.m_waterfallPage.m_hoverLoadLifetime;
        app.m_waterfallPage.OnMouseMove(900, 600, {});
        loader.CancelObsoleteRequests();
        Check(oldHover.expired() && loader.m_taskQueue.empty(), "Waterfall hover exit cancels queued animation");
        app.m_waterfallPage.OnMouseMove(150, 150, {});
        oldHover = app.m_waterfallPage.m_hoverLoadLifetime;
        app.m_folderData.SortFolder(0, FolderDataManager::SortMode::NameDesc);
        loader.CancelObsoleteRequests();
        Check(oldHover.expired() && loader.m_taskQueue.empty(), "sorting retains previews but cancels index-dependent hover");
        app.m_waterfallPage.m_hoverLoadLifetime = std::make_shared<const bool>(true);
        app.m_waterfallPage.m_hoveredOriginalPath = L"missing-a.png";
        loader.LoadAnimatedAsync(L"missing-a.png", [&](auto) { ++callbacks; }, app.m_waterfallPage.m_hoverLoadLifetime);
        app.DeleteImage(L"missing-a.png");
        loader.CancelObsoleteRequests();
        Check(loader.m_taskQueue.empty() && !app.m_waterfallPage.m_hoverLoadLifetime, "deleting hovered path cancels original request");

        // The Waterfall hover lease is also canceled when leaving the mode.
        Folders();
        app.m_currentPage = Page::Waterfall;
        app.m_waterfallPage.m_hoverLoadLifetime = std::make_shared<const bool>(true);
        loader.LoadAnimatedAsync(L"hover.gif", [&](auto) { ++callbacks; }, app.m_waterfallPage.m_hoverLoadLifetime);
        app.SetPage(Page::Viewer);
        loader.CancelObsoleteRequests();
        Check(loader.m_taskQueue.empty() && callbacks == 0, "mode change cancels Waterfall hover generation");

        app.m_boardPage.Initialize(&app);
        app.m_boardPage.m_items.clear();
        BoardItem item; item.type = BoardItemType::Image; item.filePath = L"same-board.png";
        app.m_boardPage.m_items.push_back(item);
        app.m_boardPage.m_items.push_back(item);
        app.m_boardPage.LoadItemTexture(app.m_boardPage.m_items[0]);
        app.m_boardPage.LoadItemTexture(app.m_boardPage.m_items[1]);
        app.m_boardPage.DeleteItemsByIndices({0});
        loader.CancelObsoleteRequests();
        Check(loader.m_taskQueue.size() == 1, "Board removal cancels one duplicate path only");
        Deliver(TakeRequest());
        Check(app.m_boardPage.m_items[0].loadFailed, "Board result applies by identity to surviving item");
        app.m_boardPage.m_items[0].loadFailed = false;
        app.m_boardPage.LoadItemTexture(app.m_boardPage.m_items[0]);
        app.m_currentPage = Page::Board;
        app.SetPage(Page::Viewer);
        loader.CancelObsoleteRequests();
        Check(loader.m_taskQueue.empty() && !app.m_boardPage.m_items[0].isLoading, "Board page departure cancels pending work");
    }

    // Holds a real Board metadata read at the cache mutex while canceling, then checks restart and catalog ownership.
    void BoardScanCancellation() {
        const auto root = std::filesystem::current_path();
        Check(std::filesystem::is_directory(root / L"user_data") && std::filesystem::is_regular_file(root / L"test/seed.png"), "isolated cancellation staging required");
        const auto directory = root / L"board-scan";
        std::filesystem::create_directory(directory);
        std::filesystem::copy_file(root / L"test/seed.png", directory / L"copy.png");
        const auto catalog = directory / L"xmpTags.json";
        { std::ofstream file(catalog); file << "sentinel"; }
        // Reads only the synthetic catalog owned by this copied harness.
        const auto readCatalog = [&] {
            std::ifstream file(catalog, std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(file), {});
        };
        app.m_tagManager.Initialize(app.m_imageLoader.GetWicFactory());
        auto& scanner = *app.m_boardPage.m_tagScanner;
        // Board's ReadTags will block before WIC, with all dependencies still alive.
        std::unique_lock cacheLock(app.m_tagManager.m_cache.m_cacheMutex);
        scanner.StartScan(directory);
        const auto readyDeadline = GetTickCount64() + 3000;
        while (scanner.m_scanTotal.load() == 0 && scanner.IsScanning() && GetTickCount64() < readyDeadline) Sleep(1);
        const bool ready = scanner.m_scanTotal.load() == 1;
        std::jthread cancel([&] { scanner.CancelScan(); });
        const auto stopDeadline = GetTickCount64() + 3000;
        while (!scanner.m_cancelled && GetTickCount64() < stopDeadline) Sleep(1);
        const bool requested = scanner.m_cancelled;
        cacheLock.unlock();
        cancel.join();
        Check(ready && requested && !scanner.IsScanning() && scanner.m_scanCurrent == 0, "Board cancel stops after current read and joins");
        Check(readCatalog() == "sentinel", "canceled Board scan preserves catalog bytes");
        scanner.CancelScan();
        scanner.StartScan(directory);
        const auto finishDeadline = GetTickCount64() + 3000;
        while (scanner.IsScanning() && GetTickCount64() < finishDeadline) Sleep(1);
        const bool finished = !scanner.IsScanning();
        scanner.CancelScan();
        Check(finished && scanner.m_scanCurrent == 1 && readCatalog() == "[\n\n]\n", "restart scans normally and atomically publishes full catalog");
        scanner.PublishCatalog(directory, "must-not-publish");
        Check(readCatalog() == "[\n\n]\n", "publication rejected after cancellation");
        // A destination held without delete sharing must remain intact if replacement fails.
        HANDLE locked = CreateFileW(catalog.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        Check(locked != INVALID_HANDLE_VALUE, "catalog replacement failure setup");
        scanner.m_cancelled = false;
        scanner.PublishCatalog(directory, "replacement");
        CloseHandle(locked);
        scanner.CancelScan();
        Check(readCatalog() == "[\n\n]\n", "failed replacement preserves previous catalog");
        for (const auto& file : std::filesystem::directory_iterator(directory)) {
            Check(file.path().filename().wstring().find(L".tmp-") == std::wstring::npos, "catalog temporaries released");
        }
    }

    // Runs the separate cancellation suite only from the staging prepared by its wrapper.
    int RunCancellation() {
        app.m_folderData.Initialize(&app);
        ConsumerCancellation();
        BoardScanCancellation();
        ProducerShutdownOrder();
        std::cout << "Passed " << checks << " consumer/scanner cancellation checks\n";
        return 0;
    }

    // Uses real viewer input/animations on the owned hidden window; writes only wrapper-owned settings.
    int RunViewerControls() {
        app.m_renderer.m_width = 800;
        app.m_renderer.m_height = 600;
        app.m_currentPage = Page::Viewer;
        app.m_viewerPage.Initialize(&app);
        app.m_currentImage = std::make_shared<ImageData>();
        app.m_currentImage->width = 1600;
        app.m_currentImage->height = 1200;
        auto& viewer = app.m_viewerPage;
        Check(!viewer.OnKeyDown('C', {}) && !viewer.OnKeyDown('I', {}), "retired keys unhandled");
        Check(viewer.OnKeyDown('P', {}) && viewer.m_pixelatedZoom, "pixelated zoom retained");
        for (bool effects : {false, true}) {
            for (bool zoom : {false, true}) {
                app.m_settings.effects = effects;
                app.m_settings.animated_zoom = zoom;
                viewer.ResetView();
                viewer.OnMouseWheel(1, 400, 300, {});
                Check(app.m_animations.IsActive("zoom") == (effects && zoom), "wheel respects effects and saved zoom");
                Check((effects && zoom) ? viewer.m_zoom == 1 : viewer.m_zoom > 1, "wheel animates or applies immediately");
                viewer.ResetView();
                viewer.ZoomToPoint(1.5f, 400, 300);
                Check(app.m_animations.IsActive("zoom") == (effects && zoom), "point zoom respects preferences");
                viewer.ResetView();
                viewer.Toggle1To1();
                Check(app.m_animations.IsActive("zoom") == (effects && zoom), "1:1 respects preferences");
                Check((effects && zoom) ? viewer.m_targetZoom == 2 : viewer.m_zoom == 2, "1:1 target preserved");
                viewer.ResetView();
                viewer.StartRegionZoom(200, 150);
                viewer.FinishRegionZoom(600, 450);
                Check(app.m_animations.IsActive("region_zoom_factor") == (effects && zoom), "region respects preferences");
                viewer.ResetView();
                Check(viewer.OnKeyDown(VK_UP, {}) && app.m_animations.IsActive("zoom"), "keyboard retains its existing independent animation path");
                viewer.ResetView();
            }
        }
        app.m_optionsMenu.Initialize(&app);
        app.m_optionsMenu.Show();
        app.m_window.m_fullscreen = true;
        app.OnMouseDoubleClick(MouseButton::Left, 400, 300, {});
        Check(app.m_window.IsFullscreen(), "Settings consumes double click before viewer");
        app.m_optionsMenu.Hide();
        app.m_dockOpacity = 1;
        const auto dockCell = app.GetDockButtonRect(0).Center();
        app.OnMouseDoubleClick(MouseButton::Left, dockCell.x, dockCell.y, {});
        Check(app.m_window.IsFullscreen(), "dock consumes double click before viewer");
        app.m_dockOpacity = 0;
        viewer.OnMouseDoubleClick(MouseButton::Right, 400, 300, {});
        Check(app.m_window.IsFullscreen(), "right double click does not toggle fullscreen");
        viewer.m_leftButtonDown = true;
        viewer.OnMouseDoubleClick(MouseButton::Left, 400, 300, {});
        Check(!viewer.m_leftButtonDown, "fullscreen transition cancels stale image drag");
        Check(!app.m_window.IsFullscreen(), "left double click always toggles viewer");
        std::cout << "viewer control checks: " << checks << '\n';
        return 0;
    }

    // Exercises actual strip/metadata geometry and input across saved sizes/DPI, without decoding or personal data.
    void StripPresentationRegressions() {
        const int checksBefore = checks;
        auto& strip = app.m_thumbnailStrip;
        auto& viewer = app.m_viewerPage;
        viewer.Initialize(&app);
        app.m_animatedImage.reset();
        app.m_currentImage = std::make_shared<ImageData>();
        app.m_currentPage = Page::Viewer;
        app.m_fileList = {L"p0", L"p1", L"p2", L"p3", L"p4", L"p5", L"p6"};
        strip.m_opacity = 1;
        strip.m_draggingThumbnails = false;
        strip.m_groupSelecting = false;
        for (const auto dpi : {96u, 144u, 192u}) {
            app.m_window.m_dpi = dpi;
            const float scale = dpi / 96.0f;
            for (const float base : {30.0f, 44.0f, 50.0f, 73.0f, 90.0f, 100.0f}) {
                app.m_settings.thumbnail_width = base;
                for (const auto width : {320u, 480u, 736u, 1024u}) {
                    app.m_renderer.m_width = static_cast<uint32_t>(width * scale);
                    app.m_renderer.m_height = static_cast<uint32_t>(480 * scale);
                    const auto size = app.m_renderer.GetTargetSize().ToFloat();
                    for (const int active : {0, 3, 6}) {
                        app.m_currentIndex = active;
                        app.m_currentImage->filePath = app.m_fileList[active];
                        strip.SetFileList(app.m_fileList);
                        strip.SyncSelection(active);
                        strip.m_scrollX = strip.m_targetScrollX;
                        auto rect = strip.GetThumbnailRect(active, size.width, size.height);
                        Check(std::abs(rect.Width() - 1.3f * base * scale) < 0.001f && std::abs(rect.Height() - rect.Width()) < 0.001f, "active square honors DIP scale and saved size");
                        Check(std::abs(rect.Center().x - size.width / 2) < 0.01f, "navigation centers active geometry");
                        Check(strip.HitTest(rect.Center().x, rect.top + 1) == active, "enlarged top of active thumbnail is clickable");
                        strip.m_selected.assign(7, false);
                        strip.m_selected[(active + 1) % 7] = true;
                        strip.m_hoveredIndex = (active + 2) % 7;
                        auto unchanged = strip.GetThumbnailRect(active, size.width, size.height);
                        Check(unchanged.Width() == rect.Width() && unchanged.left == rect.left, "hover and multi-selection do not change active geometry");
                        Check(strip.HandleMouseDoubleClick(MouseButton::Left, rect.Center().x, rect.top + 1, {}), "strip consumes double click over enlarged active area");
                        Check(!strip.HandleMouseDoubleClick(MouseButton::Left, rect.Center().x, strip.GetBounds().top - 1, {}), "strip does not consume outside visible envelope");
                        for (int i = 0; i < 6; ++i) {
                            auto left = strip.GetThumbnailRect(i, size.width, size.height);
                            auto right = strip.GetThumbnailRect(i + 1, size.width, size.height);
                            Check(std::abs(right.left - left.right - 3 * scale) < 0.001f, "neighbor thumbnails use the requested half gap at every DPI");
                        }
                        const auto bar = viewer.GetInfoBarRect(10000 * scale, 30 * scale);
                        Check(bar.left >= 0 && bar.right <= size.width && bar.bottom <= strip.GetBounds().top - 12 * scale + 0.001f, "metadata bounds preserve client and 12 DIP separation");
                        Check(std::abs(bar.Center().x - rect.Center().x) < 0.01f, "metadata aligns with centered active image");
                        for (const float shift : {-345.125f, 0.0f, 720.75f}) {
                            strip.m_scrollX = shift * scale;
                            const auto [first, end] = strip.GetVisibleRange(size.width);
                            std::vector<size_t> reference, bounded;
                            for (size_t i = 0; i < 7; ++i) {
                                const auto r = strip.GetThumbnailRect(static_cast<int>(i), size.width, size.height);
                                if (r.right >= 0 && r.left <= size.width) reference.push_back(i);
                            }
                            for (size_t i = first; i < end; ++i) bounded.push_back(i);
                            Check(reference == bounded, "active-width visible interval matches full traversal");
                        }
                        app.m_currentPage = Page::Waterfall;
                        const auto modalBar = viewer.GetInfoBarRect(100, 30 * scale);
                        Check(std::abs(modalBar.bottom - (size.height - 15 * scale)) < 0.01f, "Waterfall modal reserves no nonexistent strip");
                        app.m_currentPage = Page::Viewer;
                    }
                }
            }
        }
        app.m_window.m_dpi = 96;
        app.m_renderer.m_width = 800; app.m_renderer.m_height = 600;
        app.m_settings.thumbnail_width = 90;
        app.m_currentImage->filePath = L"p3";
        app.m_currentIndex = 5;
        strip.RefreshActiveIndex(5);
        Check(strip.m_activeIndex == 3, "pending requested index does not replace presented original");
        app.m_currentImage->filePath = L"p5";
        strip.RefreshActiveIndex(5);
        Check(strip.m_activeIndex == 5, "successful apply changes active occurrence");
        app.m_fileList = {L"duplicate", L"other", L"duplicate"};
        app.m_currentImage->filePath = L"duplicate"; app.m_currentIndex = 2;
        strip.SetFileList(app.m_fileList);
        Check(strip.m_activeIndex == 2, "duplicate paths select exactly the preferred occurrence");
        strip.m_selected = {true, false, false};
        strip.Rearrange(3);
        Check(app.m_fileList == std::vector<std::wstring>{L"other", L"duplicate", L"duplicate"} && strip.m_activeIndex == 1, "reorder preserves duplicate count and presented occurrence");
        KeyModifiers selectionMods{}; selectionMods.ctrl = true;
        strip.m_scrollX = strip.GetCenteredScroll(1);
        Check(strip.HandleMouseDown(MouseButton::Left, 350, 465, selectionMods), "selection box begins in shared strip area");
        strip.HandleMouseMove(790, 599, selectionMods);
        strip.HandleMouseUp(MouseButton::Left, 790, 599, selectionMods);
        Check(strip.m_activeIndex == 1 && std::any_of(strip.m_selected.begin(), strip.m_selected.end(), [](bool selected) { return selected; }), "real group gesture preserves active image while selecting thumbnails");
        const auto limits = strip.GetScrollLimits();
        strip.m_scrollX = limits.second;
        strip.m_targetScrollX = limits.second;
        auto rect = strip.GetThumbnailRect(0, 800, 600);
        strip.HandleMouseWheel(100, rect.Center().x, rect.Center().y, {});
        Check(strip.m_targetScrollX == limits.second, "wheel clamps to first centered occurrence");
        for (int tick = 0; tick < 8; ++tick) strip.HandleMouseWheel(-1, rect.Center().x, rect.Center().y, {});
        Check(strip.m_targetScrollX == limits.first, "wheel clamps to last centered occurrence");
        TextFactory();
        const std::wstring longName = L"Imagen \u03a9 \U0001F642 " + std::wstring(120, L'W') + L".png";
        const auto fitted = viewer.FitInfoName(app.m_renderer, longName, 120, 13);
        Check(fitted != longName && app.m_renderer.MeasureText(fitted, 13).x <= 120, "long Unicode filename fits available width");
        strip.SetFileList({});
        Check(strip.HitTest(10, 590) == -1 && strip.GetVisibleRange(800) == std::pair<size_t,size_t>{0,0}, "empty strip has no hit or visible range");
        std::cout << "strip presentation checks added: " << checks - checksBefore << '\n';
    }

    // Exercises modal control precedence and return to the retained grid through production App handlers.
    void SharedViewerRegressions() {
        app.m_viewerPage.Initialize(&app);
        app.m_renderer.m_width = 800; app.m_renderer.m_height = 600; app.m_window.m_dpi = 96;
        app.m_currentPage = Page::Waterfall;
        auto& grid = app.m_waterfallPage; auto& strip = app.m_thumbnailStrip; auto& viewer = app.m_viewerPage;
        grid.m_viewerModal = true;
        grid.m_scrollOffset = -347.5f; grid.m_targetScrollOffset = -360;
        const int folder = app.m_currentFolderIdx;
        app.m_fileList = {L"shared-a", L"shared-b"};
        strip.SetFileList(app.m_fileList); strip.m_opacity = strip.m_targetOpacity = 1;
        Check(app.IsViewerActive(), "Waterfall modal hosts the shared viewer");
        Check(viewer.GetInfoBarRect(200, 28).bottom <= strip.GetBounds().top - 12, "modal metadata reserves strip envelope");
        viewer.m_metadataOpacity = 1; viewer.m_infoBarRect = RectF::FromXYWH(200, 400, 250, 30);
        app.OnMouseDown(MouseButton::Left, 220, 410, {});
        Check(grid.m_viewerModal && !viewer.m_leftButtonDown, "metadata press does not dismiss modal or start image drag");
        KeyModifiers ctrl; ctrl.ctrl = true;
        app.OnMouseDown(MouseButton::Left, 100, 590, ctrl);
        Check(grid.m_viewerModal && strip.m_groupSelecting, "strip selection precedes exterior close");
        app.OnMouseMove(700, 590, ctrl); app.OnMouseUp(MouseButton::Left, 700, 590, ctrl);
        Check(grid.m_viewerModal && !strip.m_groupSelecting, "strip release reaches owner in modal host");
        app.OnKeyDown(VK_ESCAPE, {});
        Check(!grid.m_viewerModal && !app.IsViewerActive(), "Escape returns to grid");
        Check(app.m_currentPage == Page::Waterfall && app.m_currentFolderIdx == folder && grid.m_scrollOffset == -347.5f && grid.m_targetScrollOffset == -360, "exit retains folder and scroll offsets");
        grid.m_viewerModal = true; viewer.m_leftButtonDown = true;
        app.OnMouseDown(MouseButton::Left, 0, 100, {});
        Check(grid.m_viewerModal, "active image gesture prevents exterior dismissal");
        viewer.CancelPointerGesture(); app.OnMouseDown(MouseButton::Left, 0, 100, {});
        Check(!grid.m_viewerModal, "unclaimed exterior press returns to grid");
        strip.CancelPendingLoads(); strip.SetFileList({}); app.m_currentPage = Page::Viewer;
    }

    // Checks responsive target sizes, effects/autohide, commands and keyboard/control precedence.
    void DockRegressions() {
        app.m_currentPage = Page::Viewer;
        app.m_settings.effects = false;
        app.m_waterfallPage.m_viewerModal = false;
        for (const auto dpi : {96u, 144u, 192u}) {
            app.m_window.m_dpi = dpi;
            for (const auto width : {320u, 480u, 736u, 1024u}) {
                app.m_renderer.m_width = width; app.m_renderer.m_height = 800;
                const auto bounds = app.GetDockBounds();
                Check(std::abs(bounds.Center().x - width / 2.0f) < 0.001f, "dock is centered at every width and DPI");
                const auto tooltip = app.GetDockTooltipRect();
                Check(tooltip.left >= 0 && tooltip.right <= width && std::abs(tooltip.Center().x-width/2.0f)<0.001f, "dock tooltip stays centered and inside client");
                for (int i = 0; i < 6; ++i) {
                    const auto cell = app.GetDockButtonRect(i);
                    Check(cell.left >= bounds.left && cell.right <= bounds.right && cell.top >= bounds.top && cell.bottom <= bounds.bottom, "dock targets fit responsive bounds");
                    Check(cell.Width() >= 44 * dpi / 96.0f && bounds.right <= width, "dock maintains 44 DIP target and client bounds");
                }
            }
        }
        app.m_window.m_dpi = 96; app.m_renderer.m_width = 800;
        app.m_mouseX = app.GetDockBounds().Center().x; app.m_mouseY = 10;
        app.UpdateDock(0.1);
        Check(app.m_dockOpacity == 1, "top band reveals dock without effects");
        app.m_mouseX = 500; app.m_mouseY = 500; app.UpdateDock(0.8);
        Check(app.m_dockOpacity == 1, "dock retains 900 ms grace");
        app.UpdateDock(0.11);
        Check(app.m_dockOpacity == 0 && app.HitDock(30,30) < 0, "hidden dock does not intercept");
        app.HandleDockKey(VK_TAB, {});
        Check(app.m_dockFocus == 0 && app.m_dockOpacity == 1, "Tab reveals named dock focus");
        const auto rotation = app.m_viewerPage.m_rotation;
        app.InvokeDock(4);
        Check(app.m_currentPage == Page::Viewer && app.m_viewerPage.m_rotation == rotation && !app.m_optionsMenu.IsVisible(), "future filter is inert");
        app.m_dockFocus = 2; app.HandleDockKey(VK_RETURN, {});
        Check(app.m_viewerPage.m_rotation == rotation + 90, "focused rotate invokes existing image command");
        const auto objectResult = app.GetDockAccessibleObject(0, OBJID_CLIENT);
        ComPtr<IAccessible> accessible;
        Check(SUCCEEDED(ObjectFromLresult(objectResult, IID_IAccessible, 0, reinterpret_cast<void**>(accessible.GetAddressOf()))), "Windows MSAA bridge exposes dock object");
        VARIANT child{}; child.vt = VT_I4; child.lVal = 5;
        BSTR name = nullptr;
        Check(SUCCEEDED(accessible->get_accName(child, &name)) && std::wstring(name).find(L"Sort & Filter") != std::wstring::npos, "disabled icon has accessible name");
        SysFreeString(name);
        VARIANT state{};
        Check(SUCCEEDED(accessible->get_accState(child, &state)) && (state.lVal & STATE_SYSTEM_UNAVAILABLE), "MSAA marks future action unavailable");
        Check(accessible->accDoDefaultAction(child) == S_FALSE, "MSAA cannot invoke future action");
        app.m_dockAccessibleState.reset();
        Check(accessible->get_accState(child, &state) == CO_E_OBJNOTCONNECTED, "MSAA external references cannot outlive owner state");
        app.m_dockFocus = -1; app.m_waterfallPage.m_viewerModal = true; app.m_currentPage = Page::Waterfall;
        app.m_dockOpacity = 1;
        const auto cell = app.GetDockButtonRect(4).Center();
        app.OnMouseDown(MouseButton::Left, cell.x, cell.y, {}); app.OnMouseUp(MouseButton::Left, cell.x, cell.y, {});
        Check(app.m_waterfallPage.m_viewerModal, "disabled dock press cannot dismiss modal");
        app.InvokeDock(0);
        Check(app.m_currentPage == Page::Waterfall && !app.m_waterfallPage.m_viewerModal, "Waterfall dock returns to grid");
        app.UpdateDock(1);
        Check(app.m_dockOpacity == 0, "grid has no dock");
        app.m_currentPage = Page::Board; app.UpdateDock(1);
        Check(app.m_dockOpacity == 0 && !app.HandleDockKey(VK_TAB, {}), "Board has no dock input");
        app.m_currentPage = Page::Viewer; app.m_dockFocus = -1; app.m_dockOpacity = 0;
    }

    // Exercises transformed bounds rather than bitmap alpha and excludes controls/gestures from compacting.
    void ExteriorGestureRegressions() {
        auto& viewer = app.m_viewerPage;
        app.m_currentPage = Page::Viewer; app.m_waterfallPage.m_viewerModal = false;
        app.m_renderer.m_width = 800; app.m_renderer.m_height = 600;
        app.m_currentImage = std::make_shared<ImageData>(); app.m_animatedImage.reset();
        app.m_currentImage->width = 200; app.m_currentImage->height = 100;
        app.m_window.m_translucent = true; app.m_settings.window_opacity = 0.5f;
        viewer.ResetView(); viewer.m_metadataOpacity = 0;
        Check(!viewer.IsExteriorPress(400,300,{}), "transparent bitmap interior counts as image bounds");
        Check(viewer.IsExteriorPress(0,200,{}), "transparent exterior qualifies in standalone Viewer");
        viewer.m_rotation = 90;
        Check(viewer.IsPointOverImage(400,390) && !viewer.IsPointOverImage(490,300), "inverse rotation matches real image rather than bounding alpha");
        viewer.m_pan = {20,30}; viewer.m_zoom = 2;
        Check(viewer.IsPointOverImage(420,330), "pan and zoom preserve image hit");
        viewer.CancelPointerGesture(); app.m_window.m_fullscreen = true;
        viewer.OnMouseDown(MouseButton::Left,0,200,{});
        Check(!app.m_window.IsFullscreen() && viewer.m_exteriorPress && !viewer.m_leftButtonDown, "first exterior click compacts and consumes image gesture");
        viewer.OnMouseUp(MouseButton::Left,0,200,{});
        viewer.OnMouseDoubleClick(MouseButton::Left,0,200,{});
        Check(!app.m_window.IsFullscreen() && !viewer.m_exteriorPress, "compact exterior double click never reenters fullscreen");
        viewer.m_leftButtonDown = true;
        Check(!viewer.IsExteriorPress(0,200,{}), "active image drag excludes compacting");
        viewer.CancelPointerGesture();
        viewer.m_metadataOpacity = 1; viewer.m_infoBarRect = RectF::FromXYWH(0,190,100,30);
        Check(!viewer.IsExteriorPress(0,200,{}), "metadata surface excludes compacting");
        viewer.m_metadataOpacity = 0;
        app.m_currentPage = Page::Waterfall;
        Check(!viewer.IsExteriorPress(0,200,{}), "Waterfall retains modal return semantics");
        app.m_currentPage = Page::Viewer; app.m_settings.window_opacity = 1;
        Check(!viewer.IsExteriorPress(0,200,{}), "opaque background excluded");
        app.m_settings.window_opacity = 0.5f; app.m_window.m_translucent = false;
        Check(!viewer.IsExteriorPress(0,200,{}), "opaque window composition excluded");
        app.m_window.m_translucent = true;
        const auto original = RectF::FromXYWH(30,50,120,80);
        const auto grown = app.m_waterfallPage.EnlargeActiveItemRect(original);
        Check(std::abs(grown.Width()-original.Width()-200.0f/3.0f)<0.001f && std::abs(grown.Width()/grown.Height()-1.5f)<0.001f && grown.Center().x==original.Center().x && grown.Center().y==original.Center().y, "hover adds 66.6667 preserving aspect and center");
        viewer.ResetView();
    }

    // Preserves diagnostic aliases for historical scripts while giving neutral names explicit precedence.
    void DiagnosticAliasRegressions() {
        SetEnvironmentVariableW(L"IMAGE_VIEWER_TEST_ALIAS", nullptr);
        SetEnvironmentVariableW(L"LENTEUV_TEST_ALIAS", L"legacy");
        Check(PerformanceTrace::Environment(L"IMAGE_VIEWER_TEST_ALIAS") == L"legacy", "historical diagnostic variable remains readable");
        SetEnvironmentVariableW(L"IMAGE_VIEWER_TEST_ALIAS", L"neutral");
        Check(PerformanceTrace::Environment(L"IMAGE_VIEWER_TEST_ALIAS") == L"neutral", "neutral diagnostic variable wins");
        SetEnvironmentVariableW(L"IMAGE_VIEWER_TEST_ALIAS", L"");
        Check(PerformanceTrace::Environment(L"IMAGE_VIEWER_TEST_ALIAS").empty(), "explicit empty neutral value does not enable historical diagnostics");
        SetEnvironmentVariableW(L"IMAGE_VIEWER_TEST_ALIAS", nullptr);
        SetEnvironmentVariableW(L"LENTEUV_TEST_ALIAS", nullptr);
    }

    // Checks native caption precedence on an owned hidden HWND, without touching the desktop or another app.
    void ClientHitTestRegressions() {
        HWND hidden = CreateWindowExW(0,L"STATIC",L"viewer-hit-tests",WS_POPUP,100,100,800,600,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        Check(hidden != nullptr, "hidden native hit-test window");
        Window window;
        window.m_hwnd = hidden; window.m_dpi = 96; window.m_resizeBorderWidth = 6;
        Check(window.HitTest(200,120) == HTCAPTION, "grid retains native caption band");
        window.events.onClientHitTest = [](float,float) { return true; };
        Check(window.HitTest(200,120) == HTCLIENT, "viewer custom surfaces precede native caption double click");
        Check(window.HitTest(101,150) == HTLEFT && window.HitTest(400,101) == HTTOP, "native resizing borders remain available");
    }

    // Exercises real hidden update/input delivery without a drawable target or GPU submission.
    void HiddenRenderUpdates() {
        AsyncPreviewTests isolated;
        auto& live = isolated.app;
        auto& window = live.m_window;
        live.m_currentPage = Page::Start;
        live.m_renderer.m_width = 800;
        live.m_renderer.m_height = 600;
        live.m_renderer.m_initialized = true; // No context: any accidental drawing in this hidden test fails.
        live.m_frameTimer.Reset();
        window.m_redrawPending = false;
        int deliveries = 0;
        live.m_imageLoader.m_pendingTextures.push_back({nullptr, [&](auto) { ++deliveries; }});
        Check(!live.RenderFrame() && deliveries == 1, "hidden tick delivers pending callback without drawing");
        Check(window.m_redrawPending.load(), "completion retains redraw while hidden");
        Check(!window.ConsumeRedrawRequest() && window.m_redrawPending.load(), "hidden consumption preserves pending frame");
        window.m_redrawPending = false;
        Check(!live.RenderFrame() && !window.m_redrawPending.load(), "settled hidden tick does not invent a frame");
        live.m_animations.Start("test-property", 0, 1, 0.001);
        Sleep(5);
        Check(!live.RenderFrame() && window.m_redrawPending.load() && !live.m_animations.HasActiveAnimations(),
            "last property animation tick retains its final frame while hidden");
        window.m_redrawPending = false;
        Check(!live.RenderFrame() && !window.m_redrawPending.load(), "completed property transition stops invalidating");
        window.RequestRedraw();
        Check(!live.RenderFrame() && window.m_redrawPending.load(), "explicit redraw remains pending across hidden updates");

        int inputs = 0;
        window.events.onKeyDown = [&](uint32_t, KeyModifiers) { ++inputs; };
        window.m_redrawPending = false;
        window.HandleMessage(WM_KEYDOWN, 'O', 0);
        Check(inputs == 1 && window.m_redrawPending.load(), "input dirties state even when handler does not invalidate");
        window.m_redrawPending = false;
        window.HandleMessage(WM_PAINT, 0, 0);
        Check(window.m_redrawPending.load(), "native expose retains redraw after validation");

        live.m_viewerPage.Initialize(&live);
        live.m_currentPage = Page::Viewer;
        live.m_viewerPage.m_mouseInactivityTimer = 10;
        live.m_viewerPage.m_metadataOpacity = 0;
        live.m_animatedImage = std::make_shared<AnimatedImageData>();
        live.m_frameSequencer.SetFrames({1.0}, 1);
        window.m_redrawPending = false;
        Sleep(5);
        Check(!live.RenderFrame() && live.m_frameSequencer.IsComplete() && window.m_redrawPending.load(),
            "finite animation completes and retains its final hidden frame");
        window.m_redrawPending = false;
        Check(!live.RenderFrame() && !window.m_redrawPending.load(), "completed unpaused animation does not request repeated frames");
        live.m_frameSequencer.SetFrames({1000.0}, 0);
        window.m_redrawPending = false;
        Check(!live.RenderFrame() && window.m_redrawPending.load(), "visible progress bar invalidates between source frames");
        live.m_currentPage = Page::Start;
        window.m_redrawPending = false;
        Check(!live.RenderFrame() && !window.m_redrawPending.load(), "background playback does not dirty another page");
        live.m_animatedImage.reset();

        live.m_viewerPage.Initialize(&live);
        live.m_thumbnailStrip.SetFileList({L"synthetic-missing.png"});
        auto& strip = live.m_thumbnailStrip;
        strip.m_thumbnails[0].loadFailed = true;
        strip.m_opacity = 0;
        strip.m_targetOpacity = 1;
        window.m_redrawPending = false;
        strip.Update(0.05);
        Check(strip.m_opacity > 0 && window.m_redrawPending.load(), "strip fade invalidates without pointer input");
        live.m_renderer.m_initialized = false;
    }

    // Runs original regressions plus hidden update/invalidation contracts, with checks active in Release.
    int Run() {
        ProducerShutdownOrder();
        StripReplacement();
        StripMutations();
        StripSuccess();
        StripRebuildEquivalence();
        StripVisibilityEquivalence();
        PreviewMutations();
        SharedPreviews();
        LayoutCacheRegressions();
        TextFormatRegressions();
        UploadValidityRegressions();
        StripPresentationRegressions();
        SharedViewerRegressions();
        DockRegressions();
        ExteriorGestureRegressions();
        DiagnosticAliasRegressions();
        ClientHitTestRegressions();
        HiddenRenderUpdates();
        std::cout << "Passed " << checks << " production async preview checks\n";
        return 0;
    }

private:
    App app;
    int checks = 0;
};
} // namespace kv

// Runs production Settings against wrapper-owned synthetic preferences.
int RunSettingsPreferencesTests();

// Runs the IPC diagnostic parent or sender role; App::Initialize is never called.
int RunIpcDiagnosticTests();
// Runs the isolated real-WIC decoder suite with its own COM and data setup.
int RunImageDecodeTests();
// Runs isolated cache, scheduler, folder and async viewer regressions.
int RunStartupCacheTests();
// Runs bounded child processes exercising loader queues and private IPC shutdown.
int RunShutdownTests(int argc, char* argv[]);

// Dispatches isolated IPC, visual snapshot or benchmark modes; otherwise runs preview/optimization regressions.
int main(int argc, char* argv[]) {
    if (argc > 1 && std::string_view(argv[1]).starts_with("--shutdown-")) return RunShutdownTests(argc, argv);
    if (argc > 1 && std::string_view(argv[1]).starts_with("--ipc-")) return RunIpcDiagnosticTests();
    if (argc > 1 && std::string_view(argv[1]).starts_with("--startup-cache-")) return RunStartupCacheTests();
    if (argc > 1 && std::string_view(argv[1]) == "--decode-tests") return RunImageDecodeTests();
    try {
        const bool preferences = argc == 2 && std::string_view(argv[1]) == "--settings-preferences-tests";
        if (preferences && RunSettingsPreferencesTests() != 0) return 1;
        kv::ScopedComInit com;
        kv::AsyncPreviewTests tests;
        if (preferences) return tests.RunViewerControls();
        if (argc == 2 && std::string_view(argv[1]) == "--cancellation-tests") return tests.RunCancellation();
        if (argc == 3 && std::string_view(argv[1]) == "--dock-snapshot")
            return tests.VisualSnapshot(std::filesystem::path(argv[2]), false, false, false, true);
        if (argc == 3 && std::string_view(argv[1]) == "--optimization-snapshot")
            return tests.VisualSnapshot(std::filesystem::path(argv[2]));
        if (argc == 3 && std::string_view(argv[1]) == "--layout-snapshot")
            return tests.VisualSnapshot(std::filesystem::path(argv[2]), true);
        if (argc == 3 && std::string_view(argv[1]) == "--text-snapshot")
            return tests.VisualSnapshot(std::filesystem::path(argv[2]), false, true);
        if (argc == 3 && std::string_view(argv[1]) == "--text-reference-snapshot")
            return tests.VisualSnapshot(std::filesystem::path(argv[2]), false, true, true);
        if (argc == 2 && std::string_view(argv[1]) == "--layout-benchmark")
            return tests.LayoutBenchmark();
        if (argc == 2 && std::string_view(argv[1]) == "--text-benchmark")
            return tests.TextBenchmark();
        if (argc == 2 && std::string_view(argv[1]) == "--optimization-benchmark")
            return tests.OptimizationBenchmark();
        return tests.Run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
