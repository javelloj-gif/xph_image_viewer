#pragma once

#include "common/types.h"
#include "board_item.h"
#include <vector>
#include <filesystem>

namespace kv {

class BoardPage;

class BoardLayoutHelper {
public:
    explicit BoardLayoutHelper(BoardPage* page);

    /// Calcula una posición libre en espiral alrededor de un centroide para evitar colisiones
    PointF FindSpiralPosition(const PointF& center, float width, float height, float margin, 
                              const std::vector<BoardItem>& extraItems, bool ignoreCanvasItems = false);

    /// Calcula la distancia de un punto a un segmento de línea
    float DistanceToLineSegment(const PointF& p, const PointF& a, const PointF& b) const;

    /// Lista todas las imágenes soportadas en el directorio activo y las inyecta en el lienzo en espiral
    void ListCurrentDirectoryImages();

    /// Construye y despliega el menú contextual de click derecho sobre la posición especificada
    void ShowContextMenu(float screenX, float screenY);

private:
    BoardPage* m_page = nullptr;
};

} // namespace kv
