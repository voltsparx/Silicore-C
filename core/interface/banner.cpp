#include "core/interface/banner.h"

#include "core/foundation/metadata.h"
#include "core/interface/colors.h"

#include <iostream>
#include <utility>
#include <vector>

namespace silicore::interface {

void show_banner(const std::string& anonymity_status) {
    const std::vector<std::pair<const char*, const char*>> left_right_lines = {
        {"       .d8888. d888888b db      d888888b  .o88b.  .d8b.", "          db    db"},
        {"       88'  YP   `88'   88        `88'   d8P  Y8 d8' `8b", "         `8b  d8'"},
        {"       `8bo.      88    88         88    8P      88ooo88", "          `8bd8'"},
        {"         `Y8b.    88    88         88    8b      88~~~88  C8888D", "  .dPYb."},
        {"       db   8D   .88.   88booo.   .88.   Y8b  d8 88   88", "         .8P  Y8."},
        {"       `8888Y' Y888888P Y88888P Y888888P  `Y88P' YP   YP", "         YP    YP"},
    };

    for (const auto& row : left_right_lines) {
        std::cout << c(row.first, Colors::GREY) << c(row.second, Colors::YELLOW) << "\n";
    }

    std::cout << c(std::string("                                                                          v") + foundation::VERSION, Colors::GREY) << "\n";
    std::cout << "_________________________________________________________________________________\n";
    std::cout << c(
        std::string("    Automated Multi-OSINT Tool - Developed by ") + foundation::AUTHOR +
            " (github.com/" + foundation::AUTHOR + ")",
        Colors::CYAN
    ) << "\n";
    std::cout << c(std::string("                      Current Anonymity: ") + anonymity_status, Colors::CYAN) << "\n\n";
}

} // namespace silicore::interface
