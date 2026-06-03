// ============================================================================
// Class: I18nEngine
// Description: I18nEngine loads and queries gettext-compatible localization files for language switching in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Translate wide strings using `.po` runtime tables.
// ============================================================================
#include "common/pch.h"
#include "i18n_engine.h"
#include "common/types.h"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>

namespace kv {

static std::unordered_map<std::wstring, std::wstring> g_translations;
std::wstring I18nEngine::m_lang = L"en";

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static std::string Unescape(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            char next = input[i + 1];
            if (next == 'n') {
                result += '\n';
                i++;
            } else if (next == 't') {
                result += '\t';
                i++;
            } else if (next == '"') {
                result += '"';
                i++;
            } else if (next == '\\') {
                result += '\\';
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

static std::wstring GetPoFilePath(const std::wstring& lang) {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    for (int i = 0; i < 6; ++i) {
        p = p.parent_path();
        std::filesystem::path poPath = p / "locales" / lang / "LC_MESSAGES" / "base.po";
        if (std::filesystem::exists(poPath)) {
            return poPath.wstring();
        }
    }
    return L"";
}

void I18nEngine::Initialize(const std::wstring& lang) {
    m_lang = lang;
    g_translations.clear();

    if (lang == L"en" || lang.empty()) {
        LogInfo(L"Using default English locale.");
        return;
    }

    std::wstring poPath = GetPoFilePath(lang);
    if (poPath.empty()) {
        LogWarn(std::format(L"Translation base.po for '{}' not found", lang));
        return;
    }

    LogInfo(std::format(L"Loading translations from {}", poPath));

    std::ifstream file(poPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        LogError(std::format(L"Failed to open base.po for '{}'", lang));
        return;
    }

    std::string line;
    std::string currentMsgId;
    std::string currentMsgStr;
    enum ParseState { None, InMsgId, InMsgStr };
    ParseState state = None;

    auto finalizeEntry = [&]() {
        if (!currentMsgId.empty() && !currentMsgStr.empty()) {
            std::wstring wId = Utf8ToWide(Unescape(currentMsgId));
            std::wstring wStr = Utf8ToWide(Unescape(currentMsgStr));
            g_translations[wId] = wStr;
        }
        currentMsgId.clear();
        currentMsgStr.clear();
    };

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos) {
            continue;
        }

        if (line[first] == '#') {
            continue;
        }

        if (line.compare(first, 6, "msgid ") == 0) {
            finalizeEntry();
            state = InMsgId;
            size_t quoteStart = line.find('"', first + 6);
            size_t quoteEnd = line.rfind('"');
            if (quoteStart != std::string::npos && quoteEnd > quoteStart) {
                currentMsgId = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            }
        } else if (line.compare(first, 7, "msgstr ") == 0) {
            state = InMsgStr;
            size_t quoteStart = line.find('"', first + 7);
            size_t quoteEnd = line.rfind('"');
            if (quoteStart != std::string::npos && quoteEnd > quoteStart) {
                currentMsgStr = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            }
        } else if (line[first] == '"') {
            size_t quoteEnd = line.rfind('"');
            if (quoteEnd > first) {
                std::string chunk = line.substr(first + 1, quoteEnd - first - 1);
                if (state == InMsgId) {
                    currentMsgId += chunk;
                } else if (state == InMsgStr) {
                    currentMsgStr += chunk;
                }
            }
        }
    }
    finalizeEntry();

    LogInfo(std::format(L"Loaded {} translation entries.", g_translations.size()));
}

std::wstring I18nEngine::Translate(const std::wstring& msgid) {
    auto it = g_translations.find(msgid);
    if (it != g_translations.end()) {
        return it->second;
    }
    return msgid;
}

std::wstring I18nEngine::Translate(const std::string& msgid) {
    std::wstring wId = Utf8ToWide(msgid);
    return Translate(wId);
}

std::wstring I18nEngine::GetLanguage() {
    return m_lang;
}

} // namespace kv
