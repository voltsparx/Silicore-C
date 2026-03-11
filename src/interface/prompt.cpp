#include "interface/prompt.h"

#include "interface/colors.h"
#include "interface/line_input.h"
#include "interface/symbols.h"
#include "interface/banner.h"
#include "utils/strings.h"

#include <sstream>

namespace silicore::interface {

namespace {

struct PromptState {
    std::string active_module = "profile";
    std::string profile_preset = "balanced";
    std::string surface_preset = "balanced";
    std::string extension_control = "auto";
    std::vector<std::string> plugins;
    std::vector<std::string> filters;
    bool all_plugins = false;
    bool all_filters = false;
    bool html_output = false;
    bool json_output = false;
    bool text_output = true;
    bool csv_output = false;
    std::string output_dir;
};

bool is_scan_command(const CliArgs& args) {
    auto cmd = utils::to_lower(args.command);
    return cmd == "profile" || cmd == "surface" || cmd == "fusion";
}

std::vector<std::string> parse_selectors(const std::string& value) {
    std::vector<std::string> out;
    if (value.find(',') != std::string::npos) {
        auto list = utils::split(value, ',');
        for (auto& item : list) {
            auto trimmed = utils::trim(item);
            if (!trimmed.empty()) {
                out.push_back(trimmed);
            }
        }
        return out;
    }
    std::istringstream iss(value);
    std::string token;
    while (iss >> token) {
        auto trimmed = utils::trim(token);
        if (!trimmed.empty()) {
            out.push_back(trimmed);
        }
    }
    return out;
}

void apply_prompt_outputs(CliArgs& args, PromptState& state) {
    if (!is_scan_command(args)) {
        return;
    }

    if (!args.html_output && !args.json_output && !args.text_output && !args.csv_output) {
        std::string prompt = c(std::string(symbol("action")) + " Output formats (txt, html, json, csv) [txt]: ", Colors::CYAN);
        std::string input = read_line(prompt);
        auto trimmed = utils::trim(input);
        if (trimmed.empty()) {
            args.text_output = true;
            state.text_output = true;
            state.html_output = false;
            state.json_output = false;
            state.csv_output = false;
        } else {
            args.text_output = false;
            args.html_output = false;
            args.json_output = false;
            args.csv_output = false;
            auto parts = utils::split(trimmed, ',');
            for (auto& part : parts) {
                auto key = utils::to_lower(utils::trim(part));
                if (key == "txt" || key == "text" || key == "cli") {
                    args.text_output = true;
                } else if (key == "html") {
                    args.html_output = true;
                } else if (key == "json") {
                    args.json_output = true;
                } else if (key == "csv") {
                    args.csv_output = true;
                } else if (key == "all") {
                    args.text_output = true;
                    args.html_output = true;
                    args.json_output = true;
                    args.csv_output = true;
                }
            }
            if (!args.text_output && !args.html_output && !args.json_output && !args.csv_output) {
                args.text_output = true;
            }
            state.text_output = args.text_output;
            state.html_output = args.html_output;
            state.json_output = args.json_output;
            state.csv_output = args.csv_output;
        }
    }

    if (args.output_dir.empty()) {
        std::string prompt = c(std::string(symbol("action")) + " Output directory [cwd]: ", Colors::CYAN);
        std::string input = read_line(prompt);
        auto trimmed = utils::trim(input);
        if (!trimmed.empty()) {
            args.output_dir = trimmed;
            state.output_dir = trimmed;
        }
    }
}

void apply_state_to_args(PromptState& state, CliArgs& args) {
    if (args.command.empty() && !args.targets.empty()) {
        args.command = state.active_module;
    }
    if (args.profile_preset.empty()) {
        args.profile_preset = state.profile_preset;
    }
    if (args.surface_preset.empty()) {
        args.surface_preset = state.surface_preset;
    }
    if (args.extension_control.empty()) {
        args.extension_control = state.extension_control;
    }
    if (!args.all_plugins && args.plugins.empty()) {
        args.plugins = state.plugins;
        args.all_plugins = state.all_plugins;
    }
    if (!args.all_filters && args.filters.empty()) {
        args.filters = state.filters;
        args.all_filters = state.all_filters;
    }
    if (!args.html_output && !args.json_output && !args.text_output && !args.csv_output) {
        args.html_output = state.html_output;
        args.json_output = state.json_output;
        args.text_output = state.text_output;
        args.csv_output = state.csv_output;
    }
    if (args.output_dir.empty() && !state.output_dir.empty()) {
        args.output_dir = state.output_dir;
    }
}

bool handle_state_command(const std::string& trimmed, PromptState& state) {
    auto lower = utils::to_lower(trimmed);
    if (lower.rfind("use ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(4));
        auto mod = utils::to_lower(value);
        if (mod == "scan" || mod == "persona" || mod == "social") mod = "profile";
        if (mod == "domain" || mod == "asset") mod = "surface";
        if (mod == "full" || mod == "combo") mod = "fusion";
        if (mod == "profile" || mod == "surface" || mod == "fusion") {
            state.active_module = mod;
            std::cout << c(std::string(symbol("feature")) + " Active module: " + mod, Colors::CYAN) << "\n";
            return true;
        }
    }
    if (lower.rfind("select module ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(14));
        auto mod = utils::to_lower(value);
        if (mod == "scan" || mod == "persona" || mod == "social") mod = "profile";
        if (mod == "domain" || mod == "asset") mod = "surface";
        if (mod == "full" || mod == "combo") mod = "fusion";
        if (mod == "profile" || mod == "surface" || mod == "fusion") {
            state.active_module = mod;
            std::cout << c(std::string(symbol("feature")) + " Active module: " + mod, Colors::CYAN) << "\n";
            return true;
        }
    }
    if (lower.rfind("set plugins ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(12));
        if (utils::to_lower(value) == "all") {
            state.all_plugins = true;
            state.plugins.clear();
        } else if (utils::to_lower(value) == "none") {
            state.all_plugins = false;
            state.plugins.clear();
        } else {
            state.plugins = parse_selectors(value);
            state.all_plugins = false;
        }
        std::cout << c(std::string(symbol("feature")) + " Plugin selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("set filters ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(12));
        if (utils::to_lower(value) == "all") {
            state.all_filters = true;
            state.filters.clear();
        } else if (utils::to_lower(value) == "none") {
            state.all_filters = false;
            state.filters.clear();
        } else {
            state.filters = parse_selectors(value);
            state.all_filters = false;
        }
        std::cout << c(std::string(symbol("feature")) + " Filter selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("add plugins ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(12));
        auto selectors = parse_selectors(value);
        for (const auto& sel : selectors) {
            state.plugins.push_back(sel);
        }
        state.all_plugins = false;
        std::cout << c(std::string(symbol("feature")) + " Plugin selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("remove plugins ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(15));
        auto selectors = parse_selectors(value);
        std::vector<std::string> keep;
        for (const auto& plugin : state.plugins) {
            bool remove = false;
            for (const auto& sel : selectors) {
                if (utils::to_lower(plugin) == utils::to_lower(sel)) {
                    remove = true;
                    break;
                }
            }
            if (!remove) {
                keep.push_back(plugin);
            }
        }
        state.plugins.swap(keep);
        state.all_plugins = false;
        std::cout << c(std::string(symbol("feature")) + " Plugin selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("add filters ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(11));
        auto selectors = parse_selectors(value);
        for (const auto& sel : selectors) {
            state.filters.push_back(sel);
        }
        state.all_filters = false;
        std::cout << c(std::string(symbol("feature")) + " Filter selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("remove filters ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(14));
        auto selectors = parse_selectors(value);
        std::vector<std::string> keep;
        for (const auto& filter : state.filters) {
            bool remove = false;
            for (const auto& sel : selectors) {
                if (utils::to_lower(filter) == utils::to_lower(sel)) {
                    remove = true;
                    break;
                }
            }
            if (!remove) {
                keep.push_back(filter);
            }
        }
        state.filters.swap(keep);
        state.all_filters = false;
        std::cout << c(std::string(symbol("feature")) + " Filter selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("set profile_preset ", 0) == 0) {
        state.profile_preset = utils::to_lower(utils::trim(trimmed.substr(19)));
        std::cout << c(std::string(symbol("feature")) + " Profile preset: " + state.profile_preset, Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("set surface_preset ", 0) == 0) {
        state.surface_preset = utils::to_lower(utils::trim(trimmed.substr(19)));
        std::cout << c(std::string(symbol("feature")) + " Surface preset: " + state.surface_preset, Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("set extension_control ", 0) == 0) {
        state.extension_control = utils::to_lower(utils::trim(trimmed.substr(22)));
        std::cout << c(std::string(symbol("feature")) + " Extension control: " + state.extension_control, Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("select plugins ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(15));
        state.plugins = parse_selectors(value);
        state.all_plugins = false;
        std::cout << c(std::string(symbol("feature")) + " Plugin selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("select filters ", 0) == 0) {
        auto value = utils::trim(trimmed.substr(15));
        state.filters = parse_selectors(value);
        state.all_filters = false;
        std::cout << c(std::string(symbol("feature")) + " Filter selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    return false;
}

} // namespace

int run_prompt(const CommandHandler& handler) {
    PromptState state;
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
            std::cout << c("  orchestrate | quicktest | wizard", Colors::GREY) << "\n";
            std::cout << c("  plugins | filters | modules | history | keywords", Colors::GREY) << "\n";
            std::cout << c("  anonymity | live", Colors::GREY) << "\n";
            std::cout << c("  about | explain", Colors::GREY) << "\n";
            std::cout << c("  show plugins | show filters | show platforms", Colors::GREY) << "\n";
            std::cout << c("  banner | help | exit", Colors::GREY) << "\n";
            std::cout << c(std::string(symbol("feature")) + " Prompt controls:", Colors::CYAN) << "\n";
            std::cout << c("  use profile|surface|fusion", Colors::GREY) << "\n";
            std::cout << c("  set plugins <all|none|list>", Colors::GREY) << "\n";
            std::cout << c("  set filters <all|none|list>", Colors::GREY) << "\n";
            std::cout << c("  add/remove plugins <list>", Colors::GREY) << "\n";
            std::cout << c("  add/remove filters <list>", Colors::GREY) << "\n";
            std::cout << c("  set profile_preset <fast|balanced|deep|max>", Colors::GREY) << "\n";
            std::cout << c("  set surface_preset <fast|balanced|deep|max>", Colors::GREY) << "\n";
            std::cout << c("  set extension_control <auto|manual|hybrid>", Colors::GREY) << "\n";
            std::cout << c(std::string(symbol("feature")) + " Outputs:", Colors::CYAN) << "\n";
            std::cout << c("  Formats: txt, html, json, csv", Colors::GREY) << "\n";
            std::cout << c("  Prompt will ask for formats and output directory (default: ./output)", Colors::GREY) << "\n";
            continue;
        }
        if (lower == "banner") {
            show_banner("No Anonymization");
            continue;
        }
        if (handle_state_command(trimmed, state)) {
            continue;
        }
        CliArgs args = parse_line(trimmed);
        args.prompt_mode = true;
        apply_state_to_args(state, args);
        apply_prompt_outputs(args, state);
        handler(args);
    }
    return 0;
}

} // namespace silicore::interface
