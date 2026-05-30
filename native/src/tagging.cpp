// ============================================================================
// tagging.cpp — Image Tagging Subsystem Implementation
// Krumassan Image Viewer — Native C++20
// ============================================================================
#include "pch.h"
#include "tagging.h"
#include "types.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>

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

std::wstring TagManager::GetTaggingFolder() const {
    std::filesystem::path tagsDir = std::filesystem::path(GetUserDataDir()) / "tags";
    std::filesystem::create_directories(tagsDir);
    return tagsDir.wstring();
}

std::pair<std::wstring, std::wstring> TagManager::GetTagFilePaths(int id) const {
    wchar_t filename[100] = {};
    swprintf_s(filename, L"ID%04d", id);
    std::filesystem::path tagsFolder = GetTaggingFolder();
    return {
        (tagsFolder / (std::wstring(filename) + L".info")).wstring(),
        (tagsFolder / (std::wstring(filename) + L".list")).wstring()
    };
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

bool TagManager::Load() {
    m_tags.clear();
    m_maxTagId = 0;

    std::wstring folder = GetTaggingFolder();
    if (!std::filesystem::exists(folder)) return false;

    std::wregex infoPattern(L"ID(\\d{4})\\.info", std::regex_constants::icase);

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;

        std::wstring filename = entry.path().filename().wstring();
        std::wsmatch match;
        if (std::regex_match(filename, match, infoPattern)) {
            int id = std::stoi(match[1].str());
            m_maxTagId = std::max(m_maxTagId, id);

            // Read .info
            std::wifstream infoFile(entry.path(), std::ios::in | std::ios::binary);
            if (!infoFile.is_open()) continue;
            infoFile.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));

            Tag tag;
            tag.id = id;
            tag.name = ReadLineW(infoFile);
            
            // Read description (remaining lines)
            std::wstring descLine;
            while (std::getline(infoFile, descLine)) {
                if (!descLine.empty() && descLine.back() == L'\r') descLine.pop_back();
                if (!tag.description.empty()) tag.description += L"\n";
                tag.description += descLine;
            }
            infoFile.close();

            // Read .list
            auto [infoPath, listPath] = GetTagFilePaths(id);
            if (std::filesystem::exists(listPath)) {
                std::wifstream listFile(listPath, std::ios::in | std::ios::binary);
                if (listFile.is_open()) {
                    listFile.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
                    std::wstring recordLine;
                    while (std::getline(listFile, recordLine)) {
                        if (!recordLine.empty() && recordLine.back() == L'\r') recordLine.pop_back();
                        
                        // Parse: <md5> <disk_size> <filepath>
                        size_t firstSpace = recordLine.find(L' ');
                        if (firstSpace != std::wstring::npos) {
                            size_t secondSpace = recordLine.find(L' ', firstSpace + 1);
                            if (secondSpace != std::wstring::npos) {
                                TagRecord rec;
                                rec.md5 = recordLine.substr(0, firstSpace);
                                std::wstring szStr = recordLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
                                try {
                                    rec.disk_size = std::stoull(szStr);
                                } catch (...) { rec.disk_size = 0; }
                                rec.filepath = recordLine.substr(secondSpace + 1);
                                tag.records.push_back(rec);
                            }
                        }
                    }
                    listFile.close();
                }
            }

            m_tags[id] = tag;
        }
    }

    return true;
}

bool TagManager::SaveTag(const Tag& tag) {
    auto [infoPath, listPath] = GetTagFilePaths(tag.id);

    // Write .info
    std::wofstream infoFile(infoPath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!infoFile.is_open()) return false;
    infoFile.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
    infoFile << tag.name << L"\n" << tag.description;
    infoFile.close();

    // Write .list
    std::wofstream listFile(listPath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!listFile.is_open()) return false;
    listFile.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
    for (const auto& rec : tag.records) {
        listFile << rec.md5 << L" " << rec.disk_size << L" " << rec.filepath << L"\n";
    }
    listFile.close();

    return true;
}

void TagManager::DeleteTag(int tagId) {
    auto [infoPath, listPath] = GetTagFilePaths(tagId);
    std::filesystem::remove(infoPath);
    std::filesystem::remove(listPath);
    m_tags.erase(tagId);
}

std::vector<Tag> TagManager::GetTagsForImage(const std::wstring& md5) {
    std::vector<Tag> result;
    for (const auto& [id, tag] : m_tags) {
        for (const auto& rec : tag.records) {
            if (rec.md5 == md5) {
                result.push_back(tag);
                break;
            }
        }
    }
    return result;
}

void TagManager::ApplyTagsToImage(const std::wstring& md5, uint64_t diskSize, const std::wstring& filepath, const std::vector<std::wstring>& tagNames) {
    // 1. Remove this image from all tags first
    for (auto& [id, tag] : m_tags) {
        auto it = std::remove_if(tag.records.begin(), tag.records.end(), [&](const TagRecord& r) {
            return r.md5 == md5;
        });
        if (it != tag.records.end()) {
            tag.records.erase(it, tag.records.end());
            SaveTag(tag);
        }
    }

    // 2. Add to specified tags
    for (const auto& name : tagNames) {
        if (name.empty()) continue;

        // Find existing tag by name
        Tag* pTag = nullptr;
        for (auto& [id, tag] : m_tags) {
            if (_wcsicmp(tag.name.c_str(), name.c_str()) == 0) {
                pTag = &tag;
                break;
            }
        }

        // If not found, create new
        if (!pTag) {
            m_maxTagId++;
            Tag newTag;
            newTag.id = m_maxTagId;
            newTag.name = name;
            m_tags[newTag.id] = newTag;
            pTag = &m_tags[newTag.id];
        }

        // Add record
        TagRecord rec;
        rec.md5 = md5;
        rec.disk_size = diskSize;
        rec.filepath = filepath;
        pTag->records.push_back(rec);
        SaveTag(*pTag);
    }
}

std::vector<Tag> TagManager::GetAllTags() const {
    std::vector<Tag> result;
    for (const auto& [id, tag] : m_tags) {
        result.push_back(tag);
    }
    return result;
}

} // namespace kv
