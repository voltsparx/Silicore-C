#include "interface/help_menu.h"

#include "foundation/metadata.h"
#include "interface/colors.h"
#include "interface/symbols.h"
#include "utils/strings.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace silicore::interface {

namespace {

constexpr int kCommandColWidth = 44;
constexpr int kDescriptionGap = 6;
constexpr int kRuleWidth = 118;

std::string pad_right(const std::string& text, int width) {
    if (static_cast<int>(text.size()) >= width) {
        return text;
    }
    return text + std::string(static_cast<size_t>(width - static_cast<int>(text.size())), ' ');
}

void rule(const char* color = Colors::BLUE, int width = kRuleWidth) {
    std::cout << c(std::string(static_cast<size_t>(width), '='), color) << "\n";
}

void section(const std::string& title, const char* color = Colors::BLUE, const std::string& icon = "major") {
    std::cout << "\n";
    rule(color);
    std::string label = "  " + symbol(icon) + " " + title;
    std::cout << c(label, color) << "\n";
    rule(color);
}

void item(const std::string& command, const std::string& description) {
    std::string label = command;
    while (!label.empty() && label.back() == ':') {
        label.pop_back();
    }
    label = utils::trim(label);
    std::string command_label = label + ":";
    std::string gap(static_cast<size_t>(kDescriptionGap), ' ');

    if (static_cast<int>(command_label.size()) > kCommandColWidth) {
        std::cout << c("  " + command_label, Colors::CYAN) << "\n";
        std::cout << c("  " + std::string(static_cast<size_t>(kCommandColWidth), ' '), Colors::CYAN)
                  << c(gap + description, Colors::GREY) << "\n";
        return;
    }

    std::string padded = pad_right(command_label, kCommandColWidth);
    std::cout << c("  " + padded, Colors::CYAN) << c(gap + description, Colors::GREY) << "\n";
}

void render_items(const std::vector<std::pair<std::string, std::string>>& items) {
    for (const auto& entry : items) {
        item(entry.first, entry.second);
    }
}

void example(const std::string& command, const std::string& description) {
    std::cout << c("  " + symbol("action") + " " + command, Colors::YELLOW) << "\n";
    std::cout << c("    " + description, Colors::GREY) << "\n";
}

} // namespace

void show_flag_help() {
    std::cout << c(
        "\n" + std::string(foundation::PROJECT_NAME) + " v" + foundation::VERSION + " [" +
            foundation::VERSION_THEME + "] Flag Help",
        Colors::CYAN
    ) << "\n";
    std::cout << c(std::string(symbol("action")) + " Usage: silicore-c <command> [flags]", Colors::GREY) << "\n";

    section("Global Flags", Colors::BLUE, "feature");
    render_items({
        {"--about:", "Show framework description and exit."},
        {"--explain:", "Show plain-language command and extension guide and exit."},
    });

    section("Primary Workflows");
    render_items({
        {"profile <username...>:", "Scan usernames for profile intelligence."},
        {"surface <domain>:", "Scan a domain for surface exposure signals."},
        {"fusion <username> <domain>:", "Run profile and surface workflows together."},
        {"orchestrate <mode> <target>:", "Run policy-driven layered orchestration."},
        {"wizard:", "Run guided workflow questions."},
        {"quicktest [flags]:", "Run one random built-in victim template with report outputs."},
    });

    section("Inventory and Utility");
    render_items({
        {"plugins [--scope ...]:", "List available plugins."},
        {"filters [--scope ...]:", "List available filters."},
        {"modules [query flags]:", "List/sync/query source-intel module catalog."},
        {"history [--limit N]:", "List previously scanned targets."},
        {"live <target> [--port]:", "Open local live dashboard."},
        {"anonymity [flags]:", "Check or change Tor/proxy routing."},
        {"keywords:", "Show prompt keyword shortcuts."},
        {"about | explain | prompt | help:", "Metadata, explainers, interactive mode, help."},
    });

    section("Extension and Routing Controls");
    render_items({
        {"--plugin / --all-plugins:", "Enable one or all plugins (repeatable/comma-separated selectors)."},
        {"--filter / --all-filters:", "Enable one or all filters (repeatable/comma-separated selectors)."},
        {"--extension-control <mode>:", "auto | manual | hybrid (fail-fast conflict validation)"},
        {"wizard preflight:", "Wizard validates extension compatibility before scans."},
        {"--tor / --proxy:", "Enable Tor/proxy routing."},
        {"--no-tor / --no-proxy:", "Disable Tor/proxy routing."},
        {"--check / --prompt:", "Run diagnostics or guided anonymity setup."},
    });

    section("Output");
    render_items({
        {"--html / --csv:", "Write HTML and CSV artifacts."},
        {"wizard --help:", "Show full wizard flags (phases, presets, selectors, toggles)."},
        {"output/data output/html output/cli output/logs:", "Default artifact directories."},
    });

    section("Quick Start", Colors::BLUE, "tip");
    example(
        "silicore-c profile alice --preset deep --plugin threat_conductor --html --csv:",
        "Run profile intelligence with explicit plugin selection and full artifacts."
    );
    example(
        "silicore-c orchestrate fusion alice --secondary-target example.com --html:",
        "Run orchestration in fusion mode with HTML reporting."
    );
    std::cout << "\n";
}

