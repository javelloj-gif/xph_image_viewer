#pragma once

#include "common/types.h"
#include <string>
#include <atomic>
#include <thread>
#include <filesystem>

namespace kv {

class BoardPage;
class Renderer;

class BoardTagScanner {
public:
    explicit BoardTagScanner(BoardPage* page);
    ~BoardTagScanner();

    BoardTagScanner(const BoardTagScanner&) = delete;
    BoardTagScanner& operator=(const BoardTagScanner&) = delete;

    /// Inicia el barrido e indexación de etiquetas en un hilo de fondo
    void StartScan(const std::filesystem::path& activeDir);

    /// Dibuja la barra de progreso en la parte superior si el escaneo está activo
    void DrawProgressBar(Renderer& renderer);

    /// Cancela de forma segura el escaneo activo y sincroniza el hilo
    void CancelScan();

    /// Retorna si hay un escaneo de tags activo en este momento
    bool IsScanning() const { return m_scanningTags; }

private:
    BoardPage* m_page = nullptr;

    bool m_scanningTags = false;
    std::atomic<size_t> m_scanCurrent{ 0 };
    size_t m_scanTotal = 0;
    std::wstring m_scanDir;
    std::thread m_scanThread;
};

} // namespace kv
