#include "interface/prompt.h"

#include "interface/colors.h"
#include "interface/cli_config.h"
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
    std::string orchestrate_extension_control = "auto";
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

std::string join_selectors(const std::vector<std::string>& items) {
    if (items.empty()) {
        return "none";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << items[i];
    }
    return oss.str();
}

std::string format_selector_set(const std::vector<std::string>& items, bool all) {
    if (all) {
        return "all";
    }
    return join_selectors(items);
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
        args.text_output = state.text_output;
        args.html_output = state.html_output;
        args.json_output = state.json_output;
        args.csv_output = state.csv_output;
        if (!args.text_output && !args.html_output && !args.json_output && !args.csv_output) {
            args.text_output = true;
        }
    }

    if (args.output_dir.empty() && !state.output_dir.empty()) {
        args.output_dir = state.output_dir;
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
        if (utils::to_lower(args.command) == "orchestrate" && !state.orchestrate_extension_control.empty()) {
            args.extension_control = state.orchestrate_extension_control;
        } else {
            args.extension_control = state.extension_control;
        }
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

bool extensions_locked(const PromptState& state) {
    return utils::to_lower(state.extension_control) == "auto";
}

bool has_manual_extensions(const PromptState& state) {
    return state.all_plugins || state.all_filters || !state.plugins.empty() || !state.filters.empty();
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
        if (extensions_locked(state)) {
            std::cout << c(std::string(symbol("warn")) + " Plugin selection is locked while extension_control=auto.", Colors::RED) << "\n";
            return true;
        }
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
        if (extensions_locked(state)) {
            std::cout << c(std::string(symbol("warn")) + " Filter selection is locked while extension_control=auto.", Colors::RED) << "\n";
            return true;
        }
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
        if (extensions_locked(state)) {
            std::cout << c(std::string(symbol("warn")) + " Plugin selection is locked while extension_control=auto.", Colors::RED) << "\n";
            return true;
        }
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
        if (extensions_locked(state)) {
            std::cout << c(std::string(symbol("warn")) + " Plugin selection is locked while extension_control=auto.", Colors::RED) << "\n";
            return true;
        }
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
        if (extensions_locked(state)) {
            std::cout << c(std::string(symbol("warn")) + " Filter selection is locked while extension_control=auto.", Colors::RED) << "\n";
            return true;
        }
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
        if (extensions_locked(state)) {
            std::cout << c(std::string(symbol("warn")) + " Filter selection is locked while extension_control=auto.", Colors::RED) << "\n";
            return true;
        }
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
        auto next = utils::to_lower(utils::trim(trimmed.substr(22)));
        if (next == "auto" && has_manual_extensions(state)) {
            std::cout << c(std::string(symbol("warn")) + " Clear manual plugin/filter selections before switching to auto.", Colors::RED) << "\n";
            return true;
        }
        state.extension_control = next;
        std::cout << c(std::string(symbol("feature")) + " Extension control: " + state.extension_control, Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("set orchestrate_extension_control ", 0) == 0) {
        state.orchestrate_extension_control = utils::to_lower(utils::trim(trimmed.substr(33)));
        std::cout << c(std::string(symbol("feature")) + " Orchestrate extension control: " + state.orchestrate_extension_control, Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("select plugins ", 0) == 0) {
        if (extensions_locked(state)) {
            std::cout << c(std::string(symbol("warn")) + " Plugin selection is locked while extension_control=auto.", Colors::RED) << "\n";
            return true;
        }
        auto value = utils::trim(trimmed.substr(15));
        state.plugins = parse_selectors(value);
        state.all_plugins = false;
        std::cout << c(std::string(symbol("feature")) + " Plugin selection updated.", Colors::CYAN) << "\n";
        return true;
    }
    if (lower.rfind("select filters ", 0) == 0) {
        if (extensions_locked(state)) {
            std::cout << c(std::string(symbol("warn")) + " Filter selection is locked while extension_control=auto.", Colors::RED) << "\n";
            return true;
        }
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
        std::string plugin_view = format_selector_set(state.plugins, state.all_plugins);
        std::string filter_view = format_selector_set(state.filters, state.all_filters);
        std::string prompt = "(console " + state.active_module + " ec=" + state.extension_control +
                             " plugins=" + plugin_view + " filters=" + filter_view + ")>> ";
        prompt = c(prompt, Colors::CYAN);
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
            std::cout << c("  help | config | history [--limit N]", Colors::GREY) << "\n";
            std::cout << c("  quicktest [--template <id>] [--seed N] [--list-templates] [--json]", Colors::GREY) << "\n";
            std::cout << c("  anonymity [--check|--prompt|--tor|--no-tor|--proxy|--no-proxy]", Colors::GREY) << "\n";
            std::cout << c("  plugins [--scope ...] | filters [--scope ...] | modules [flags] | keywords", Colors::GREY) << "\n";
            std::cout << c("  about | explain | banner | clear | exit", Colors::GREY) << "\n";
            std::cout << c("  scan <username> | profile <username...>", Colors::GREY) << "\n";
            std::cout << c("  surface <domain> | fusion <username> <domain>", Colors::GREY) << "\n";
            std::cout << c("  orchestrate <mode> <target>", Colors::GREY) << "\n";
            std::cout << c(std::string(symbol("feature")) + " Prompt controls:", Colors::CYAN) << "\n";
            std::cout << c("  use profile|surface|fusion", Colors::GREY) << "\n";
            std::cout << c("  select module <profile|surface|fusion>", Colors::GREY) << "\n";
            std::cout << c("  set plugins <none|all|selector1,selector2>", Colors::GREY) << "\n";
            std::cout << c("  set filters <none|all|selector1,selector2>", Colors::GREY) << "\n";
            std::cout << c("  select plugins <selector1,selector2>", Colors::GREY) << "\n";
            std::cout << c("  select filters <selector1,selector2>", Colors::GREY) << "\n";
            std::cout << c("  add/remove plugins <selector1,selector2>", Colors::GREY) << "\n";
            std::cout << c("  add/remove filters <selector1,selector2>", Colors::GREY) << "\n";
            std::cout << c("  set profile_preset <fast|quick|balanced|deep|max>", Colors::GREY) << "\n";
            std::cout << c("  set surface_preset <quick|balanced|deep>", Colors::GREY) << "\n";
            std::cout << c("  set extension_control <auto|manual|hybrid>", Colors::GREY) << "\n";
            std::cout << c("  set orchestrate_extension_control <auto|manual|hybrid>", Colors::GREY) << "\n";
            continue;
        }
        if (lower == "banner") {
            show_banner("No Anonymization");
            continue;
        }
        if (lower == "config") {
            std::cout << c(std::string(symbol("feature")) + " " + cli_config_summary(), Colors::GREY) << "\n";
            continue;
        }
        if (lower == "clear") {
            std::cout << "\x1B[2J\x1B[H";
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
