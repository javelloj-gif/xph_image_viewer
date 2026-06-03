// ============================================================================
// Class: CommentsManager
// Description: CommentsManager parses, updates, and saves spatial annotations/comments associated with images in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Maintain folder comments inside `comments.txt`.
// ============================================================================
#include "common/pch.h"
#include "comments_manager.h"
#include "common/types.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <time.h>

namespace kv {

static std::wstring GetUserDataDir() {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    for (int i = 0; i < 6; ++i) {
        p = p.parent_path();
        if (std::filesystem::exists(p / "user_data")) {
            return (p / "user_data").wstring();
        }
    }
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    return (exeDir / "user_data").wstring();
}

std::wstring CommentsManager::GetCommentsFilePath() const {
    return (std::filesystem::path(GetUserDataDir()) / "comments.txt").wstring();
}

std::wstring CommentsManager::MakeImageKey(const std::wstring& md5, uint64_t diskSize) const {
    return md5 + L"_" + std::to_wstring(diskSize);
}

static std::wstring UnescapeW(const std::wstring& input) {
    std::wstring result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == L'\\' && i + 1 < input.size()) {
            wchar_t next = input[i + 1];
            if (next == L'n') {
                result += L'\n';
                i++;
            } else if (next == L't') {
                result += L'\t';
                i++;
            } else if (next == L'"') {
                result += L'"';
                i++;
            } else if (next == L'\\') {
                result += L'\\';
                i++;
            } else {
                result += input[i];
            }
        } else {
            result += input[i];
        }
    }
    return result;
}

static std::wstring EscapeW(const std::wstring& input) {
    std::wstring result;
    result.reserve(input.size() * 2);
    for (wchar_t c : input) {
        if (c == L'\n') {
            result += L"\\n";
        } else if (c == L'\t') {
            result += L"\\t";
        } else if (c == L'"') {
            result += L"\\\"";
        } else if (c == L'\\') {
            result += L"\\\\";
        } else {
            result += c;
        }
    }
    return result;
}

static std::wstring ReadLineW(std::wifstream& file) {
    std::wstring line;
    if (std::getline(file, line)) {
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        return line;
    }
    return L"";
}

bool CommentsManager::Load() {
    m_comments.clear();
    std::wstring filePath = GetCommentsFilePath();
    if (!std::filesystem::exists(filePath)) {
        return false;
    }

    std::wifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));

    while (file.peek() != WEOF) {
        CommentData comment;
        comment.md5 = ReadLineW(file);
        if (comment.md5.empty() && file.eof()) break;

        try {
            comment.disk_size = std::stoull(ReadLineW(file));
        } catch (...) { comment.disk_size = 0; }

        comment.filepath = ReadLineW(file);
        
        try {
            comment.date = std::stod(ReadLineW(file));
        } catch (...) { comment.date = 0.0; }
        
        comment.date_str = ReadLineW(file);

        try {
            comment.date_edited = std::stod(ReadLineW(file));
        } catch (...) { comment.date_edited = 0.0; }

        comment.date_edited_str = ReadLineW(file);

        try {
            comment.left = std::stof(ReadLineW(file));
            comment.top = std::stof(ReadLineW(file));
            comment.right = std::stof(ReadLineW(file));
            comment.bottom = std::stof(ReadLineW(file));
        } catch (...) {
            comment.left = comment.top = comment.right = comment.bottom = 0.0f;
        }

        comment.text = UnescapeW(ReadLineW(file));
        comment.separator = ReadLineW(file);

        std::wstring key = MakeImageKey(comment.md5, comment.disk_size);
        m_comments[key].push_back(comment);
    }

    return true;
}

bool CommentsManager::Save() {
    std::wstring filePath = GetCommentsFilePath();
    std::wofstream file(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));

    for (const auto& [key, list] : m_comments) {
        for (const auto& el : list) {
            file << el.md5 << L"\n";
            file << el.disk_size << L"\n";
            file << el.filepath << L"\n";
            file << el.date << L"\n";
            file << el.date_str << L"\n";
            file << el.date_edited << L"\n";
            file << el.date_edited_str << L"\n";
            file << el.left << L"\n";
            file << el.top << L"\n";
            file << el.right << L"\n";
            file << el.bottom << L"\n";
            file << EscapeW(el.text) << L"\n";
            file << el.separator << L"\n";
        }
    }

    return true;
}