void show_prompt_help() {
    std::cout << c(
        "\n" + std::string(foundation::PROJECT_NAME) + " v" + foundation::VERSION + " [" +
            foundation::VERSION_THEME + "] Prompt Help",
        Colors::CYAN
    ) << "\n";
    std::cout << c(std::string(symbol("action")) + " Type one command and press Enter.", Colors::GREY) << "\n";

    section("Workflow Commands");
    render_items({
        {"scan <username>:", "Quick profile alias."},
        {"profile <username...>:", "Run profile workflow."},
        {"surface <domain>:", "Run surface workflow."},
        {"fusion <username> <domain>:", "Run fusion workflow."},
        {"orchestrate <mode> <target>:", "Run layered orchestration."},
        {"quicktest [flags]:", "Run one random built-in victim template."},
        {"wizard:", "Guided workflow with prompts."},
    });

    section("Inventory and Session");
    render_items({
        {"show plugins | show filters | show modules:", "Inventory and module intelligence catalog."},
        {"show history [--limit N]:", "List previously scanned targets."},
        {"show config:", "Show prompt defaults and active module."},
        {"anonymity [flags]:", "Check or change Tor/proxy routing."},
        {"show keywords:", "Show all prompt shortcut aliases."},
        {"about | explain | help | clear | exit:", "Metadata, docs, help, clear screen, quit."},
    });

    section("Selection Controls");
    render_items({
        {"use <profile|surface|fusion>:", "Switch active module context."},
        {"select module <profile|surface|fusion>:", "Alias for `use` module switch by name."},
        {"set plugins <none|all|a,b>:", "Set module-compatible plugins (strict compatibility checks)."},
        {"set filters <none|all|a,b>:", "Set module-compatible filters (strict compatibility checks)."},
        {"select plugins <a,b>:", "Alias for `set plugins` (name-based selectors)."},
        {"select filters <a,b>:", "Alias for `set filters` (name-based selectors)."},
        {"add plugins <a,b> / remove plugins <a,b>:", "Incremental plugin control by name."},
        {"add filters <a,b> / remove filters <a,b>:", "Incremental filter control by name."},
    });

    section("Defaults and Modes");
    render_items({
        {"set profile_preset <...>:", "Default profile preset."},
        {"set surface_preset <...>:", "Default surface preset."},
        {"set extension_control <...>:", "Default control mode for active module."},
        {"set orchestrate_extension_control <...>:", "Default control mode for orchestrate."},
    });

    section("Prompt Format");
    std::cout << c(
        "  " + symbol("feature") + " (console <module> ec=<mode> plugins=<set> filters=<set>)>>",
        Colors::CYAN
    ) << "\n";
    std::cout << c("  " + symbol("tip") + " Run 'keywords' to inspect all alias mappings.", Colors::GREY) << "\n";

    section("Prompt Examples", Colors::BLUE, "tip");
    example("show plugins:", "List plugin inventory.");
    example("use fusion:", "Switch prompt context to fusion workflows.");
    example("set plugins threat_conductor,signal_fusion_core:", "Set plugin defaults by name.");
    example("quicktest --seed 7 --html --csv:", "Run deterministic synthetic smoke with reports.");
    std::cout << "\n";
}

void show_help() {
    show_prompt_help();
}

} // namespace silicore::interface
