#include "interface/cli_parser.h"

#include "utils/strings.h"

#include <sstream>

namespace silicore::interface {

namespace {

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        out.push_back(token);
    }
    return out;
}

CliArgs parse_tokens(const std::vector<std::string>& tokens) {
    CliArgs args;
    if (tokens.empty()) {
        args.prompt_mode = true;
        return args;
    }
    bool extension_control_set = false;

    auto normalize_command = [](const std::string& input) {
        std::string key = utils::to_lower(input);
        if (key == "scan" || key == "persona" || key == "social") return std::string("profile");
        if (key == "domain" || key == "asset") return std::string("surface");
        if (key == "full" || key == "combo") return std::string("fusion");
        if (key == "orch") return std::string("orchestrate");
        if (key == "qtest" || key == "smoke") return std::string("quicktest");
        return key;
    };

    auto normalize_preset = [](const std::string& input) {
        std::string key = utils::to_lower(input);
        if (key == "quick") key = "fast";
        if (key == "safe") key = "fast";
        if (key == "standard") key = "balanced";
        if (key == "aggressive") key = "max";
        return key;
    };

    auto add_list = [&](std::vector<std::string>& out, const std::string& value) {
        auto list = utils::split(value, ',');
        for (auto& item : list) {
            auto trimmed = utils::trim(item);
            if (!trimmed.empty()) {
                out.push_back(trimmed);
            }
        }
    };

    size_t i = 0;
    if (!tokens[i].empty() && tokens[i][0] != '-') {
        args.command = normalize_command(tokens[i++]);
    }

    for (; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token == "--preset" && i + 1 < tokens.size()) {
            args.preset = normalize_preset(tokens[++i]);
        } else if (token == "--profile" && i + 1 < tokens.size()) {
            args.profile_preset = normalize_preset(tokens[++i]);
        } else if (token == "--profile-preset" && i + 1 < tokens.size()) {
            args.profile_preset = normalize_preset(tokens[++i]);
        } else if (token == "--surface-preset" && i + 1 < tokens.size()) {
            args.surface_preset = normalize_preset(tokens[++i]);
        } else if (token == "--extension-control" && i + 1 < tokens.size()) {
            args.extension_control = utils::to_lower(tokens[++i]);
            extension_control_set = true;
        } else if (token == "--about") {
            args.command = "about";
        } else if (token == "--explain") {
            args.command = "explain";
        } else if (token == "--timeout" && i + 1 < tokens.size()) {
            auto seconds = std::stod(tokens[++i]);
            if (seconds > 0) {
                args.timeout_ms = static_cast<int>(seconds * 1000.0);
            }
        } else if ((token == "--concurrency" || token == "--max-concurrency" || token == "--max-workers") && i + 1 < tokens.size()) {
            args.concurrency = std::stoi(tokens[++i]);
        } else if (token == "--proxy" && i + 1 < tokens.size()) {
            args.proxy_url = tokens[++i];
        } else if (token == "--no-proxy") {
            args.no_proxy = true;
        } else if (token == "--tor") {
            args.tor_enabled = true;
        } else if (token == "--no-tor") {
            args.no_tor = true;
        } else if (token == "--check") {
            args.check_only = true;
        } else if (token == "--prompt") {
            args.prompt_only = true;
        } else if (token == "--html") {
            args.html_output = true;
        } else if (token == "--no-html") {
            args.html_output = false;
        } else if (token == "--json") {
            if (args.command == "modules") {
                args.modules_json = true;
            } else {
                args.json_output = true;
            }
        } else if (token == "--txt") {
            args.text_output = true;
        } else if (token == "--csv") {
            args.csv_output = true;
        } else if (token == "--no-csv") {
            args.csv_output = false;
        } else if ((token == "--out" || token == "--output" || token == "--output-dir") && i + 1 < tokens.size()) {
            args.output_dir = tokens[++i];
        } else if ((token == "--plugins" || token == "--plugin") && i + 1 < tokens.size()) {
            add_list(args.plugins, tokens[++i]);
        } else if (token == "--all-plugins") {
            args.all_plugins = true;
        } else if (token == "--list-plugins") {
            args.list_plugins = true;
        } else if (token == "--filters" && i + 1 < tokens.size()) {
            add_list(args.filters, tokens[++i]);
        } else if (token == "--filter" && i + 1 < tokens.size()) {
            add_list(args.filters, tokens[++i]);
        } else if (token == "--all-filters") {
            args.all_filters = true;
        } else if (token == "--list-filters") {
            args.list_filters = true;
        } else if (token == "--list-modules") {
            args.list_modules = true;
        } else if (token == "--scope" && i + 1 < tokens.size()) {
            args.scope = utils::to_lower(tokens[++i]);
        } else if (token == "--kind" && i + 1 < tokens.size()) {
            args.kind = utils::to_lower(tokens[++i]);
        } else if (token == "--framework" && i + 1 < tokens.size()) {
            add_list(args.framework, tokens[++i]);
        } else if (token == "--tag" && i + 1 < tokens.size()) {
            add_list(args.tag, tokens[++i]);
        } else if (token == "--search" && i + 1 < tokens.size()) {
            args.search = tokens[++i];
        } else if (token == "--stats-only") {
            args.stats_only = true;
        } else if (token == "--min-score" && i + 1 < tokens.size()) {
            args.min_score = std::stoi(tokens[++i]);
        } else if (token == "--sort-by" && i + 1 < tokens.size()) {
            args.sort_by = utils::to_lower(tokens[++i]);
        } else if (token == "--descending") {
            args.descending = true;
        } else if (token == "--offset" && i + 1 < tokens.size()) {
            args.offset = std::stoi(tokens[++i]);
        } else if (token == "--sync") {
            args.modules_sync = true;
        } else if (token == "--validate") {
            args.modules_validate = true;
        } else if (token == "--limit" && i + 1 < tokens.size()) {
            args.limit = std::stoi(tokens[++i]);
        } else if (token == "--max-platforms" && i + 1 < tokens.size()) {
            args.max_platforms = std::stoi(tokens[++i]);
        } else if (token == "--min-confidence" && i + 1 < tokens.size()) {
            args.min_confidence = std::stod(tokens[++i]);
        } else if (token == "--secondary-target" && i + 1 < tokens.size()) {
            args.secondary_target = tokens[++i];
        } else if (token == "--source-profile" && i + 1 < tokens.size()) {
            args.source_profile = normalize_preset(tokens[++i]);
        } else if (token == "--max-subdomains" && i + 1 < tokens.size()) {
            args.max_subdomains = std::stoi(tokens[++i]);
        } else if (token == "--ct") {
            args.include_ct = true;
        } else if (token == "--no-ct") {
            args.include_ct = false;
        } else if (token == "--rdap") {
            args.include_rdap = true;
        } else if (token == "--no-rdap") {
            args.include_rdap = false;
        } else if (token == "--live") {
            args.live = true;
        } else if (token == "--port" && i + 1 < tokens.size()) {
            args.live_port = std::stoi(tokens[++i]);
        } else if (token == "--live-port" && i + 1 < tokens.size()) {
            args.live_port = std::stoi(tokens[++i]);
        } else if (token == "--no-browser") {
            args.no_browser = true;
        } else if (token == "--profile-phase") {
            args.profile_phase = true;
        } else if (token == "--no-profile-phase") {
            args.profile_phase = false;
        } else if (token == "--surface-phase") {
            args.surface_phase = true;
        } else if (token == "--no-surface-phase") {
            args.surface_phase = false;
        } else if (token == "--fusion-phase") {
            args.fusion_phase = true;
        } else if (token == "--no-fusion-phase") {
            args.fusion_phase = false;
        } else if (token == "--usernames" && i + 1 < tokens.size()) {
            add_list(args.usernames, tokens[++i]);
        } else if (token == "--domain" && i + 1 < tokens.size()) {
            args.domain = tokens[++i];
        } else if (token == "--sync-modules") {
            args.sync_modules = true;
        } else if (token == "--list-templates") {
            args.list_templates = true;
        } else if (token == "--template" && i + 1 < tokens.size()) {
            args.template_id = tokens[++i];
        } else if (token == "--seed" && i + 1 < tokens.size()) {
            args.seed = std::stoi(tokens[++i]);
            args.seed_provided = true;
        } else if (!token.empty() && token[0] != '-') {
            args.targets.push_back(token);
        }
    }

    if (args.no_tor) {
        args.tor_enabled = false;
    }
    if (args.no_proxy) {
        args.proxy_url.clear();
    }
    if (args.command == "prompt") {
        args.prompt_mode = true;
    }

    if (!extension_control_set) {
        if (args.command == "profile" || args.command == "surface" || args.command == "fusion") {
            args.extension_control = "manual";
        } else if (args.command == "orchestrate") {
            args.extension_control = "auto";
        }
    }

    return args;
}

} // namespace

CliArgs parse_args(int argc, char* argv[]) {
    std::vector<std::string> tokens;
    for (int i = 1; i < argc; ++i) {
        tokens.emplace_back(argv[i]);
    }
    if (tokens.empty()) {
        CliArgs args;
        args.prompt_mode = true;
        return args;
    }
    auto args = parse_tokens(tokens);
    if (!args.html_output && !args.json_output && !args.text_output && !args.csv_output) {
        args.text_output = true;
    }
    return args;
}

CliArgs parse_line(const std::string& line) {
    return parse_tokens(tokenize(line));
}

} // namespace silicore::interface

