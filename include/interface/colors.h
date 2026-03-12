#pragma once

#include <string>
#include <cstdlib>
#include <cctype>
#include <cstdio>

#if defined(_WIN32)
#include <io.h>
#define SILICORE_ISATTY _isatty
#define SILICORE_FILENO _fileno
#else
#include <unistd.h>
#define SILICORE_ISATTY isatty
#define SILICORE_FILENO fileno
#endif

namespace silicore::interface {

struct Colors {
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* DIM = "\033[2m";
    static constexpr const char* UNDERLINE = "\033[4m";

    static constexpr const char* WHITE = "\033[38;5;255m";
    static constexpr const char* SILVER = "\033[38;5;250m";
    static constexpr const char* GREY = "\033[38;5;245m";
    static constexpr const char* DARK_GREY = "\033[38;5;240m";

    // Silica-X parity: yellow accent is mapped to sky-blue in Silicore-C.
    static constexpr const char* YELLOW = "\033[38;2;135;206;235m";
    static constexpr const char* CYAN = "\033[38;5;51m";
    // Silica-X parity: existing blue is darkened for contrast in Silicore-C.
    static constexpr const char* BLUE = "\033[38;2;91;163;201m";
    static constexpr const char* MAGENTA = "\033[38;5;201m";

    static constexpr const char* GREEN = "\033[38;5;46m";
    static constexpr const char* RED = "\033[38;5;196m";
    static constexpr const char* ORANGE = "\033[38;5;208m";

    static constexpr const char* SKY = "\033[38;2;135;206;235m";
    static constexpr const char* SKY_DARK = "\033[38;2;91;163;201m";

    static constexpr const char* GRAY = GREY;
    static constexpr const char* DARK_GRAY = DARK_GREY;
    static constexpr const char* LIGHT_GREY = SILVER;
    static constexpr const char* LIGHT_GRAY = SILVER;
    static constexpr const char* DEFAULT = RESET;
};

inline bool colors_enabled() {
    const char* force = std::getenv("FORCE_COLOR");
    if (force && *force) {
        std::string value(force);
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (value == "1" || value == "true" || value == "yes" || value == "on") {
            return true;
        }
    }
    if (std::getenv("NO_COLOR") != nullptr) {
        return false;
    }
    return SILICORE_ISATTY(SILICORE_FILENO(stdout)) != 0;
}

inline std::string c(const std::string& text, const char* color) {
    if (!color || !colors_enabled()) {
        return text;
    }
    return std::string(color) + text + Colors::RESET;
}

inline std::string bold(const std::string& text) {
    return c(text, Colors::BOLD);
}

inline std::string dim(const std::string& text) {
    return c(text, Colors::DIM);
}

inline std::string colorize(const std::string& text, const char* color) {
    return c(text, color);
}

} // namespace silicore::interface

#undef SILICORE_ISATTY
#undef SILICORE_FILENO
