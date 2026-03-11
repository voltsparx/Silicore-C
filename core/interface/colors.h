#pragma once

#include <string>

namespace silicore::interface {

struct Colors {
    static constexpr const char* SKY = "\033[38;2;135;206;235m";
    static constexpr const char* SKY_DARK = "\033[38;2;91;163;201m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREY = "\033[90m";
    static constexpr const char* RESET = "\033[0m";
};

inline std::string colorize(const std::string& text, const char* color) {
    return std::string(color) + text + Colors::RESET;
}

} // namespace silicore::interface