std::vector<CommentData> CommentsManager::GetCommentsForImage(const std::wstring& md5, uint64_t diskSize) {
    std::wstring key = MakeImageKey(md5, diskSize);
    auto it = m_comments.find(key);
    if (it != m_comments.end()) {
        return it->second;
    }
    return {};
}

void CommentsManager::AddComment(const CommentData& comment) {
    std::wstring key = MakeImageKey(comment.md5, comment.disk_size);
    m_comments[key].push_back(comment);
    Save();
}

void CommentsManager::DeleteComment(const std::wstring& md5, uint64_t diskSize, size_t index) {
    std::wstring key = MakeImageKey(md5, diskSize);
    auto it = m_comments.find(key);
    if (it != m_comments.end() && index < it->second.size()) {
        it->second.erase(it->second.begin() + index);
        if (it->second.empty()) {
            m_comments.erase(it);
        }
        Save();
    }
}

void CommentsManager::UpdateCommentText(const std::wstring& md5, uint64_t diskSize, size_t index, const std::wstring& text) {
    std::wstring key = MakeImageKey(md5, diskSize);
    auto it = m_comments.find(key);
    if (it != m_comments.end() && index < it->second.size()) {
        it->second[index].text = text;
        it->second[index].date_edited = (double)time(nullptr);
        
        time_t rawtime = (time_t)it->second[index].date_edited;
        tm timeinfo = {};
        localtime_s(&timeinfo, &rawtime);
        wchar_t buf[100] = {};
        wcsftime(buf, 100, L"%d %B %Y %H:%M:%S", &timeinfo);
        it->second[index].date_edited_str = buf;

        Save();
    }
}

void CommentsManager::DrawComments(Renderer& renderer, const RectF& imageRect, const std::wstring& md5, uint64_t diskSize, PointF cursorPos) {
    std::wstring key = MakeImageKey(md5, diskSize);
    auto it = m_comments.find(key);
    if (it == m_comments.end()) return;

    for (const auto& comment : it->second) {
        float x1 = imageRect.left + imageRect.Width() * comment.left;
        float y1 = imageRect.top + imageRect.Height() * comment.top;
        float x2 = imageRect.left + imageRect.Width() * comment.right;
        float y2 = imageRect.top + imageRect.Height() * comment.bottom;

        RectF commentRect(x1, y1, x2, y2);
        bool cursorInside = commentRect.Contains(cursorPos.x, cursorPos.y);

        float opacity = cursorInside ? 1.0f : 0.4f;
        Color commentColor = Color::FromRGBA(255, 255, 0, (uint8_t)(255 * opacity));

        renderer.DrawRect(commentRect, commentColor, 1.5f);

        if (cursorInside) {
            std::wstring tooltipText = comment.date_str;
            if (comment.date_edited > 0.0) {
                tooltipText += L"\n(Edited: " + comment.date_edited_str + L")";
            }
            tooltipText += L"\n\n" + comment.text;

            RectF tooltipRect = RectF::FromXYWH(commentRect.left, commentRect.bottom + 5.0f, 320.0f, 120.0f);
            
            renderer.FillRoundedRect(tooltipRect, 4.0f, 4.0f, Color::Black(0.85f));
            renderer.DrawRoundedRect(tooltipRect, 4.0f, 4.0f, Color::White(0.2f), 1.0f);

            RectF textRect = tooltipRect;
            textRect.left += 8.0f;
            textRect.top += 8.0f;
            textRect.right -= 8.0f;
            textRect.bottom -= 8.0f;
            renderer.DrawText(tooltipText, textRect, Color::White(), 11.0f, TextAlign::Left, TextVAlign::Top);
        }
    }
}

} // namespace kv
