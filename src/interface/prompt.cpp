#include "interface/prompt.h"

#include "extensions/control_plane.h"
#include "foundation/metadata.h"
#include "interface/banner.h"
#include "interface/cli_config.h"
#include "interface/cli_parser.h"
#include "interface/colors.h"
#include "interface/help_menu.h"
#include "interface/line_input.h"
#include "interface/symbols.h"
#include "utils/strings.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
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
    std::cout << c("Active module: " + module, Colors::GREEN) << "\n";

    if (!session.all_plugins) {
        auto resolved = resolve_names_for_scope(session.plugin_names, module, "plugin");
        session.plugin_names = resolved.selected;
        if (!resolved.rejected.empty()) {
            std::cout << c(
                "Removed incompatible plugins for module '" + module + "': " + utils::join(resolved.rejected, ", "),
                Colors::YELLOW
            ) << "\n";
        }
    }
    if (!session.all_filters) {
        auto resolved = resolve_names_for_scope(session.filter_names, module, "filter");
        session.filter_names = resolved.selected;
        if (!resolved.rejected.empty()) {
            std::cout << c(
                "Removed incompatible filters for module '" + module + "': " + utils::join(resolved.rejected, ", "),
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
            "Cannot " + action + " " + kind + " for module '" + scope + "' while extension_control=auto. "
            "Use `set extension_control manual` or `set extension_control hybrid` first.",
            Colors::RED
        ) << "\n";
        return true;
    }
    auto requested = split_selectors(value);
    if (requested.empty()) {
        std::cout << c("Provide at least one " + kind.substr(0, kind.size() - 1) + " selector (id/alias/name).", Colors::YELLOW) << "\n";
        return true;
    }

    if (kind == "plugins") {
        if (session.all_plugins) {
            std::cout << c("Cannot mutate plugins while current selection is `all`. Use `set plugins none` first.", Colors::RED) << "\n";
            return true;
        }
        auto resolved = resolve_names_for_scope(requested, scope, "plugin");
        if (!resolved.rejected.empty()) {
            std::cout << c(
                "Plugin selection blocked for module '" + scope + "'. Incompatible or unknown selectors: " +
                    utils::join(resolved.rejected, ", "),
                Colors::RED
            ) << "\n";
            std::cout << c("Use `plugins --scope ...` to inspect compatible selectors.", Colors::YELLOW) << "\n";
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
        std::cout << c("Cannot mutate filters while current selection is `all`. Use `set filters none` first.", Colors::RED) << "\n";
        return true;
    }
    auto resolved = resolve_names_for_scope(requested, scope, "filter");
    if (!resolved.rejected.empty()) {
        std::cout << c(
            "Filter selection blocked for module '" + scope + "'. Incompatible or unknown selectors: " +
                utils::join(resolved.rejected, ", "),
            Colors::RED
        ) << "\n";
        std::cout << c("Use `filters --scope ...` to inspect compatible selectors.", Colors::YELLOW) << "\n";
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
bool handle_prompt_set_command(const std::string& command_text, PromptState& session) {
    std::istringstream iss(command_text);
    std::string verb;
    std::string key;
    if (!(iss >> verb >> key)) {
        std::cout << c(
            "Usage: set <plugins|filters|profile_preset|surface_preset|extension_control|orchestrate_extension_control> <value>",
            Colors::YELLOW
        ) << "\n";
        return true;
    }
    std::string value;
    std::getline(iss, value);
    value = utils::trim(value);
    if (value.empty()) {
        std::cout << c("Provide a value.", Colors::YELLOW) << "\n";
        return true;
    }

    auto normalized_key = utils::to_lower(key);
    utils::replace_all(normalized_key, "-", "_");
    if (normalized_key == "ext" || normalized_key == "extension" || normalized_key == "control") {
        normalized_key = "extension_control";
    }
    if (normalized_key == "orchestrate_control") {
        normalized_key = "orchestrate_extension_control";
    }

    auto scope = normalize_module(session.module);

    if (normalized_key == "plugins") {
        if (session.extension_control_for_module(scope) == "auto") {
            std::cout << c(
                "Cannot set plugins for module '" + scope + "' while extension_control=auto. "
                "Use `set extension_control manual` or `set extension_control hybrid` first.",
                Colors::RED
            ) << "\n";
            return true;
        }
        auto lowered = utils::to_lower(value);
        if (lowered == "all") {
            auto errors = validate_extension_combo(session, scope, {}, session.filter_names, true, session.all_filters);
            if (!errors.empty()) {
                for (const auto& error : errors) {
                    std::cout << c("Plugin selection blocked: " + error, Colors::RED) << "\n";
                }
                return true;
            }
            session.all_plugins = true;
            session.plugin_names.clear();
            std::cout << c("Plugins set to: " + session.plugins_label() + " (module=" + scope + ")", Colors::GREEN) << "\n";
            return true;
        }
        if (lowered == "none" || lowered == "off") {
            session.all_plugins = false;
            session.plugin_names.clear();
            std::cout << c("Plugins set to: " + session.plugins_label() + " (module=" + scope + ")", Colors::GREEN) << "\n";
            return true;
        }
        auto requested = split_selectors(value);
        if (requested.empty()) {
            std::cout << c("Provide at least one plugin selector (id/alias/name).", Colors::YELLOW) << "\n";
            return true;
        }
        auto resolved = resolve_names_for_scope(requested, scope, "plugin");
        if (!resolved.rejected.empty()) {
            std::cout << c(
                "Plugin selection blocked for module '" + scope + "'. Incompatible or unknown selectors: " +
                    utils::join(resolved.rejected, ", "),
                Colors::RED
            ) << "\n";
            std::cout << c("Use `plugins --scope ...` to inspect compatible selectors.", Colors::YELLOW) << "\n";
            return true;
        }
        if (resolved.selected.empty()) {
            std::cout << c("No compatible plugins selected for module '" + scope + "'.", Colors::RED) << "\n";
            return true;
        }
        auto errors = validate_extension_combo(session, scope, resolved.selected, session.filter_names, false, session.all_filters);
        if (!errors.empty()) {
            for (const auto& error : errors) {
                std::cout << c("Plugin selection blocked: " + error, Colors::RED) << "\n";
            }
            return true;
        }
        session.all_plugins = false;
        session.plugin_names = resolved.selected;
        std::cout << c("Plugins set to: " + session.plugins_label() + " (module=" + scope + ")", Colors::GREEN) << "\n";
        return true;
    }

    if (normalized_key == "filters") {
        if (session.extension_control_for_module(scope) == "auto") {
            std::cout << c(
                "Cannot set filters for module '" + scope + "' while extension_control=auto. "
                "Use `set extension_control manual` or `set extension_control hybrid` first.",
                Colors::RED
            ) << "\n";
            return true;
        }
        auto lowered = utils::to_lower(value);
        if (lowered == "all") {
            auto errors = validate_extension_combo(session, scope, session.plugin_names, {}, session.all_plugins, true);
            if (!errors.empty()) {
                for (const auto& error : errors) {
                    std::cout << c("Filter selection blocked: " + error, Colors::RED) << "\n";
                }
                return true;
            }
            session.all_filters = true;
            session.filter_names.clear();
            std::cout << c("Filters set to: " + session.filters_label() + " (module=" + scope + ")", Colors::GREEN) << "\n";
            return true;
        }
        if (lowered == "none" || lowered == "off") {
            session.all_filters = false;
            session.filter_names.clear();
            std::cout << c("Filters set to: " + session.filters_label() + " (module=" + scope + ")", Colors::GREEN) << "\n";
            return true;
        }
        auto requested = split_selectors(value);
        if (requested.empty()) {
            std::cout << c("Provide at least one filter selector (id/alias/name).", Colors::YELLOW) << "\n";
            return true;
        }
        auto resolved = resolve_names_for_scope(requested, scope, "filter");
        if (!resolved.rejected.empty()) {
            std::cout << c(
                "Filter selection blocked for module '" + scope + "'. Incompatible or unknown selectors: " +
                    utils::join(resolved.rejected, ", "),
                Colors::RED
            ) << "\n";
            std::cout << c("Use `filters --scope ...` to inspect compatible selectors.", Colors::YELLOW) << "\n";
            return true;
        }
        if (resolved.selected.empty()) {
            std::cout << c("No compatible filters selected for module '" + scope + "'.", Colors::RED) << "\n";
            return true;
        }
        auto errors = validate_extension_combo(session, scope, session.plugin_names, resolved.selected, session.all_plugins, false);
        if (!errors.empty()) {
            for (const auto& error : errors) {
                std::cout << c("Filter selection blocked: " + error, Colors::RED) << "\n";
            }
            return true;
        }
        session.all_filters = false;
        session.filter_names = resolved.selected;
        std::cout << c("Filters set to: " + session.filters_label() + " (module=" + scope + ")", Colors::GREEN) << "\n";
        return true;
    }

    if (normalized_key == "profile_preset") {
        auto preset = utils::to_lower(value);
        if (!profile_presets().count(preset)) {
            std::cout << c("Invalid profile preset: " + value, Colors::RED) << "\n";
            return true;
        }
        session.profile_preset = preset;
        std::cout << c("Profile preset set to: " + preset, Colors::GREEN) << "\n";
        return true;
    }

    if (normalized_key == "surface_preset") {
        auto preset = utils::to_lower(value);
        if (!surface_presets().count(preset)) {
            std::cout << c("Invalid surface preset: " + value, Colors::RED) << "\n";
            return true;
        }
        session.surface_preset = preset;
        std::cout << c("Surface preset set to: " + preset, Colors::GREEN) << "\n";
        return true;
    }

    if (normalized_key == "extension_control") {
        auto mode = utils::to_lower(value);
        if (!is_valid_extension_control(mode)) {
            std::cout << c("Invalid extension control mode: " + value, Colors::RED) << "\n";
            return true;
        }
        if (mode == "auto" && (session.all_plugins || session.all_filters || !session.plugin_names.empty() || !session.filter_names.empty())) {
            std::cout << c(
                "Cannot set extension_control=auto for module '" + scope + "' while plugins/filters are configured. "
                "Reset them first with `set plugins none` and `set filters none`.",
                Colors::RED
            ) << "\n";
            return true;
        }
        session.set_extension_control_for_module(scope, mode);
        std::cout << c("Extension control set to: " + mode + " (module=" + scope + ")", Colors::GREEN) << "\n";
        return true;
    }

    if (normalized_key == "orchestrate_extension_control") {
        auto mode = utils::to_lower(value);
        if (!is_valid_extension_control(mode)) {
            std::cout << c("Invalid orchestrate extension control mode: " + value, Colors::RED) << "\n";
            return true;
        }
        if (mode == "auto" && (session.all_plugins || session.all_filters || !session.plugin_names.empty() || !session.filter_names.empty())) {
            std::cout << c(
                "Cannot set orchestrate_extension_control=auto while plugins/filters are configured. "
                "Reset them first with `set plugins none` and `set filters none`.",
                Colors::RED
            ) << "\n";
            return true;
        }
        session.orchestrate_extension_control = mode;
        std::cout << c("Orchestrate extension control set to: " + mode, Colors::GREEN) << "\n";
        return true;
    }

    std::cout << c("Unknown set key: " + normalized_key, Colors::YELLOW) << "\n";
    return true;
}

bool handle_prompt_control_command(const std::string& command_text, PromptState& session) {
    std::istringstream iss(command_text);
    std::string verb;
    std::string target;
    if (!(iss >> verb >> target)) {
        return false;
    }
    std::string value;
    std::getline(iss, value);
    value = utils::trim(value);
    auto action = utils::to_lower(verb);
    auto normalized_target = utils::to_lower(target);
    utils::replace_all(normalized_target, "-", "_");

    if (action != "select" && action != "add" && action != "remove") {
        return false;
    }
    if (value.empty()) {
        std::cout << c("Usage: select|add|remove <module|plugins|filters> <value>", Colors::YELLOW) << "\n";
        return true;
    }

    if (normalized_target == "module" || normalized_target == "mode") {
        if (action != "select") {
            std::cout << c("Only `select module <profile|surface|fusion>` is supported for module controls.", Colors::YELLOW) << "\n";
            return true;
        }
        return handle_prompt_use_command("use " + value, session);
    }

    if (normalized_target == "plugins" || normalized_target == "plugin") {
        if (action == "select") {
            return handle_prompt_set_command("set plugins " + value, session);
        }
        return mutate_selection(session, normalize_module(session.module), "plugins", action, value);
    }

    if (normalized_target == "filters" || normalized_target == "filter") {
        if (action == "select") {
            return handle_prompt_set_command("set filters " + value, session);
        }
        return mutate_selection(session, normalize_module(session.module), "filters", action, value);
    }

    std::cout << c("Unknown control target: " + normalized_target, Colors::YELLOW) << "\n";
    return true;
}
std::string anonymity_status(const PromptState& session) {
    if (session.use_tor && session.use_proxy) {
        return "Tor + Proxy";
    }
    if (session.use_tor) {
        return "Tor only";
    }
    if (session.use_proxy) {
        return "Proxy only";
    }
    return "No anonymization";
}

void print_prompt_config(const PromptState& session) {
    std::cout << c("\n" + std::string(symbol("major")) + " Prompt Configuration", Colors::BLUE) << "\n";
    std::cout << c(std::string(36, '-'), Colors::BLUE) << "\n";
    std::cout << c("prompt: " + session.module_prompt(), Colors::CYAN) << "\n";
    std::cout << c("module: " + session.module, Colors::CYAN) << "\n";
    std::cout << c("plugins: " + session.plugins_label(), Colors::CYAN) << "\n";
    std::cout << c("filters: " + session.filters_label(), Colors::CYAN) << "\n";
    std::cout << c("profile preset: " + session.profile_preset, Colors::CYAN) << "\n";
    std::cout << c("surface preset: " + session.surface_preset, Colors::CYAN) << "\n";
    std::cout << c("profile extension control: " + session.profile_extension_control, Colors::CYAN) << "\n";
    std::cout << c("surface extension control: " + session.surface_extension_control, Colors::CYAN) << "\n";
    std::cout << c("fusion extension control: " + session.fusion_extension_control, Colors::CYAN) << "\n";
    std::cout << c("orchestrate extension control: " + session.orchestrate_extension_control, Colors::CYAN) << "\n";
    std::cout << c("anonymity: " + anonymity_status(session), Colors::CYAN) << "\n";
    std::cout << "\n";
}

void print_prompt_help() {
    show_prompt_help();
}

} // namespace
int run_prompt(const CommandHandler& handler) {
    PromptState session;
    while (true) {
        std::string prompt = c(session.module_prompt(), Colors::CYAN);
        std::string line = read_line(prompt);
        if (line.empty() && last_read_eof()) {
            break;
        }
        auto trimmed = utils::trim(line);
        if (trimmed.empty()) {
            continue;
        }
        add_history(trimmed);

        std::vector<std::string> raw_tokens;
        if (!tokenize_line(trimmed, raw_tokens)) {
            std::cout << c(std::string(symbol("tip")) + " Invalid command. Use `help` command.", Colors::YELLOW) << "\n";
            continue;
        }
        if (raw_tokens.empty()) {
            continue;
        }

        auto first_token = utils::to_lower(raw_tokens.front());
        if (first_token == "show") {
            if (raw_tokens.size() < 2) {
                std::cout << c(std::string(symbol("tip")) + " Invalid command. Use `help` command.", Colors::YELLOW) << "\n";
                continue;
            }
            auto show_target = keyword_to_command(raw_tokens[1]);
            if (show_target.empty()) {
                show_target = utils::to_lower(raw_tokens[1]);
            }
            if (!kPromptShowCommands.count(show_target)) {
                std::cout << c(std::string(symbol("tip")) + " Invalid command. Use `help` command.", Colors::YELLOW) << "\n";
                continue;
            }
            std::vector<std::string> rewritten;
            rewritten.push_back(show_target);
            if (raw_tokens.size() > 2) {
                rewritten.insert(rewritten.end(), raw_tokens.begin() + 2, raw_tokens.end());
            }
            raw_tokens.swap(rewritten);
        }

        auto lowered = utils::to_lower(utils::join(raw_tokens, " "));
        auto keyword_match = keyword_to_command(lowered);

        const auto& keywords = prompt_keywords();
        auto exit_it = keywords.find("exit");
        if (exit_it != keywords.end()) {
            for (const auto& word : exit_it->second) {
                if (utils::to_lower(word) == lowered) {
                    std::cout << c("\nExiting Silicore-C.", Colors::RED) << "\n";
                    return 0;
                }
            }
        }
        auto help_it = keywords.find("help");
        if (help_it != keywords.end()) {
            for (const auto& word : help_it->second) {
                if (utils::to_lower(word) == lowered) {
                    print_prompt_help();
                    goto next_loop;
                }
            }
        }
        if (lowered == "clear") {
            std::cout << "\x1B[2J\x1B[H";
            goto next_loop;
        }
        if (keyword_match == "banner" || lowered == "banner") {
            std::cout << "\x1B[2J\x1B[H";
            show_banner(anonymity_status(session));
            goto next_loop;
        }
        if (lowered == "version") {
            std::cout << c(foundation::framework_signature(), Colors::CYAN) << "\n";
            goto next_loop;
        }
        if (keyword_match == "config" || lowered == "config") {
            print_prompt_config(session);
            goto next_loop;
        }
        if (keyword_match == "keywords") {
            CliArgs args;
            args.command = "keywords";
            handler(args);
            goto next_loop;
        }
        if (keyword_match == "plugins") {
            CliArgs args;
            args.command = "plugins";
            args.scope = "all";
            handler(args);
            goto next_loop;
        }
        if (keyword_match == "filters") {
            CliArgs args;
            args.command = "filters";
            args.scope = "all";
            handler(args);
            goto next_loop;
        }
        if (keyword_match == "modules") {
            CliArgs args;
            args.command = "modules";
            args.scope = "all";
            args.kind = "all";
            args.limit = 25;
            handler(args);
            goto next_loop;
        }
        if (keyword_match == "history") {
            CliArgs args;
            args.command = "history";
            args.limit = 25;
            handler(args);
            goto next_loop;
        }
        if (keyword_match == "about" || lowered == "about") {
            CliArgs args;
            args.command = "about";
            handler(args);
            goto next_loop;
        }
        if (keyword_match == "explain" || lowered == "explain") {
            CliArgs args;
            args.command = "explain";
            handler(args);
            goto next_loop;
        }
        if (lowered.rfind("set ", 0) == 0) {
            handle_prompt_set_command(trimmed, session);
            goto next_loop;
        }
        if (lowered.rfind("use ", 0) == 0) {
            handle_prompt_use_command(trimmed, session);
            goto next_loop;
        }
        if (lowered.rfind("select ", 0) == 0 || lowered.rfind("add ", 0) == 0 || lowered.rfind("remove ", 0) == 0) {
            handle_prompt_control_command(trimmed, session);
            goto next_loop;
        }

        {
            auto tokens = rewrite_tokens_with_keywords(raw_tokens);
            if (!tokens.empty() && tokens[0] == "profile" && tokens.size() == 1) {
                auto target = utils::trim(read_line("Username target: "));
                tokens.push_back(target);
            }
            if (!tokens.empty() && tokens[0] == "surface" && tokens.size() == 1) {
                auto target = utils::trim(read_line("Domain target: "));
                tokens.push_back(target);
            }
            if (!tokens.empty() && tokens[0] == "fusion" && tokens.size() == 1) {
                auto username = utils::trim(read_line("Username target: "));
                auto domain = utils::trim(read_line("Domain target: "));
                tokens.push_back(username);
                tokens.push_back(domain);
            }
            if (!tokens.empty() && tokens[0] == "scan" && tokens.size() == 1) {
                auto target = utils::trim(read_line("Username target: "));
                tokens.push_back(target);
            }
            if (!tokens.empty() && tokens[0] == "orchestrate" && tokens.size() == 1) {
                auto mode = utils::trim(read_line("Orchestration mode [profile|surface|fusion] [profile]: "));
                if (mode.empty()) {
                    mode = "profile";
                }
                auto primary = utils::trim(read_line("Primary target: "));
                tokens.push_back(mode);
                tokens.push_back(primary);
                if (utils::to_lower(mode) == "fusion") {
                    auto secondary = utils::trim(read_line("Secondary domain target: "));
                    if (!secondary.empty()) {
                        tokens.push_back("--secondary-target");
                        tokens.push_back(secondary);
                    }
                }
            }
            if (!keyword_match.empty() && tokens.size() == 1) {
                if (keyword_match == "profile") {
                    auto target = utils::trim(read_line("Username target: "));
                    tokens.push_back(target);
                } else if (keyword_match == "surface") {
                    auto target = utils::trim(read_line("Domain target: "));
                    tokens.push_back(target);
                } else if (keyword_match == "fusion") {
                    auto username = utils::trim(read_line("Username target: "));
                    auto domain = utils::trim(read_line("Domain target: "));
                    tokens.push_back(username);
                    tokens.push_back(domain);
                } else if (keyword_match == "orchestrate") {
                    auto mode = utils::trim(read_line("Orchestration mode [profile|surface|fusion] [profile]: "));
                    if (mode.empty()) {
                        mode = "profile";
                    }
                    auto primary = utils::trim(read_line("Primary target: "));
                    tokens.push_back(mode);
                    tokens.push_back(primary);
                    if (utils::to_lower(mode) == "fusion") {
                        auto secondary = utils::trim(read_line("Secondary domain target: "));
                        if (!secondary.empty()) {
                            tokens.push_back("--secondary-target");
                            tokens.push_back(secondary);
                        }
                    }
                }
            }
            if (tokens.size() == 2 && tokens[0] == "scan") {
                tokens[0] = "profile";
            }

            auto explicit_flags = extract_explicit_flags(tokens);
            CliArgs args = parse_tokens(tokens);
            args.prompt_mode = true;
            apply_prompt_defaults(args, session, explicit_flags);

            bool tor_explicit = explicit_flags.count("--tor") || explicit_flags.count("--no-tor");
            bool proxy_explicit = explicit_flags.count("--proxy") || explicit_flags.count("--no-proxy");
            if (!tor_explicit) {
                args.tor_enabled = session.use_tor;
            }
            if (!proxy_explicit) {
                args.proxy_enabled = session.use_proxy;
            }

            if (args.command == "anonymity") {
                bool has_flags = tor_explicit || proxy_explicit || args.check_only;
                if (args.prompt_only || !has_flags) {
                    std::string reply = read_line("Enable Tor routing? (y/N): ");
                    bool allow_tor = !reply.empty() && (reply[0] == 'y' || reply[0] == 'Y');
                    std::string proxy_reply = read_line("Enable proxy routing? (y/N): ");
                    bool allow_proxy = !proxy_reply.empty() && (proxy_reply[0] == 'y' || proxy_reply[0] == 'Y');
                    args.tor_enabled = allow_tor;
                    args.proxy_enabled = allow_proxy;
                    args.no_tor = !allow_tor;
                    args.no_proxy = !allow_proxy;
                    if (!args.check_only) {
                        session.use_tor = allow_tor;
                        session.use_proxy = allow_proxy;
                    }
                } else if (!args.check_only) {
                    if (tor_explicit) {
                        session.use_tor = args.tor_enabled && !args.no_tor;
                    }
                    if (proxy_explicit) {
                        session.use_proxy = args.proxy_enabled && !args.no_proxy;
                    }
                }
            }

            handler(args);
            session.history.push_back(trimmed);
            if (session.history.size() > 200) {
                session.history.erase(session.history.begin(), session.history.end() - 200);
            }
        }
next_loop:
        continue;
    }
    return 0;
}

} // namespace silicore::interface
