// ============================================================================
// tagging.h — Image Tagging Subsystem
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "renderer.h"

namespace kv {

struct TagRecord {
    std::wstring md5;
    uint64_t disk_size = 0;
    std::wstring filepath;
};

struct Tag {
    int id = 0;
    std::wstring name;
    std::wstring description;
    std::vector<TagRecord> records;
};

class TagManager {
public:
    TagManager() = default;
    ~TagManager() = default;

    bool Load();
    bool SaveTag(const Tag& tag);
    void DeleteTag(int tagId);

    std::vector<Tag> GetTagsForImage(const std::wstring& md5);
    void ApplyTagsToImage(const std::wstring& md5, uint64_t diskSize, const std::wstring& filepath, const std::vector<std::wstring>& tagNames);
    
    std::vector<Tag> GetAllTags() const;

private:
    std::wstring GetTaggingFolder() const;
    std::pair<std::wstring, std::wstring> GetTagFilePaths(int id) const;

    std::unordered_map<int, Tag> m_tags;
    int m_maxTagId = 0;
};

} // namespace kv
