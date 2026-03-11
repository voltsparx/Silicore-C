#include "core/interface/prompt.h"

#include "core/interface/colors.h"
#include "core/interface/line_input.h"
#include "core/utils/strings.h"

namespace silicore::interface {

int run_prompt(const CommandHandler& handler) {
    while (true) {
        std::string line = read_line(std::string(Colors::SKY) + "silicore-c" + Colors::RESET + " > ");
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
            std::cout << "Commands: profile <username>, surface <domain>, show plugins, show platforms, exit\n";
            continue;
        }
        CliArgs args = parse_line(trimmed);
        handler(args);
    }
    return 0;
}

} // namespace silicore::interface
