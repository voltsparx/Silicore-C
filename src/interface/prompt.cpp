#include "interface/prompt.h"

#include "extensions/control_plane.h"
#include "interface/banner.h"
#include "interface/cli_config.h"
#include "interface/cli_parser.h"
#include "interface/colors.h"
#include "interface/line_input.h"
#include "interface/symbols.h"
#include "utils/strings.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace silicore::interface {

namespace {

const std::unordered_set<std::string> kPromptShowCommands = {
    "plugins", "filters", "modules", "history", "keywords", "config",
};

struct PromptState {
    std::string module = "profile";
    std::vector<std::string> plugin_names;
    std::vector<std::string> filter_names;
    std::vector<std::string> history;
    bool all_plugins = false;
    bool all_filters = false;
    std::string profile_preset = "balanced";
    std::string surface_preset = "balanced";
    std::string profile_extension_control = "manual";
    std::string surface_extension_control = "manual";
    std::string fusion_extension_control = "manual";
    std::string orchestrate_extension_control = "auto";
    bool use_tor = false;
    bool use_proxy = false;

    std::string plugins_label() const {
        if (all_plugins) {
            return "all";
        }
        if (plugin_names.empty()) {
            return "none";
        }
        return utils::join(plugin_names, ",");
    }

    std::string filters_label() const {
        if (all_filters) {
            return "all";
        }
        if (filter_names.empty()) {
            return "none";
        }
        return utils::join(filter_names, ",");
    }

    std::string extension_control_for_module(const std::string& module_name) const {
        auto normalized = utils::to_lower(utils::trim(module_name));
        if (normalized == "surface") {
            return surface_extension_control;
        }
        if (normalized == "fusion") {
            return fusion_extension_control;
        }
        return profile_extension_control;
    }

    void set_extension_control_for_module(const std::string& module_name, const std::string& value) {
        auto normalized = utils::to_lower(utils::trim(module_name));
        auto lowered = utils::to_lower(utils::trim(value));
        if (normalized == "surface") {
            surface_extension_control = lowered;
        } else if (normalized == "fusion") {
            fusion_extension_control = lowered;
        } else {
            profile_extension_control = lowered;
        }
    }

    std::string module_prompt() const {
        auto control_label = extension_control_for_module(module);
        auto compact_values = [](const std::vector<std::string>& values, size_t max_items) {
            if (values.empty()) {
                return std::string("none");
            }
            if (values.size() <= max_items) {
                return utils::join(values, ",");
            }
            std::vector<std::string> shown(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(max_items));
            auto remaining = values.size() - max_items;
            return utils::join(shown, ",") + ",+" + std::to_string(remaining);
        };

        std::string plugin_label = all_plugins ? "all" : compact_values(plugin_names, 2);
        std::string filter_label = all_filters ? "all" : compact_values(filter_names, 2);
        return "(console " + module + " ec=" + control_label +
               " plugins=" + plugin_label + " filters=" + filter_label + ")>>";
    }
};

std::string keyword_to_command(const std::string& value) {
    auto lowered = utils::to_lower(utils::trim(value));
    if (lowered.empty()) {
        return "";
    }
    const auto& map = prompt_keywords();
    for (const auto& entry : map) {
        for (const auto& keyword : entry.second) {
            if (utils::to_lower(keyword) == lowered) {
                return entry.first;
            }
        }
    }
    return "";
}

std::vector<std::string> rewrite_tokens_with_keywords(const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return tokens;
    }
    auto mapped = keyword_to_command(tokens.front());
    if (mapped.empty()) {
        return tokens;
    }
    std::vector<std::string> out = tokens;
    out[0] = mapped;
    return out;
}

