// ============================================================================
// i18n.h — Internationalization Manager
// Krumassan Image Viewer — Native C++20
// ============================================================================
#pragma once

#include <string>

namespace kv {

class I18n {
public:
    static void Initialize(const std::wstring& lang);
    static std::wstring Translate(const std::wstring& msgid);
    static std::wstring Translate(const std::string& msgid);

    static std::wstring GetLanguage();

private:
    static std::wstring m_lang;
};

// Global translation macro
#define _(str) kv::I18n::Translate(str)

} // namespace kv
