#pragma once

#include "common/types.h"
#include <string>
#include <atomic>
#include <thread>
#include <filesystem>
#include <mutex>

namespace kv {

class BoardPage;
class Renderer;

// UI owns lifecycle; one CPU worker collects tags, with atomic progress and transactional catalog publication.
class BoardTagScanner {
    friend class AsyncPreviewTests; // Installs controlled work to verify App destruction joins producers first.
public:
    // Borrows the page/App, which must outlive CancelScan and the worker it joins.
    explicit BoardTagScanner(BoardPage* page);
    // Requests cooperative cancellation and joins before releasing page dependencies.
    ~BoardTagScanner();

    BoardTagScanner(const BoardTagScanner&) = delete;
    BoardTagScanner& operator=(const BoardTagScanner&) = delete;

    /// UI: cancels/joins the previous scan before starting a fresh scan; resets atomic progress.
    void StartScan(const std::filesystem::path& activeDir);

    /// Dibuja la barra de progreso en la parte superior si el escaneo está activo
    void DrawProgressBar(Renderer& renderer);

    /// UI: requests stop, suppresses catalog publication and joins. Waits only the current E/S/WIC operation.
    void CancelScan();

    /// Retorna si hay un escaneo de tags activo en este momento
    bool IsScanning() const { return m_scanningTags; }

private:
    // Worker-only: checks stop between discovery/files/tags and writes a new catalog only on completion.
    void Scan(const std::filesystem::path& activeDir);
    // Worker: writes an isolated temporary and atomically replaces the catalog unless cancellation won the mutex.
    void PublishCatalog(const std::filesystem::path& activeDir, const std::string& json);
    BoardPage* m_page = nullptr;

    std::atomic<bool> m_scanningTags{false};
    std::atomic<bool> m_cancelled{true};
    std::mutex m_publishMutex; // Serializes stop with final catalog replacement; never held during join.
    std::atomic<size_t> m_scanCurrent{ 0 };
    std::atomic<size_t> m_scanTotal{0};
    std::wstring m_scanDir;
    std::thread m_scanThread;
};

} // namespace kv
