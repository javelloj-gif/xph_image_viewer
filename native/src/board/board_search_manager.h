#pragma once

#include "common/types.h"
#include "board_item.h"
#include <vector>
#include <string>
#include <filesystem>

namespace kv {

class BoardPage;
class Renderer;

class BoardSearchManager {
public:
    explicit BoardSearchManager(BoardPage* page);

    void Initialize();
    void Update(double dt);
    void Draw(Renderer& renderer);

    // Enrutamiento de inputs
    bool OnMouseMove(float x, float y, KeyModifiers mods);
    bool OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnKeyDown(uint32_t vk, KeyModifiers mods);
    bool OnChar(wchar_t ch);

    // Getters y Setters de estado de búsqueda
    bool IsSearchVisible() const { return m_searchVisible; }
    void SetSearchVisible(bool visible);

    bool IsSearchModeGroup() const { return m_searchModeGroup; }
    void SetSearchModeGroup(bool group) { m_searchModeGroup = group; }

    bool IsHoveredToggleBtn() const { return m_hoveredToggleBtn; }
    void SetHoveredToggleBtn(bool hover) { m_hoveredToggleBtn = hover; }

    RectF GetSearchToggleRect(const SizeF& screenSize) const;
    void LoadSuggestionsFromDirectory(const std::filesystem::path& activeDir);
    void PerformTagQueryAndInsert();

private:
    RectF GetSearchModalBounds() const;
    void UpdateFilteredSuggestions();
    void SelectSuggestion(const std::wstring& suggestion);

    BoardPage* m_page = nullptr;

    bool m_searchVisible = false;
    std::wstring m_searchText;
    std::vector<std::wstring> m_suggestions;
    std::vector<std::wstring> m_filteredSuggestions;
    int m_hoveredSuggestionIdx = -1;
    int m_hoveredSearchButton = 0; // 0 = None, 1 = Search, 2 = Close
    bool m_searchModeGroup = true;   // true = Agrupar y enfocar, false = Filtrar
    bool m_hoveredToggleBtn = false;
};

} // namespace kv
