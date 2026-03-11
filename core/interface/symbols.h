#pragma once

#include <cctype>
#include <cstdlib>
#include <string>

namespace silicore::interface {

inline bool supports_unicode_stdout() {
    const char* envs[] = {"LC_ALL", "LC_CTYPE", "LANG"};
    for (const char* key : envs) {
        const char* val = std::getenv(key);
        if (!val) {
            continue;
        }
        std::string lower(val);
        for (auto& ch : lower) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (lower.find("utf-8") != std::string::npos || lower.find("utf8") != std::string::npos) {
            return true;
        }
    }
    return false;
}

inline std::string symbol(const std::string& name) {
    std::string key;
    key.reserve(name.size());
    for (char ch : name) {
        key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    static const bool use_unicode = supports_unicode_stdout();
    if (key == "bullet") return use_unicode ? "[\xE2\x80\xA2]" : "[*]";
    if (key == "action") return use_unicode ? "[\xE2\x86\x92]" : "[>]";
    if (key == "ok") return use_unicode ? "[\xE2\x9C\x93]" : "[+]";
    if (key == "error") return use_unicode ? "[\xE2\x9C\x97]" : "[x]";
    if (key == "tip") return use_unicode ? "[\xE2\x98\x85]" : "[i]";
    if (key == "feature") return use_unicode ? "[\xE2\x9C\xA6]" : "[*]";
    if (key == "warn") return "[!]";
    if (key == "major") return use_unicode ? "[\xE2\x95\x90]" : "[=]";
    if (key == "minor") return use_unicode ? "[\xE2\x94\x80]" : "[-]";

    return "[" + key + "]";
}

} // namespace silicore::interface
