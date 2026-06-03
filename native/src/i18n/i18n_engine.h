// ============================================================================
// Class: I18nEngine
// Description: I18nEngine loads and queries gettext-compatible localization files for language switching in [IMAGE_VIEWER_APP_NAME].
// Responsibility: Translate wide strings using `.po` runtime tables.
// ============================================================================
#pragma once

#include <string>

namespace kv {

class I18nEngine {
public:
    static void Initialize(const std::wstring& lang);
    static std::wstring Translate(const std::wstring& msgid);
    static std::wstring Translate(const std::string& msgid);

    static std::wstring GetLanguage();

private:
    static std::wstring m_lang;
};

// Global translation macro
#define _(str) kv::I18nEngine::Translate(str)

} // namespace kv
