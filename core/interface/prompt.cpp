#include "core/interface/prompt.h"

#include "core/interface/colors.h"
#include "core/interface/line_input.h"
#include "core/interface/symbols.h"
#include "core/interface/banner.h"
#include "core/utils/strings.h"

namespace silicore::interface {

int run_prompt(const CommandHandler& handler) {
    while (true) {
        std::string prompt = c(std::string(symbol("feature")) + " silicore-c", Colors::CYAN) + " > ";
        std::string line = read_line(prompt);
        if (line.empty() && last_read_eof()) {
            break;
        }
        auto trimmed = utils::trim(line);
        if (trimmed.empty()) {
            continue;
        }
        add_history(trimmed);
        auto lower = utils::to_lower(trimmed);
        if (lower == "exit" || lower == "quit") {
            break;
        }
        if (lower == "help") {
            std::cout << c(std::string(symbol("action")) + " Commands:", Colors::CYAN) << "\n";
            std::cout << c("  profile <username>", Colors::GREY) << "\n";
            std::cout << c("  surface <domain>", Colors::GREY) << "\n";
            std::cout << c("  show plugins | show platforms", Colors::GREY) << "\n";
            std::cout << c("  banner | help | exit", Colors::GREY) << "\n";
            continue;
        }
        if (lower == "banner") {
            show_banner("No Anonymization");
            continue;
        }
        CliArgs args = parse_line(trimmed);
        handler(args);
    }
    return 0;
}

} // namespace silicore::interface
