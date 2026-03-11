#include "core/interface/prompt.h"

#include "core/interface/colors.h"
#include "core/interface/line_input.h"
#include "core/interface/symbols.h"
#include "core/interface/banner.h"
#include "core/utils/strings.h"

namespace silicore::interface {

namespace {

bool is_scan_command(const CliArgs& args) {
    auto cmd = utils::to_lower(args.command);
    return cmd == "profile" || cmd == "surface" || cmd == "fusion";
}

void apply_prompt_outputs(CliArgs& args) {
    if (!is_scan_command(args)) {
        return;
    }

    if (!args.html_output && !args.json_output && !args.text_output) {
        std::string prompt = c(std::string(symbol("action")) + " Output formats (txt, html, json) [txt]: ", Colors::CYAN);
        std::string input = read_line(prompt);
        auto trimmed = utils::trim(input);
        if (trimmed.empty()) {
            args.text_output = true;
        } else {
            args.text_output = false;
            auto parts = utils::split(trimmed, ',');
            for (auto& part : parts) {
                auto key = utils::to_lower(utils::trim(part));
                if (key == "txt" || key == "text" || key == "cli") {
                    args.text_output = true;
                } else if (key == "html") {
                    args.html_output = true;
                } else if (key == "json") {
                    args.json_output = true;
                } else if (key == "all") {
                    args.text_output = true;
                    args.html_output = true;
                    args.json_output = true;
                }
            }
            if (!args.text_output && !args.html_output && !args.json_output) {
                args.text_output = true;
            }
        }
    }

    if (args.output_dir.empty()) {
        std::string prompt = c(std::string(symbol("action")) + " Output directory [cwd]: ", Colors::CYAN);
        std::string input = read_line(prompt);
        auto trimmed = utils::trim(input);
        if (!trimmed.empty()) {
            args.output_dir = trimmed;
        }
    }
}

} // namespace

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
            std::cout << c("  fusion <username> <domain>", Colors::GREY) << "\n";
            std::cout << c("  show plugins | show platforms", Colors::GREY) << "\n";
            std::cout << c("  banner | help | exit", Colors::GREY) << "\n";
            std::cout << c(std::string(symbol("feature")) + " Outputs:", Colors::CYAN) << "\n";
            std::cout << c("  Prompt will ask for formats and output directory (default: ./output)", Colors::GREY) << "\n";
            continue;
        }
        if (lower == "banner") {
            show_banner("No Anonymization");
            continue;
        }
        CliArgs args = parse_line(trimmed);
        apply_prompt_outputs(args);
        handler(args);
    }
    return 0;
}

} // namespace silicore::interface
