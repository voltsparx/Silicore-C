#pragma once

#include <string>

namespace silicore::foundation {

inline constexpr const char* PROJECT_NAME = "Silicore-C";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* VERSION_THEME = "Crystal Lattice";
inline constexpr const char* AUTHOR = "Voltsparx";
inline constexpr const char* AUTHOR_HANDLE = "voltsparx";
inline constexpr const char* CONTACT_EMAILS = "voltsparx@gmail.com, voltsparx303@gmail.com";
inline constexpr const char* PROJECT_URL = "https://github.com/voltsparx/Silica-X";

inline std::string framework_signature() {
    return std::string(PROJECT_NAME) + " v" + VERSION + " [" + VERSION_THEME + "] by " + AUTHOR + " (" + CONTACT_EMAILS + ")";
}

} // namespace silicore::foundation