bool tokenize_line(const std::string& line, std::vector<std::string>& tokens) {
    tokens.clear();
    std::string current;
    char quote = '\0';
    bool escape = false;
    for (char ch : line) {
        if (escape) {
            current.push_back(ch);
            escape = false;
            continue;
        }
        if (ch == '\\') {
            if (quote != '\0') {
                escape = true;
                continue;
            }
        }
        if (quote != '\0') {
            if (ch == quote) {
                quote = '\0';
            } else {
                current.push_back(ch);
            }
            continue;
        }
        if (ch == '"' || ch == '\'') {
            quote = ch;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (escape) {
        current.push_back('\\');
    }
    if (quote != '\0') {
        return false;
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return true;
}

std::unordered_set<std::string> extract_explicit_flags(const std::vector<std::string>& tokens) {
    std::unordered_set<std::string> flags;
    for (const auto& token : tokens) {
        if (token.rfind("--", 0) != 0) {
            continue;
        }
        auto flag = utils::to_lower(token);
        auto eq = flag.find('=');
        if (eq != std::string::npos) {
            flag = flag.substr(0, eq);
        }
        flags.insert(flag);
    }
    return flags;
}

std::string normalize_module(const std::string& value) {
    auto lowered = utils::to_lower(utils::trim(value));
    if (lowered == "profile" || lowered == "surface" || lowered == "fusion") {
        return lowered;
    }
    return "profile";
}

std::string module_for_command(const std::string& command) {
    auto lowered = utils::to_lower(utils::trim(command));
    if (lowered == "profile" || lowered == "scan" || lowered == "persona" || lowered == "social") {
        return "profile";
    }
    if (lowered == "surface" || lowered == "domain" || lowered == "asset") {
        return "surface";
    }
    if (lowered == "fusion" || lowered == "full" || lowered == "combo") {
        return "fusion";
    }
    return "";
}

std::string scope_for_args(const CliArgs& args, const PromptState& session) {
    auto scope = module_for_command(args.command);
    if (!scope.empty()) {
        return scope;
    }
    if (args.command == "orchestrate") {
        if (!args.targets.empty()) {
            return normalize_module(args.targets.front());
        }
        return normalize_module(session.module);
    }
    return "";
}

std::string scan_mode_for_scope(const PromptState& session, const std::string& scope) {
    auto normalized = normalize_module(scope);
    if (normalized == "surface") {
        return session.surface_preset;
    }
    if (normalized == "fusion") {
        return extensions::merge_scan_modes(session.profile_preset, session.surface_preset);
    }
    return session.profile_preset;
}

struct ResolvedNames {
    std::vector<std::string> selected;
    std::vector<std::string> rejected;
};
ResolvedNames resolve_names_for_scope(const std::vector<std::string>& names, const std::string& scope, const std::string& kind) {
    auto descriptors = extensions::list_descriptors(kind, scope);
    std::unordered_map<std::string, std::string> lookup;
    std::unordered_set<std::string> ids;
    extensions::build_lookup(descriptors, lookup, ids);
    auto resolved = extensions::resolve_selector_ids(names, lookup);
    return {resolved.first, resolved.second};
}

std::vector<std::string> split_selectors(const std::string& value) {
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

std::vector<std::string> dedupe_names(const std::vector<std::string>& values) {
    std::vector<std::string> deduped;
    std::unordered_set<std::string> seen;
    for (const auto& item : values) {
        auto key = utils::to_lower(utils::trim(item));
        if (key.empty()) {
            continue;
        }
        if (seen.insert(key).second) {
            deduped.push_back(key);
        }
    }
    return deduped;
}

std::vector<std::string> validate_extension_combo(
    const PromptState& session,
    const std::string& scope,
    const std::vector<std::string>& plugins,
    const std::vector<std::string>& filters,
    bool all_plugins,
    bool all_filters
) {
    auto plan = extensions::resolve_extension_control(
        scope,
        scan_mode_for_scope(session, scope),
        "manual",
        plugins,
        filters,
        all_plugins,
        all_filters
    );
    return plan.errors;
}

void apply_prompt_defaults(CliArgs& args, PromptState& session, const std::unordered_set<std::string>& explicit_flags) {
    auto scope = scope_for_args(args, session);
    if (scope.empty()) {
        return;
    }

    if (args.plugins.empty() && !args.all_plugins) {
        if (session.all_plugins) {
            args.all_plugins = true;
            args.plugins.clear();
        } else {
            auto resolved = resolve_names_for_scope(session.plugin_names, scope, "plugin");
            args.plugins = resolved.selected;
            args.all_plugins = false;
        }
    }

    if (args.filters.empty() && !args.all_filters) {
        if (session.all_filters) {
            args.all_filters = true;
            args.filters.clear();
        } else {
            auto resolved = resolve_names_for_scope(session.filter_names, scope, "filter");
            args.filters = resolved.selected;
            args.all_filters = false;
        }
    }

    if (!explicit_flags.count("--extension-control")) {
        if (args.command == "orchestrate") {
            args.extension_control = session.orchestrate_extension_control;
        } else {
            args.extension_control = session.extension_control_for_module(scope);
        }
    }

    if (module_for_command(args.command) == "profile") {
        if (!explicit_flags.count("--preset")) {
            args.preset = session.profile_preset;
        }
        return;
    }

    if (module_for_command(args.command) == "surface") {
        if (!explicit_flags.count("--preset")) {
            args.preset = session.surface_preset;
        }
        return;
    }

    if (module_for_command(args.command) == "fusion") {
        if (!explicit_flags.count("--profile-preset")) {
            args.profile_preset = session.profile_preset;
        }
        if (!explicit_flags.count("--surface-preset")) {
            args.surface_preset = session.surface_preset;
        }
        return;
    }

    if (args.command == "orchestrate") {
        if (!explicit_flags.count("--profile")) {
            args.profile_preset = (scope == "surface") ? session.surface_preset : session.profile_preset;
        }
    }
}

bool handle_prompt_use_command(const std::string& command_text, PromptState& session) {
    std::istringstream iss(command_text);
    std::string verb;
    std::string value;
    if (!(iss >> verb >> value)) {
        std::cout << c("Usage: use <profile|surface|fusion>", Colors::YELLOW) << "\n";
        return true;
    }
    auto module = utils::to_lower(value);
    if (module != "profile" && module != "surface" && module != "fusion") {
        std::cout << c("Unknown module: " + module, Colors::YELLOW) << "\n";
        return true;
    }
    session.module = module;
    std::cout << c(std::string(symbol("feature")) + " Active module: " + module, Colors::CYAN) << "\n";

    if (!session.all_plugins) {
        auto resolved = resolve_names_for_scope(session.plugin_names, module, "plugin");
        session.plugin_names = resolved.selected;
        if (!resolved.rejected.empty()) {
            std::cout << c(
                std::string(symbol("warn")) + " Removed incompatible plugins for module '" + module + "': " +
                    utils::join(resolved.rejected, ", "),
                Colors::YELLOW
            ) << "\n";
        }
    }
    if (!session.all_filters) {
        auto resolved = resolve_names_for_scope(session.filter_names, module, "filter");
        session.filter_names = resolved.selected;
        if (!resolved.rejected.empty()) {
            std::cout << c(
                std::string(symbol("warn")) + " Removed incompatible filters for module '" + module + "': " +
                    utils::join(resolved.rejected, ", "),
                Colors::YELLOW
            ) << "\n";
        }
    }
    return true;
}

bool mutate_selection(
    PromptState& session,
    const std::string& scope,
    const std::string& kind,
    const std::string& action,
    const std::string& value
) {
    if (session.extension_control_for_module(scope) == "auto") {
        std::cout << c(
            "Cannot " + action + " " + kind + " while extension_control=auto. "
            "Use `set extension_control manual` or `set extension_control hybrid` first.",
            Colors::RED
        ) << "\n";
        return true;
    }
    auto requested = split_selectors(value);
    if (requested.empty()) {
        std::cout << c("Provide at least one selector.", Colors::YELLOW) << "\n";
        return true;
    }

    if (kind == "plugins") {
        if (session.all_plugins) {
            std::cout << c("Cannot mutate plugins while selection is `all`. Use `set plugins none` first.", Colors::RED) << "\n";
            return true;
        }
        auto resolved = resolve_names_for_scope(requested, scope, "plugin");
        if (!resolved.rejected.empty()) {
            std::cout << c(
                "Plugin selection blocked for module '" + scope + "'. Incompatible selectors: " +
                    utils::join(resolved.rejected, ", "),
                Colors::RED
            ) << "\n";
            std::cout << c("Use `show plugins --scope " + scope + "` to inspect selectors.", Colors::YELLOW) << "\n";
            return true;
        }
        auto current = dedupe_names(session.plugin_names);
        std::vector<std::string> updated;
        if (action == "add") {
            updated = dedupe_names([&]() {
                std::vector<std::string> merged = current;
                merged.insert(merged.end(), resolved.selected.begin(), resolved.selected.end());
                return merged;
            }());
        } else {
            std::unordered_set<std::string> remove_set;
            for (const auto& item : resolved.selected) {
                remove_set.insert(utils::to_lower(item));
            }
            for (const auto& item : current) {
                if (!remove_set.count(utils::to_lower(item))) {
                    updated.push_back(item);
                }
            }
        }
        auto errors = validate_extension_combo(session, scope, updated, session.filter_names, false, session.all_filters);
        if (!errors.empty()) {
            for (const auto& error : errors) {
                std::cout << c("Plugin selection blocked: " + error, Colors::RED) << "\n";
            }
            return true;
        }
        session.all_plugins = false;
        session.plugin_names = updated;
        std::cout << c("Plugins set to: " + session.plugins_label() + " (module=" + scope + ")", Colors::GREEN) << "\n";
        return true;
    }

    if (session.all_filters) {
        std::cout << c("Cannot mutate filters while selection is `all`. Use `set filters none` first.", Colors::RED) << "\n";
        return true;
    }
    auto resolved = resolve_names_for_scope(requested, scope, "filter");
    if (!resolved.rejected.empty()) {
        std::cout << c(
            "Filter selection blocked for module '" + scope + "'. Incompatible selectors: " +
                utils::join(resolved.rejected, ", "),
            Colors::RED
        ) << "\n";
        std::cout << c("Use `show filters --scope " + scope + "` to inspect selectors.", Colors::YELLOW) << "\n";
        return true;
    }
    auto current = dedupe_names(session.filter_names);
    std::vector<std::string> updated;
    if (action == "add") {
        updated = dedupe_names([&]() {
            std::vector<std::string> merged = current;
            merged.insert(merged.end(), resolved.selected.begin(), resolved.selected.end());
            return merged;
        }());
    } else {
        std::unordered_set<std::string> remove_set;
        for (const auto& item : resolved.selected) {
            remove_set.insert(utils::to_lower(item));
        }
        for (const auto& item : current) {
            if (!remove_set.count(utils::to_lower(item))) {
                updated.push_back(item);
            }
        }
    }
    auto errors = validate_extension_combo(session, scope, session.plugin_names, updated, session.all_plugins, false);
    if (!errors.empty()) {
        for (const auto& error : errors) {
            std::cout << c("Filter selection blocked: " + error, Colors::RED) << "\n";
        }
        return true;
    }
    session.all_filters = false;
    session.filter_names = updated;
    std::cout << c("Filters set to: " + session.filters_label() + " (module=" + scope + ")", Colors::GREEN) << "\n";
    return true;
}
