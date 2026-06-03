// ============================================================================
// Class: None
// Description: tagging_tests implements test assertions to verify metadata reading, writing, and search behaviors in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Unit-test core tags R/W, binary cache loads, and asynchronous cancel search pipelines.
// ============================================================================
#include "common/pch.h"
#include "metadata/xmp_tag_manager.h"
#include "loader/image_loader.h"
#include <iostream>
#include <cassert>
#include <filesystem>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <io.h>

using namespace kv;

// Simple assertions for tests
#define ASSERT_TRUE(x) \
    do { \
        if (!(x)) { \
            std::wcerr << L"[FAIL] Assertion failed: " << L## #x << L" at line " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while(0)

#define ASSERT_FALSE(x) \
    do { \
        if (x) { \
            std::wcerr << L"[FAIL] Assertion NOT(x) failed: " << L## #x << L" at line " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while(0)

// ─── Test 1: Write & Read Tags (JPEG) ────────────────────────────────────────

void TestReadWriteTagsJpeg(XmpTagManager& manager, const std::wstring& testFilePath) {
    std::wcout << L"[TEST 1] JPEG Read/Write Tags..." << std::endl;

    std::vector<std::wstring> originalTags = { L"unit-test", L"automated", L"xmp-metadata", L"2026" };

    bool writeOk = manager.WriteTags(testFilePath, originalTags);
    ASSERT_TRUE(writeOk);

    std::vector<std::wstring> readTags;
    bool readOk = manager.ReadTags(testFilePath, readTags);
    ASSERT_TRUE(readOk);

    std::wcout << L"  Original tags: " << originalTags.size() << L", Read back: " << readTags.size() << std::endl;
    for (size_t i = 0; i < readTags.size(); ++i) {
        std::wcout << L"    [" << i << L"]: " << readTags[i] << std::endl;
    }

    ASSERT_TRUE(readTags.size() == originalTags.size());
    for (const auto& tag : originalTags) {
        auto it = std::find_if(readTags.begin(), readTags.end(),
            [&tag](const std::wstring& t) { return _wcsicmp(t.c_str(), tag.c_str()) == 0; });
        ASSERT_TRUE(it != readTags.end());
    }

    std::wcout << L"[TEST 1] PASSED." << std::endl;
}

// ─── Test 2: Write & Read Tags (PNG) — Raw iTXt Injection ────────────────────

void TestReadWriteTagsPng(XmpTagManager& manager, const std::wstring& testFilePath) {
    std::wcout << L"[TEST 2] PNG Read/Write Tags (raw iTXt injection)..." << std::endl;

    std::vector<std::wstring> originalTags = { L"vacation", L"nature", L"2026", L"png-xmp-test" };

    bool writeOk = manager.WriteTags(testFilePath, originalTags);
    ASSERT_TRUE(writeOk);

    std::vector<std::wstring> readTags;
    bool readOk = manager.ReadTags(testFilePath, readTags);
    ASSERT_TRUE(readOk);

    std::wcout << L"  Original tags: " << originalTags.size() << L", Read back: " << readTags.size() << std::endl;
    for (size_t i = 0; i < readTags.size(); ++i) {
        std::wcout << L"    [" << i << L"]: " << readTags[i] << std::endl;
    }

    ASSERT_TRUE(readTags.size() == originalTags.size());
    for (const auto& tag : originalTags) {
        auto it = std::find_if(readTags.begin(), readTags.end(),
            [&tag](const std::wstring& t) { return _wcsicmp(t.c_str(), tag.c_str()) == 0; });
        if (it == readTags.end()) {
            std::wcerr << L"  [FAIL] Tag not found: " << tag << std::endl;
        }
        ASSERT_TRUE(it != readTags.end());
    }

    // Test overwriting: write new tags to same file (must replace old ones)
    std::vector<std::wstring> newTags = { L"overwritten", L"replacement-tag" };
    writeOk = manager.WriteTags(testFilePath, newTags);
    ASSERT_TRUE(writeOk);

    std::vector<std::wstring> readTags2;
    manager.ReadTags(testFilePath, readTags2);
    ASSERT_TRUE(readTags2.size() == newTags.size());
    for (const auto& tag : newTags) {
        auto it = std::find_if(readTags2.begin(), readTags2.end(),
            [&tag](const std::wstring& t) { return _wcsicmp(t.c_str(), tag.c_str()) == 0; });
        ASSERT_TRUE(it != readTags2.end());
    }
    // The old tags must NOT be present
    auto oldIt = std::find_if(readTags2.begin(), readTags2.end(),
        [](const std::wstring& t) { return _wcsicmp(t.c_str(), L"vacation") == 0; });
    ASSERT_TRUE(oldIt == readTags2.end());

    std::wcout << L"[TEST 2] PASSED." << std::endl;
}

// ─── Test 3: Cache Persistence ────────────────────────────────────────────────

void TestCachePersistence(IWICImagingFactory2* factory, const std::wstring& testFilePath) {
    std::wcout << L"[TEST 3] Cache Persistence..." << std::endl;

    // Instance 1: Write tag and save cache
    {
        XmpTagManager manager1;
        manager1.Initialize(factory);

        std::vector<std::wstring> tags = { L"cache-test-tag" };
        bool writeOk = manager1.WriteTags(testFilePath, tags);
        ASSERT_TRUE(writeOk);

        bool saveOk = manager1.SaveCache();
        ASSERT_TRUE(saveOk);
    }

    // Instance 2: Load cache and assert hit
    {
        XmpTagManager manager2;
        manager2.Initialize(factory); // Auto-loads cache

        std::atomic<bool> matchFound{ false };
        std::atomic<bool> finished{ false };
        std::wstring rootFolder = std::filesystem::path(testFilePath).parent_path().wstring();

        auto startTime = std::chrono::high_resolution_clock::now();

        manager2.SearchTagsAsync(
            rootFolder,
            { L"cache-test-tag" },
            [&matchFound, testFilePath](const std::wstring& filePath, const std::vector<std::wstring>& tags) {
                if (_wcsicmp(filePath.c_str(), testFilePath.c_str()) == 0) {
                    matchFound.store(true);
                }
            },
            [&finished](size_t scanned, size_t matches) {
                finished.store(true);
            }
        );

        // Wait for search to complete
        while (!finished.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        ASSERT_TRUE(matchFound.load());
        std::wcout << L"  Cache hit search completed in " << durationUs << L" microseconds." << std::endl;
        // Sanity guard: under 5 seconds for a small test directory
        ASSERT_TRUE(durationUs < 5000000LL);
    }

    std::wcout << L"[TEST 3] PASSED." << std::endl;
}

// ─── Test 4: Search Cancellation ─────────────────────────────────────────────

void TestSearchCancellation(IWICImagingFactory2* factory, const std::wstring& testFilePath) {
    std::wcout << L"[TEST 4] Search Cancellation..." << std::endl;

    XmpTagManager manager;
    manager.Initialize(factory);

    std::wstring rootFolder = std::filesystem::path(testFilePath).parent_path().wstring();

    std::atomic<bool> finished{ false };
    manager.SearchTagsAsync(
        rootFolder,
        { L"unit-test" },
        [](const std::wstring&, const std::vector<std::wstring>&) {},
        [&finished](size_t scanned, size_t matches) {
            finished.store(true);
        }
    );

    // Cancel immediately
    manager.CancelSearch();

    // Verify it's no longer running
    ASSERT_FALSE(manager.IsSearchRunning());

    std::wcout << L"[TEST 4] PASSED." << std::endl;
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int wmain() {
    // Enable UTF-16 output for console to correctly print Unicode/wide characters
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    std::wcout << L"========================================" << std::endl;
    std::wcout << L"  Krumassan Image Viewer - Tagging Tests" << std::endl;
    std::wcout << L"========================================" << std::endl;

    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ASSERT_TRUE(SUCCEEDED(hr));

    // Create WIC factory
    ComPtr<IWICImagingFactory2> factory;
    hr = CoCreateInstance(
        CLSID_WICImagingFactory2,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    );
    if (FAILED(hr)) {
        std::wcerr << L"[FAIL] Failed to create WIC factory: 0x" << std::hex << hr << std::endl;
        CoUninitialize();
        return 1;
    }

    // Paths
    std::filesystem::path rootDir = std::filesystem::current_path();
    std::filesystem::path srcJpeg = rootDir / L"test" / L"noname.jpg";
    std::filesystem::path srcPng  = rootDir / L"test" / L"linux.png";
    std::filesystem::path destJpeg = rootDir / L"test" / L"temp_test.jpg";
    std::filesystem::path destPng  = rootDir / L"test" / L"temp_test.png";

    std::wcout << L"Working dir: " << rootDir.wstring() << std::endl;
    std::wcout << L"JPEG source: " << srcJpeg.wstring() << std::endl;
    std::wcout << L"PNG source:  " << srcPng.wstring() << std::endl;

    if (!std::filesystem::exists(srcJpeg)) {
        std::wcerr << L"[FAIL] Source JPEG not found: " << srcJpeg.wstring() << std::endl;
        CoUninitialize();
        return 1;
    }
    if (!std::filesystem::exists(srcPng)) {
        std::wcerr << L"[FAIL] Source PNG not found: " << srcPng.wstring() << std::endl;
        CoUninitialize();
        return 1;
    }

    // Copy test images to temp files
    std::filesystem::copy_file(srcJpeg, destJpeg, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file(srcPng,  destPng,  std::filesystem::copy_options::overwrite_existing);

    try {
        XmpTagManager manager;
        manager.Initialize(factory.Get());

        TestReadWriteTagsJpeg(manager, destJpeg.wstring());
        TestReadWriteTagsPng(manager, destPng.wstring());
        TestCachePersistence(factory.Get(), destPng.wstring());
        TestSearchCancellation(factory.Get(), destPng.wstring());

        std::wcout << std::endl;
        std::wcout << L"========================================" << std::endl;
        std::wcout << L"  ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
        std::wcout << L"========================================" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << std::endl;
        std::filesystem::remove(destJpeg);
        std::filesystem::remove(destPng);
        CoUninitialize();
        return 1;
    } catch (...) {
        std::wcerr << L"[EXCEPTION] Unknown exception!" << std::endl;
        std::filesystem::remove(destJpeg);
        std::filesystem::remove(destPng);
        CoUninitialize();
        return 1;
    }

    // Cleanup temp test images
    std::filesystem::remove(destJpeg);
    std::filesystem::remove(destPng);
    CoUninitialize();
    return 0;
}

// ─── Renderer Stub (linker dependency from image_loader.cpp in console context)
namespace kv {
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> Renderer::CreateBitmapFromMemory(
        const unsigned char*, unsigned int, unsigned int, unsigned int) {
        return nullptr;
    }
}
