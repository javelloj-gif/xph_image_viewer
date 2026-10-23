#include "common/pch.h"
#include "metadata/metadata_cache.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
// Fails in Release too; malformed files must never replace a live cache.
void Require(bool condition) {
    if (!condition) throw std::runtime_error("KTAG transactional load regression");
}

// Appends the native KTAG v1 representation without invoking the product writer.
template<class T> void Append(std::vector<char>& bytes, const T& value) {
    const auto* data = reinterpret_cast<const char*>(&value);
    bytes.insert(bytes.end(), data, data + sizeof(value));
}

// Encodes a UTF-16 string with the historical character-count prefix.
void AppendString(std::vector<char>& bytes, const std::wstring& value) {
    Append(bytes, static_cast<uint32_t>(value.size()));
    const auto* data = reinterpret_cast<const char*>(value.data());
    bytes.insert(bytes.end(), data, data + value.size() * sizeof(wchar_t));
}
}

// Runs only in the copied tagging harness with executable-local user_data, never fixtures.
void TestMalformedMetadataCache() {
    kv::MetadataCache cache;
    const std::filesystem::path path(cache.GetCacheFilePath());
    Require(path.parent_path() == std::filesystem::current_path() / L"user_data");

    // Writes synthetic bytes only to this isolated harness's cache path.
    const auto write = [&](const std::vector<char>& bytes) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        Require(file.good());
    };
    // Restores a live sentinel before every failure and checks both contents and dirty state.
    const auto reject = [&](const std::vector<char>& bytes) {
        for (bool dirty : {false, true}) {
            cache.m_cache.clear();
            cache.m_cache[L"sentinel"] = {73, {L"retained"}};
            cache.m_cacheDirty = dirty;
            write(bytes);
            Require(!cache.LoadCache());
            Require(cache.m_cache.size() == 1 && cache.m_cache.contains(L"sentinel"));
            const auto& entry = cache.m_cache.at(L"sentinel");
            Require(entry.lastWriteTime == 73 && entry.tags == std::vector<std::wstring>{L"retained"});
            Require(cache.m_cacheDirty == dirty);
        }
    };

    std::vector<char> header{'K', 'T', 'A', 'G'};
    Append(header, uint32_t{1});
    std::vector<char> valid = header;
    Append(valid, uint32_t{2});
    Append(valid, int64_t{101});
    AppendString(valid, L"c:/imágenes/uno.png");
    Append(valid, uint32_t{2});
    AppendString(valid, L"etiqueta á");
    AppendString(valid, L"");
    Append(valid, int64_t{102});
    AppendString(valid, L"C:\\imágenes\\uno.png");
    Append(valid, uint32_t{1});
    AppendString(valid, L"última");

    for (size_t size = 0; size < valid.size(); ++size) {
        reject(std::vector<char>(valid.begin(), valid.begin() + size));
    }
    auto invalid = valid;
    invalid[0] = 'X';
    reject(invalid);
    invalid = {'K', 'T', 'A', 'G'};
    Append(invalid, uint32_t{2});
    Append(invalid, uint32_t{0});
    reject(invalid);
    // Impossible entry count, path length, tag count and tag length in tiny files.
    invalid = header;
    Append(invalid, uint32_t{0xffffffff});
    reject(invalid);
    invalid = header;
    Append(invalid, uint32_t{1});
    Append(invalid, int64_t{0});
    Append(invalid, uint32_t{0xffffffff});
    Append(invalid, uint32_t{0});
    reject(invalid);
    invalid = header;
    Append(invalid, uint32_t{1});
    Append(invalid, int64_t{0});
    AppendString(invalid, L"");
    Append(invalid, uint32_t{0xffffffff});
    reject(invalid);
    invalid = header;
    Append(invalid, uint32_t{1});
    Append(invalid, int64_t{0});
    AppendString(invalid, L"");
    Append(invalid, uint32_t{1});
    Append(invalid, uint32_t{0xffffffff});
    reject(invalid);

    write(valid);
    Require(cache.LoadCache());
    Require(!cache.m_cacheDirty && cache.m_cache.size() == 1);
    Require(cache.m_cache.at(L"C:\\imágenes\\uno.png").lastWriteTime == 102);
    Require(cache.m_cache.at(L"C:\\imágenes\\uno.png").tags == std::vector<std::wstring>{L"última"});
    Require(cache.SaveCache());
    kv::MetadataCache restored;
    Require(restored.LoadCache());
    Require(restored.m_cache.at(L"C:\\imágenes\\uno.png").tags == std::vector<std::wstring>{L"última"});

    valid.push_back('x'); // Historical reader tolerated trailing bytes.
    write(valid);
    Require(cache.LoadCache());
    invalid = header;
    Append(invalid, uint32_t{1});
    Append(invalid, int64_t{0});
    AppendString(invalid, L"");
    Append(invalid, uint32_t{0});
    write(invalid);
    Require(cache.LoadCache() && cache.m_cache.contains(L""));
    invalid = header;
    Append(invalid, uint32_t{0});
    write(invalid);
    Require(cache.LoadCache() && cache.m_cache.empty() && !cache.m_cacheDirty);
    std::wcout << L"KTAG: " << valid.size() - 1
               << L" truncations (both dirty states), malformed lengths and compatibility passed.\n";
}
