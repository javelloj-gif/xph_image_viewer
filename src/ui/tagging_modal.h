// ============================================================================
// Class: TaggingModal
// Description: TaggingModal displays a transversal overlay window to manage XMP keywords,
//              star ratings, and general metadata parameters (Title, Creator, Description).
// Responsibility: Render search inputs, lists of tags, star rating widgets, and text fields
//                 with cyberpunk aesthetics.
// ============================================================================
#pragma once

#include "common/types.h"
#include "rendering/renderer.h"
#include <string>
#include <vector>

namespace kv {

class App;

class TaggingModal {
    friend class AsyncPreviewTests; // Checks scaled metadata controls and native drawing without writing image data.
public:
    TaggingModal() = default;
    ~TaggingModal() = default;

    void Initialize(App* app);
    void Show(const std::wstring& filePath);
    void Show(const std::vector<std::wstring>& filePaths);
    void Hide();
    bool IsVisible() const { return m_visible; }

    // UI: draws localized controls in a fitted logical viewport and restores the renderer transform.
    void Draw(Renderer& renderer);
    // UI: pointer positions are mapped into the same logical viewport used for drawing.
    bool OnMouseMove(float x, float y, KeyModifiers mods);
    bool OnMouseDown(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseUp(MouseButton btn, float x, float y, KeyModifiers mods);
    bool OnMouseWheel(float delta, float x, float y, KeyModifiers mods);
    bool OnKeyDown(uint32_t vk, KeyModifiers mods);
    void OnChar(wchar_t ch);

private:
    bool LoadAvailableTags();
    void DrawTagsTab(Renderer& renderer, const RectF& leftPane, const RectF& rightPane);
    // UI: draws translated metadata labels; stored titles, authors and descriptions remain user content.
    void DrawMetadataTab(Renderer& renderer, const RectF& bounds);
    // UI: writes metadata to the selected source files, invalidates known sort attributes and closes the overlay.
    void SaveChanges();
    void GetScrollLimits(float& leftTotalH, float& leftMaxScroll, float& rightTotalH, float& rightMaxScroll);

    App* m_app = nullptr;
    bool m_visible = false;
    std::wstring m_filePath;
    int m_currentTab = 0; // 0 = Tags, 1 = Metadata

    // Tags data
    std::vector<std::wstring> m_imageTags;        // tags of the image (right)
    std::vector<std::wstring> m_availableTags;    // all tags from JSON (left)
    std::vector<std::wstring> m_filteredTags;     // left tags filtered by search
    std::vector<bool> m_selectedAvailableIndices;  // track ctrl multi-selection on the left list

    // Categorized tags
    std::vector<std::wstring> m_categories; // Order of appearance of categories
    std::unordered_map<std::wstring, std::wstring> m_tagToCategory; // Tag to category name mapping

    // Multi-image mode state
    std::vector<std::wstring> m_filePaths;
    std::vector<std::vector<std::wstring>> m_originalFileTags;
    std::vector<std::wstring> m_initialCommonTags;
    bool m_multiImageMode = false;

    // Metadata and Rating
    int m_rating = 0;           // 0 to 5 stars
    int m_hoveredStars = 0;     // 0 to 5 stars on hover
    std::wstring m_title;
    std::wstring m_creator;
    std::wstring m_description;
    std::wstring m_deviceMake;  // Camera make
    std::wstring m_deviceModel; // Camera model

    // Input fields text
    std::wstring m_searchText;      // left search box
    std::wstring m_editingTagText;   // double click renaming text box
    std::wstring m_manualTagText;    // manual tag creation text box
    
    int m_editingTagIdx = -1;  // index of tag in m_imageTags being edited, or -1
    int m_activeInputType = 0; // 0 = None, 1 = Search, 2 = Edit Tag, 3 = Title, 4 = Creator, 5 = Description, 6 = Manual Tag

    // Scroll offsets
    float m_leftScroll = 0.0f;
    float m_rightScroll = 0.0f;
    bool m_leftScrollCaptured = false;
    bool m_rightScrollCaptured = false;
    float m_dragStartY = 0.0f;
    float m_dragStartScrollOffset = 0.0f;

    // Hovered elements coordinates/indexes for hit-testing
    int m_hoveredAvailableIdx = -1;
    int m_hoveredImageTagIdx = -1;
    int m_hoveredTabIdx = -1; // 0 = Tags, 1 = Metadata
    int m_hoveredButton = 0; // 0 = None, 1 = Save, 2 = Cancel, 3 = Left-to-Right Copy Arrow, 4 = Add Tag Button

    // Layout coordinates
    // UI: preserves the established 1152x840 layout within a client-fitted coordinate system.
    float GetLayoutScale() const;
    RectF GetBounds() const;
};

} // namespace kv
