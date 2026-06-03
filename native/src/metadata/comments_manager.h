// ============================================================================
// Class: CommentsManager
// Description: CommentsManager parses, updates, and saves spatial annotations/comments associated with images in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Maintain folder comments inside `comments.txt`.
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "rendering/renderer.h"

namespace kv {

struct CommentData {
    std::wstring md5;
    uint64_t disk_size = 0;
    std::wstring filepath;
    double date = 0.0;
    std::wstring date_str;
    double date_edited = 0.0;
    std::wstring date_edited_str;
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    std::wstring text;
    std::wstring separator;
};

class CommentsManager {
public:
    CommentsManager() = default;
    ~CommentsManager() = default;

    bool Load();
    bool Save();

    std::vector<CommentData> GetCommentsForImage(const std::wstring& md5, uint64_t diskSize);
    void AddComment(const CommentData& comment);
    void DeleteComment(const std::wstring& md5, uint64_t diskSize, size_t index);
    void UpdateCommentText(const std::wstring& md5, uint64_t diskSize, size_t index, const std::wstring& text);

    void DrawComments(Renderer& renderer, const RectF& imageRect, const std::wstring& md5, uint64_t diskSize, PointF cursorPos);

private:
    std::wstring GetCommentsFilePath() const;
    std::wstring MakeImageKey(const std::wstring& md5, uint64_t diskSize) const;

    std::unordered_map<std::wstring, std::vector<CommentData>> m_comments;
};

} // namespace kv
