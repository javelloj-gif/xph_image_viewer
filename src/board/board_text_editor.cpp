// ============================================================================
// Class: BoardTextEditor
// Description: BoardTextEditor processes text edits, cursor positions, and string formatting for active notes on the board of [IMAGE_VIEWER_APP_NAME].
// Responsibility: Manage caret blinking times, text input sequences, character insertion/erasing, and dimension calculations for note items.
// ============================================================================
#include "common/pch.h"
#include "board_text_editor.h"
#include <algorithm>

namespace kv {

void BoardTextEditor::UpdateCursor(double dt, std::function<void()> requestRedrawCallback) {
    if (m_editingNoteIdx != -1) {
        m_cursorBlinkTime += dt;
        if (m_cursorBlinkTime >= 0.5) {
            m_cursorBlinkState = !m_cursorBlinkState;
            m_cursorBlinkTime = 0.0;
            if (requestRedrawCallback) {
                requestRedrawCallback();
            }
        }
    }
}

bool BoardTextEditor::HandleKeyDown(uint32_t vk, std::function<void()> requestRedrawCallback) {
    if (m_editingNoteIdx != -1) {
        if (vk == VK_ESCAPE) {
            StopEditing();
            if (requestRedrawCallback) {
                requestRedrawCallback();
            }
        }
        return true;
    }
    return false;
}

void BoardTextEditor::HandleChar(wchar_t ch, std::vector<BoardItem>& items, std::function<void()> requestRedrawCallback) {
    if (m_editingNoteIdx == -1 || m_editingNoteIdx >= static_cast<int>(items.size())) return;

    auto& item = items[m_editingNoteIdx];
    if (ch == L'\b') {
        if (item.type == BoardItemType::Note) {
            if (!item.plainText.empty()) {
                item.plainText.pop_back();
                if (requestRedrawCallback) requestRedrawCallback();
            }
        } else if (item.type == BoardItemType::Node) {
            if (!item.label.empty()) {
                item.label.pop_back();
                UpdateNodeSize(item);
                if (requestRedrawCallback) requestRedrawCallback();
            }
        }
    } else if (ch == L'\r' || ch == L'\n') {
        if (item.type == BoardItemType::Note) {
            item.plainText.push_back(L'\n');
            if (requestRedrawCallback) requestRedrawCallback();
        } else if (item.type == BoardItemType::Node) {
            StopEditing();
            if (requestRedrawCallback) requestRedrawCallback();
        }
    } else if (ch == 27) {
        StopEditing();
        if (requestRedrawCallback) requestRedrawCallback();
    } else if (ch >= 32) {
        if (item.type == BoardItemType::Note) {
            item.plainText.push_back(ch);
            if (requestRedrawCallback) requestRedrawCallback();
        } else if (item.type == BoardItemType::Node) {
            item.label.push_back(ch);
            UpdateNodeSize(item);
            if (requestRedrawCallback) requestRedrawCallback();
        }
    }
}

void BoardTextEditor::StartEditing(int itemIdx) {
    m_editingNoteIdx = itemIdx;
    m_cursorBlinkState = true;
    m_cursorBlinkTime = 0.0;
}

void BoardTextEditor::StopEditing() {
    m_editingNoteIdx = -1;
    m_cursorBlinkState = false;
    m_cursorBlinkTime = 0.0;
}

void BoardTextEditor::UpdateNodeSize(BoardItem& item) {
    if (item.type != BoardItemType::Node) return;
    // Estimate width based on label length.
    // Standard font size is 13.f. Average character width is around 8.f pixels.
    float textWidth = static_cast<float>(item.label.size()) * 8.0f;
    item.width = std::max(80.0f, textWidth + 30.0f);
    item.height = 80.0f;
}

} // namespace kv
