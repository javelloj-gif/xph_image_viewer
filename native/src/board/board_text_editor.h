// ============================================================================
// Class: BoardTextEditor
// Description: BoardTextEditor processes text edits, cursor positions, and string formatting for active notes on the board of [IMAGE_VIEWER_APP_NAME].
// Responsibility: Manage caret blinking times, text input sequences, character insertion/erasing, and dimension calculations for note items.
// ============================================================================
#pragma once

#include "board_item.h"
#include <vector>

namespace kv {

class BoardTextEditor {
public:
    BoardTextEditor() = default;
    ~BoardTextEditor() = default;

    void UpdateCursor(double dt, std::function<void()> requestRedrawCallback);
    bool HandleKeyDown(uint32_t vk, std::function<void()> requestRedrawCallback);
    void HandleChar(wchar_t ch, std::vector<BoardItem>& items, std::function<void()> requestRedrawCallback);

    void StartEditing(int itemIdx);
    void StopEditing();
    bool IsEditing() const { return m_editingNoteIdx != -1; }
    int GetEditingIndex() const { return m_editingNoteIdx; }

    static void UpdateNodeSize(BoardItem& item);

    int m_editingNoteIdx = -1;
    bool m_cursorBlinkState = false;
    double m_cursorBlinkTime = 0.0;
};

} // namespace kv
