#include "runner.h"

#include "collect/platform_schema.h"
#include "collect/anonymity.h"
#include "analyze/correlator.h"
#include "analyze/exposure.h"
#include "analyze/narrative.h"
#include "extensions/plugin_loader.h"
#include "extensions/filter_loader.h"
#include "interface/banner.h"
#include "interface/cli_parser.h"
#include "interface/prompt.h"
#include "interface/colors.h"
#include "interface/symbols.h"
#include "foundation/metadata.h"
#include "execution_policy.h"
#include "orchestrator.h"
#include "engines/fusion_engine.h"
#include "reporting/reporting.h"
#include "utils/strings.h"
#include "modules/catalog.h"
#include "interface/line_input.h"
#include "interface/live_server.h"

#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <algorithm>
#include <sstream>

namespace silicore {

namespace interface {
void show_about();
void show_explain();
}

namespace {

std::string resolve_proxy(const interface::CliArgs& args) {
    if (args.tor_enabled) {
        if (!collect::is_tor_running()) {
            std::cout << interface::c(std::string(interface::symbol("warn")) + " Tor not running.", interface::Colors::RED) << "\n";
            std::string reply = interface::read_line(
                interface::c("Start and configure Tor now? (y/N): ", interface::Colors::GREY)
            );
            bool allow = !reply.empty() && (reply[0] == 'y' || reply[0] == 'Y');
            if (!collect::ensure_tor_running(allow)) {
                std::cout << interface::c(std::string(interface::symbol("warn")) + " Tor unavailable. Continuing without Tor proxy.", interface::Colors::RED) << "\n";
                return "";
            }
        }
        return collect::tor_proxy_url();
    }
    if (!args.proxy_url.empty()) {
        return args.proxy_url;
    }
    return "";
}

std::filesystem::path resolve_plugin_dir() {
    if (const char* env = std::getenv("SILICORE_PLUGIN_DIR")) {
        return std::filesystem::path(env);
    }
    auto has_shared = [](const std::filesystem::path& dir) {
        if (!std::filesystem::is_directory(dir)) {
            return false;
        }
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".so") {
                return true;
            }
        }
        return false;
    };

    if (has_shared("plugins")) {
        return std::filesystem::path("plugins");
    }
    if (has_shared("build/plugins")) {
        return std::filesystem::path("build/plugins");
    }
    return std::filesystem::path("plugins");
}

std::filesystem::path resolve_filter_dir() {
    if (const char* env = std::getenv("SILICORE_FILTER_DIR")) {
        return std::filesystem::path(env);
    }
    auto has_shared = [](const std::filesystem::path& dir) {
        if (!std::filesystem::is_directory(dir)) {
            return false;
        }
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".so") {
                return true;
            }
        }
        return false;
    };

    if (has_shared("filters")) {
        return std::filesystem::path("filters");
    }
    if (has_shared("build/filters")) {
        return std::filesystem::path("build/filters");
    }
    return std::filesystem::path("filters");
}
std::filesystem::path resolve_output_root(const interface::CliArgs& args) {
    if (!args.output_dir.empty()) {
        return std::filesystem::path(args.output_dir);
    }
    return std::filesystem::current_path();
}

constexpr int kDefaultLivePort = 7331;

interface::LiveServer& live_server_instance() {
    static interface::LiveServer server;
    return server;
}

std::filesystem::path resolve_live_root(const interface::CliArgs& args) {
    return resolve_output_root(args) / "output" / "html";
}

struct OutputPaths {
    std::filesystem::path cli_path;
    std::filesystem::path html_path;
    std::filesystem::path json_path;
    std::filesystem::path csv_path;
};

OutputPaths resolve_output_paths(const interface::CliArgs& args, const std::string& target_key) {
    auto root = resolve_output_root(args) / "output";
    OutputPaths paths;
    paths.cli_path = root / "cli" / (target_key + ".txt");
    paths.html_path = root / "html" / (target_key + ".html");
    paths.json_path = root / "data" / target_key / "results.json";
    paths.csv_path = root / "cli" / (target_key + ".csv");
    return paths;
}

bool ensure_live_server(const interface::CliArgs& args) {
    auto root = resolve_live_root(args);
    auto& server = live_server_instance();
    int desired_port = args.live_port > 0 ? args.live_port : kDefaultLivePort;
    bool needs_start = !server.running() || server.root() != root;
    if (!needs_start && args.live_port > 0 && server.port() != desired_port) {
        needs_start = true;
    }
    if (!needs_start) {
        return true;
    }
    server.stop();
    try {
        std::filesystem::create_directories(root);
    } catch (const std::exception& exc) {
        std::cerr << interface::c(std::string(interface::symbol("warn")) + " Live server root error: " + exc.what(), interface::Colors::RED) << "\n";
        return false;
    }
    bool ok = server.start(root, desired_port);
    if (!ok && args.live_port <= 0) {
        ok = server.start(root, 0);
    }
    if (!ok) {
        std::cerr << interface::c(std::string(interface::symbol("warn")) + " Live server failed to start.", interface::Colors::RED) << "\n";
        return false;
    }
    std::cout << interface::c(std::string(interface::symbol("feature")) + " Live server: " + server.base_url(), interface::Colors::CYAN) << "\n";
    return true;
}

struct QuicktestTemplate {
    std::string id;
    std::string title;
    std::string description;
    std::string username;
    std::string domain;
    std::string mode;
};

const std::vector<QuicktestTemplate>& quicktest_templates() {
    static const std::vector<QuicktestTemplate> templates = {
        {"smoke", "Smoke Test", "Profile + surface sanity scan.", "silicore", "example.com", "smoke"},
        {"profile", "Profile Quicktest", "Profile-only workflow.", "silicore", "", "profile"},
        {"surface", "Surface Quicktest", "Surface-only workflow.", "", "example.com", "surface"},
        {"fusion", "Fusion Quicktest", "Fusion workflow.", "silicore", "example.com", "fusion"},
    };
    return templates;
}

const QuicktestTemplate* find_quicktest_template(const std::string& id) {
    for (const auto& entry : quicktest_templates()) {
        if (utils::to_lower(entry.id) == utils::to_lower(id)) {
            return &entry;
        }
    }
    return nullptr;
}

void show_quicktest_templates() {
    std::cout << interface::c(std::string(interface::symbol("feature")) + " Quicktest templates:", interface::Colors::CYAN) << "\n";
    for (const auto& entry : quicktest_templates()) {
        std::cout << interface::c("- " + entry.id, interface::Colors::SKY)
                  << interface::c(" (" + entry.title + ")", interface::Colors::GREY)
                  << interface::c(" - " + entry.description, interface::Colors::GREY)
                  << "\n";
    }
}

std::vector<std::string> parse_target_list(const std::string& input) {
    std::vector<std::string> out;
    if (input.find(',') != std::string::npos) {
        auto parts = utils::split(input, ',');
        for (auto& part : parts) {
            auto trimmed = utils::trim(part);
            if (!trimmed.empty()) {
                out.push_back(trimmed);
            }
        }
        return out;
    }
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        auto trimmed = utils::trim(token);
        if (!trimmed.empty()) {
            out.push_back(trimmed);
        }
    }
    return out;
}

std::string normalize_command_alias(const std::string& input) {
    std::string key = utils::to_lower(input);
    if (key == "scan" || key == "persona" || key == "social") return "profile";
    if (key == "domain" || key == "asset") return "surface";
    if (key == "full" || key == "combo") return "fusion";
    if (key == "orch") return "orchestrate";
    if (key == "qtest" || key == "smoke") return "quicktest";
    return key;
}

bool extensions_enabled(const interface::CliArgs& args, const std::string& kind) {
    if (args.extension_control.empty()) {
        return true;
    }
    std::string mode = utils::to_lower(args.extension_control);
    if (mode == "off" || mode == "none" || mode == "disabled") {
        return false;
    }
    if (mode == "plugins-only") {
        return kind == "plugin";
    }
    if (mode == "filters-only") {
        return kind == "filter";
    }
    return true;
}

void ensure_output_settings(interface::CliArgs& args) {
    if (!args.html_output && !args.json_output && !args.text_output && !args.csv_output) {
        std::string prompt = interface::c(std::string(interface::symbol("action")) + " Output formats (txt, html, json, csv) [txt]: ", interface::Colors::CYAN);
        std::string input = interface::read_line(prompt);
        auto trimmed = utils::trim(input);
        if (trimmed.empty()) {
            args.text_output = true;
        } else {
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
        }
    }

    if (args.output_dir.empty()) {
        std::string prompt = interface::c(std::string(interface::symbol("action")) + " Output directory [cwd]: ", interface::Colors::CYAN);
        std::string input = interface::read_line(prompt);
        auto trimmed = utils::trim(input);
        if (!trimmed.empty()) {
            args.output_dir = trimmed;
        }
    }
}

std::string quote_arg(const std::string& value) {
    std::string out = "\"";
    out.reserve(value.size() + 2);
    for (char ch : value) {
        if (ch == '"') {
            out += "\\\"";
        } else {
            out.push_back(ch);
        }
    }
    out += "\"";
    return out;
}

void open_in_browser(const std::string& location) {
    std::string quoted = quote_arg(location);
#if defined(_WIN32)
    std::string cmd = "cmd /c start \"\" " + quoted;
#elif defined(__APPLE__)
    std::string cmd = "open " + quoted;
#else
    std::string cmd = "xdg-open " + quoted;
#endif
    std::system(cmd.c_str());
}

std::string live_relative_path_for(const std::string& target) {
    return reporting::sanitize_target(target) + ".html";
}

void open_live_target(const interface::CliArgs& args, const std::string& target, bool use_server) {
    std::string key = reporting::sanitize_target(target);
    auto paths = resolve_output_paths(args, key);
    bool html_exists = std::filesystem::exists(paths.html_path);

    if (!use_server) {
        if (!html_exists) {
            std::cerr << interface::c(std::string(interface::symbol("warn")) + " HTML report not found for " + target, interface::Colors::RED) << "\n";
            return;
        }
        if (args.no_browser) {
            std::cout << "Live report: " << paths.html_path.string() << "\n";
            return;
        }
        open_in_browser(paths.html_path.string());
        return;
    }

    if (!ensure_live_server(args)) {
        if (html_exists) {
            if (args.no_browser) {
                std::cout << "Live report: " << paths.html_path.string() << "\n";
            } else {
                open_in_browser(paths.html_path.string());
            }
        }
        return;
    }
    if (!html_exists) {
        std::cerr << interface::c(std::string(interface::symbol("warn")) + " HTML report not found for " + target, interface::Colors::RED) << "\n";
    }
    auto url = live_server_instance().url_for(live_relative_path_for(target));
    if (args.no_browser) {
        std::cout << "Live report: " << url << "\n";
        return;
    }
    open_in_browser(url);
}

void maybe_open_live(const interface::CliArgs& args, const std::string& target) {
    if (!args.live) {
        return;
    }
    open_live_target(args, target, true);
}

void maybe_wait_for_live(const interface::CliArgs& args) {
    if (!args.live || args.prompt_mode) {
        return;
    }
    auto& server = live_server_instance();
    if (!server.running()) {
        return;
    }
    std::cout << interface::c(std::string(interface::symbol("action")) + " Live server running at " + server.base_url(), interface::Colors::CYAN) << "\n";
    interface::read_line(interface::c("Press Enter to stop the live server... ", interface::Colors::GREY));
    server.stop();
}

void print_help() {
    using namespace interface;
    std::cout << c(std::string(symbol("major")) + " " + foundation::PROJECT_NAME + " v" + foundation::VERSION, Colors::SKY_DARK) << "\n";
    std::cout << c(std::string(symbol("action")) + " Commands:", Colors::CYAN) << "\n";
    std::cout << c("  profile <username...> [--preset fast|balanced|deep|max] [--timeout ms] [--concurrency n] [--plugin a,b] [--filter a,b] [--tor]", Colors::GREY) << "\n";
    std::cout << c("  surface <domain...> [--preset fast|balanced|deep|max] [--ct|--no-ct] [--rdap|--no-rdap] [--max-subdomains n] [--tor]", Colors::GREY) << "\n";
    std::cout << c("  fusion <username> <domain> [--profile-preset fast|balanced|deep|max] [--surface-preset fast|balanced|deep|max] [--plugin a,b] [--filter a,b]", Colors::GREY) << "\n";
    std::cout << c("  orchestrate <profile|surface|fusion> <target> [flags]", Colors::GREY) << "\n";
    std::cout << c("  quicktest [--template id | --list-templates]", Colors::GREY) << "\n";
    std::cout << c("  plugins | filters | modules | history | keywords", Colors::GREY) << "\n";
    std::cout << c("  anonymity [--tor|--no-tor] [--proxy url] [--check|--prompt]", Colors::GREY) << "\n";
    std::cout << c("  wizard [--profile-phase|--surface-phase|--fusion-phase]", Colors::GREY) << "\n";
    std::cout << c("  live <target> [--no-browser] [--live-port n]", Colors::GREY) << "\n";
    std::cout << c("  prompt", Colors::GREY) << "\n";
    std::cout << c("  about | --about", Colors::GREY) << "\n";
    std::cout << c("  explain | --explain", Colors::GREY) << "\n";
    std::cout << c("  show plugins | filters | platforms | modules", Colors::GREY) << "\n";
    std::cout << c("  help", Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("feature")) + " Flags:", Colors::CYAN) << "\n";
    std::cout << c("  --preset fast|balanced|deep|max --profile-preset fast|balanced|deep|max --surface-preset fast|balanced|deep|max", Colors::GREY) << "\n";
    std::cout << c("  --timeout <ms> --concurrency <n> --proxy <url> --tor --no-tor --no-proxy", Colors::GREY) << "\n";
    std::cout << c("  --html --json --txt --csv --out <dir>", Colors::GREY) << "\n";
    std::cout << c("  --plugins a,b --all-plugins --filters a,b --all-filters --extension-control auto|manual|hybrid", Colors::GREY) << "\n";
    std::cout << c("  --list-plugins --list-filters --list-modules --scope <id> --search <text> --stats-only --limit <n>", Colors::GREY) << "\n";
    std::cout << c("  --ct|--no-ct --rdap|--no-rdap --max-subdomains <n>", Colors::GREY) << "\n";
    std::cout << c("  --live --no-browser --live-port <n>", Colors::GREY) << "\n";
    std::cout << c("  --list-templates --template <id> --seed <n>", Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("feature")) + " Outputs:", Colors::CYAN) << "\n";
    std::cout << c("  output/data/<target>/results.json | output/html/<target>.html | output/cli/<target>.txt | output/cli/<target>.csv", Colors::GREY) << "\n";
    std::cout << c("  Default output root: ./output (override with --out <dir>)", Colors::GREY) << "\n";
}

std::vector<collect::PlatformConfig> load_platforms_safe() {
    try {
        return collect::load_platforms("platforms");
    } catch (const std::exception& exc) {
        std::cerr << interface::c(std::string(interface::symbol("warn")) + " Platform load error: " + exc.what(), interface::Colors::RED) << "\n";
        return {};
    }
}

bool scope_matches(const std::string& scopes, const std::string& scope) {
    if (scope.empty() || scope == "all") {
        return true;
    }
    auto parts = utils::split(scopes, ',');
    for (auto& part : parts) {
        if (utils::to_lower(utils::trim(part)) == scope) {
            return true;
        }
    }
    return false;
}

bool tags_match(const std::vector<std::string>& tags, const std::string& scope) {
    if (scope.empty() || scope == "all") {
        return true;
    }
    for (const auto& tag : tags) {
        if (utils::to_lower(tag) == scope) {
            return true;
        }
    }
    return false;
}

bool search_match(const std::string& value, const std::string& query) {
    if (query.empty()) {
        return true;
    }
    auto hay = utils::to_lower(value);
    auto needle = utils::to_lower(query);
    return hay.find(needle) != std::string::npos;
}

void show_plugins_inventory(const interface::CliArgs& args) {
    extensions::PluginManager manager;
    manager.load_all(resolve_plugin_dir());
    size_t count = 0;
    for (const auto& plugin : manager.plugins()) {
        auto id = plugin->spec().id ? plugin->spec().id : "";
        auto title = plugin->spec().title ? plugin->spec().title : "";
        auto scopes = plugin->spec().scopes ? plugin->spec().scopes : "";
        if (!scope_matches(scopes, args.scope)) {
            continue;
        }
        if (!search_match(id, args.search) && !search_match(title, args.search)) {
            continue;
        }
        count++;
        if (!args.stats_only) {
            std::cout << interface::c("- " + id, interface::Colors::SKY);
            if (!title.empty()) {
                std::cout << interface::c(" (" + title + ")", interface::Colors::GREY);
            }
            if (plugin->spec().version) {
                std::cout << interface::c(" v" + std::string(plugin->spec().version), interface::Colors::GREY);
            }
            if (!scopes.empty()) {
                std::cout << interface::c(" [" + scopes + "]", interface::Colors::GREY);
            }
            std::cout << "\n";
        }
    }
    std::cout << "Plugins: " << count << "\n";
}

void show_filters_inventory(const interface::CliArgs& args) {
    extensions::FilterManager manager;
    manager.load_all(resolve_filter_dir());
    size_t count = 0;
    for (const auto& filter : manager.filters()) {
        auto id = filter->spec().id ? filter->spec().id : "";
        auto title = filter->spec().title ? filter->spec().title : "";
        auto scopes = filter->spec().scopes ? filter->spec().scopes : "";
        if (!scope_matches(scopes, args.scope)) {
            continue;
        }
        if (!search_match(id, args.search) && !search_match(title, args.search)) {
            continue;
        }
        count++;
        if (!args.stats_only) {
            std::cout << interface::c("- " + id, interface::Colors::SKY);
            if (!title.empty()) {
                std::cout << interface::c(" (" + title + ")", interface::Colors::GREY);
            }
            if (filter->spec().version) {
                std::cout << interface::c(" v" + std::string(filter->spec().version), interface::Colors::GREY);
            }
            if (!scopes.empty()) {
                std::cout << interface::c(" [" + scopes + "]", interface::Colors::GREY);
            }
            std::cout << "\n";
        }
    }
    std::cout << "Filters: " << count << "\n";
}

void show_modules_inventory(const interface::CliArgs& args) {
    auto modules = modules::all_modules();
    size_t count = 0;
    for (const auto& entry : modules) {
        if (!args.kind.empty() && utils::to_lower(entry.kind) != args.kind) {
            continue;
        }
        if (!tags_match(entry.tags, args.scope)) {
            continue;
        }
        if (!search_match(entry.id, args.search) && !search_match(entry.title, args.search)) {
            continue;
        }
        count++;
        if (!args.stats_only) {
            std::cout << interface::c("- " + entry.id, interface::Colors::SKY)
                      << interface::c(" (" + entry.title + ")", interface::Colors::GREY)
                      << interface::c(" [" + entry.kind + "]", interface::Colors::GREY)
                      << "\n";
        }
    }
    std::cout << "Modules: " << count << "\n";
}

void show_history(const interface::CliArgs& args) {
    std::filesystem::path history_root = resolve_output_root(args) / "output" / "data";
    if (!std::filesystem::is_directory(history_root)) {
        std::cout << "History: no output directory found.\n";
        return;
    }
    struct Entry {
        std::filesystem::path path;
        std::filesystem::file_time_type time;
    };
    std::vector<Entry> entries;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(history_root)) {
        if (entry.is_regular_file() && entry.path().filename() == "results.json") {
            entries.push_back({entry.path(), std::filesystem::last_write_time(entry.path())});
        }
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.time > b.time;
    });
    int limit = args.limit > 0 ? args.limit : 20;
    int count = 0;
    for (const auto& entry : entries) {
        std::cout << "- " << entry.path().string() << "\n";
        if (++count >= limit) {
            break;
        }
    }
    std::cout << "History entries: " << entries.size() << "\n";
}

void show_keywords() {
    std::cout << "Keywords and aliases:\n";
    std::cout << "  profile: scan, persona, social\n";
    std::cout << "  surface: domain, asset\n";
    std::cout << "  fusion: full, combo\n";
    std::cout << "  quicktest: qtest, smoke\n";
    std::cout << "  orchestrate: orch\n";
}

std::vector<extensions::PluginResult> run_plugins(
    const interface::CliArgs& args,
    const std::string& scope,
    const reporting::json& context_json
) {
    if (!extensions_enabled(args, "plugin")) {
        return {};
    }
    if (!args.all_plugins && args.plugins.empty()) {
        return {};
    }

    extensions::PluginManager manager;
    manager.load_all(resolve_plugin_dir());
    auto context_str = context_json.dump();
    auto results = manager.run_scope(scope, context_str);

    if (!args.plugins.empty()) {
        std::vector<extensions::PluginResult> filtered;
        for (const auto& result : results) {
            for (const auto& id : args.plugins) {
                if (utils::to_lower(result.id) == utils::to_lower(id)) {
                    filtered.push_back(result);
                    break;
                }
            }
        }
        return filtered;
    }

    return results;
}

std::vector<extensions::FilterResult> run_filters(
    const interface::CliArgs& args,
    const std::string& scope,
    const reporting::json& context_json
) {
    if (!extensions_enabled(args, "filter")) {
        return {};
    }
    if (!args.all_filters && args.filters.empty()) {
        return {};
    }

    extensions::FilterManager manager;
    manager.load_all(resolve_filter_dir());
    auto context_str = context_json.dump();
    auto results = manager.run_scope(scope, context_str);

    if (!args.filters.empty()) {
        std::vector<extensions::FilterResult> filtered;
        for (const auto& result : results) {
            for (const auto& id : args.filters) {
                if (utils::to_lower(result.id) == utils::to_lower(id)) {
                    filtered.push_back(result);
                    break;
                }
            }
        }
        return filtered;
    }

    return results;
}

void write_reports(const interface::CliArgs& args, const reporting::json& payload, const std::string& key) {
    auto paths = resolve_output_paths(args, key);
    auto cli_report = reporting::render_cli_report(payload);
    std::cout << cli_report;
    if (args.text_output) {
        reporting::write_text_report(cli_report, paths.cli_path);
    }
    if (args.json_output) {
        reporting::write_json_report(payload, paths.json_path);
    }
    if (args.html_output) {
        auto html = reporting::render_html_report(payload);
        reporting::write_text_report(html, paths.html_path);
    }
    if (args.csv_output) {
        reporting::write_csv_reports(payload, paths.cli_path);
    }
}

reporting::json run_profile_flow(
    Orchestrator& orchestrator,
    const std::string& username,
    const ExecutionPolicy& policy,
    const interface::CliArgs& args,
    int timeout,
    int concurrency,
    const std::string& proxy_url
) {
    auto profile = orchestrator.run_profile(username, policy, timeout, concurrency, proxy_url);
    auto correlation = analyze::correlate(profile.scan_result.profiles);
    auto issues = analyze::assess_profile_exposure(profile.scan_result.profiles);
    auto issue_summary = analyze::summarize_issues(issues);
    auto narrative = analyze::build_nano_brief(
        username,
        profile.scan_result.profiles,
        "",
        nullptr,
        issues,
        issue_summary,
        correlation
    );

    reporting::ReportInputs context_input;
    context_input.target = username;
    context_input.profiles = profile.scan_result.profiles;
    context_input.correlation = correlation;
    context_input.issues = issues;
    context_input.issue_summary = issue_summary;
    context_input.narrative = narrative;
    context_input.mode = "profile";

    auto context_json = reporting::build_report_payload(context_input);
    auto plugins = run_plugins(args, "profile", context_json);
    auto filters = run_filters(args, "profile", context_json);
    context_input.plugins = plugins;
    context_input.filters = filters;
    return reporting::build_report_payload(context_input);
}

reporting::json run_surface_flow(
    Orchestrator& orchestrator,
    const std::string& domain,
    const ExecutionPolicy& policy,
    const interface::CliArgs& args,
    int timeout,
    const std::string& proxy_url
) {
    auto surface = orchestrator.run_surface(domain, policy, timeout, proxy_url, args.include_ct, args.include_rdap, args.max_subdomains);
    std::string target_domain = surface.scan_result.target_domain.empty() ? domain : surface.scan_result.target_domain;
    auto issues = analyze::assess_domain_exposure(
        target_domain,
        surface.scan_result.https.headers,
        surface.scan_result.http_redirects_to_https,
        static_cast<int>(surface.scan_result.subdomains.size())
    );
    auto issue_summary = analyze::summarize_issues(issues);
    analyze::CorrelationResult correlation;
    auto narrative = analyze::build_nano_brief(
        "",
        {},
        target_domain,
        &surface.scan_result,
        issues,
        issue_summary,
        correlation
    );

    reporting::ReportInputs context_input;
    context_input.target = target_domain;
    context_input.domain_result = &surface.scan_result;
    context_input.correlation = correlation;
    context_input.issues = issues;
    context_input.issue_summary = issue_summary;
    context_input.narrative = narrative;
    context_input.mode = "surface";

    auto context_json = reporting::build_report_payload(context_input);
    auto plugins = run_plugins(args, "surface", context_json);
    auto filters = run_filters(args, "surface", context_json);
    context_input.plugins = plugins;
    context_input.filters = filters;
    return reporting::build_report_payload(context_input);
}

reporting::json run_fusion_flow(
    Orchestrator& orchestrator,
    const std::string& username,
    const std::string& domain,
    const ExecutionPolicy& profile_policy,
    const ExecutionPolicy& surface_policy,
    const interface::CliArgs& args,
    int timeout_profile,
    int timeout_surface,
    int concurrency,
    const std::string& proxy_url
) {
    auto profile_future = std::async(std::launch::async, [&]() {
        return orchestrator.run_profile(username, profile_policy, timeout_profile, concurrency, proxy_url);
    });
    auto surface_future = std::async(std::launch::async, [&]() {
        return orchestrator.run_surface(domain, surface_policy, timeout_surface, proxy_url, args.include_ct, args.include_rdap, args.max_subdomains);
    });

    auto profile = profile_future.get();
    auto surface = surface_future.get();

    auto correlation = analyze::correlate(profile.scan_result.profiles);
    auto profile_issues = analyze::assess_profile_exposure(profile.scan_result.profiles);
    auto profile_issue_summary = analyze::summarize_issues(profile_issues);

    std::string target_domain = surface.scan_result.target_domain.empty() ? domain : surface.scan_result.target_domain;
    auto surface_issues = analyze::assess_domain_exposure(
        target_domain,
        surface.scan_result.https.headers,
        surface.scan_result.http_redirects_to_https,
        static_cast<int>(surface.scan_result.subdomains.size())
    );
    auto surface_issue_summary = analyze::summarize_issues(surface_issues);

    std::vector<analyze::Issue> combined_issues = profile_issues;
    combined_issues.insert(combined_issues.end(), surface_issues.begin(), surface_issues.end());
    auto combined_issue_summary = analyze::summarize_issues(combined_issues);

    auto narrative = analyze::build_nano_brief(
        username,
        profile.scan_result.profiles,
        target_domain,
        &surface.scan_result,
        combined_issues,
        combined_issue_summary,
        correlation
    );

    reporting::ReportInputs profile_input;
    profile_input.target = username;
    profile_input.profiles = profile.scan_result.profiles;
    profile_input.correlation = correlation;
    profile_input.issues = profile_issues;
    profile_input.issue_summary = profile_issue_summary;
    profile_input.mode = "profile";
    auto profile_payload = reporting::build_report_payload(profile_input);

    reporting::ReportInputs surface_input;
    surface_input.target = target_domain;
    surface_input.domain_result = &surface.scan_result;
    surface_input.issues = surface_issues;
    surface_input.issue_summary = surface_issue_summary;
    surface_input.mode = "surface";
    auto surface_payload = reporting::build_report_payload(surface_input);

    engines::FusionEngine fusion_engine;
    auto fused = fusion_engine.fuse_profile_domain(profile_payload, surface_payload);
    auto fusion_graph = fusion_engine.generate_graph(fused);
    fused["graph"] = fusion_graph;

    std::string combined_target = username + "@" + target_domain;

    reporting::ReportInputs context_input;
    context_input.target = combined_target;
    context_input.profiles = profile.scan_result.profiles;
    context_input.domain_result = &surface.scan_result;
    context_input.correlation = correlation;
    context_input.issues = combined_issues;
    context_input.issue_summary = combined_issue_summary;
    context_input.fused_intel = fused;
    context_input.fusion_graph = fusion_graph;
    context_input.narrative = narrative;
    context_input.mode = "fusion";

    auto context_json = reporting::build_report_payload(context_input);
    auto plugins = run_plugins(args, "fusion", context_json);
    auto filters = run_filters(args, "fusion", context_json);
    context_input.plugins = plugins;
    context_input.filters = filters;
    return reporting::build_report_payload(context_input);
}
int handle_command(const interface::CliArgs& args_in) {
    interface::CliArgs args = args_in;
    std::string command = utils::to_lower(args.command);

    if (args.list_templates && command.empty()) {
        show_quicktest_templates();
        return 0;
    }
    if (args.list_plugins) {
        show_plugins_inventory(args);
        return 0;
    }
    if (args.list_filters) {
        show_filters_inventory(args);
        return 0;
    }
    if (args.list_modules) {
        show_modules_inventory(args);
        return 0;
    }

    if (command.empty() || command == "help") {
        print_help();
        return 0;
    }
    if (command == "about") {
        interface::show_about();
        return 0;
    }
    if (command == "explain") {
        interface::show_explain();
        return 0;
    }

    if (command == "plugins") {
        show_plugins_inventory(args);
        return 0;
    }
    if (command == "filters") {
        show_filters_inventory(args);
        return 0;
    }
    if (command == "modules") {
        show_modules_inventory(args);
        return 0;
    }
    if (command == "history") {
        show_history(args);
        return 0;
    }
    if (command == "keywords") {
        show_keywords();
        return 0;
    }
    if (command == "platforms") {
        auto platforms = load_platforms_safe();
        std::cout << "Platforms loaded: " << platforms.size() << "\n";
        for (const auto& platform : platforms) {
            std::cout << "- " << platform.name << "\n";
        }
        return 0;
    }

    if (command == "anonymity") {
        bool tor_running = collect::is_tor_running();
        if (args.check_only) {
            std::cout << "Tor running: " << (tor_running ? "yes" : "no") << "\n";
            if (!args.proxy_url.empty()) {
                std::cout << "Proxy: " << args.proxy_url << "\n";
            }
            return 0;
        }
        interface::CliArgs anon_args = args;
        if (args.prompt_only || (!args.tor_enabled && args.proxy_url.empty() && !args.no_tor && !args.no_proxy)) {
            std::string reply = interface::read_line(
                interface::c(std::string(interface::symbol("action")) + " Enable Tor routing? (y/N): ", interface::Colors::CYAN)
            );
            bool allow_tor = !reply.empty() && (reply[0] == 'y' || reply[0] == 'Y');
            if (allow_tor) {
                anon_args.tor_enabled = true;
                anon_args.no_tor = false;
            } else {
                std::string proxy = interface::read_line(
                    interface::c(std::string(interface::symbol("action")) + " Proxy URL (leave blank for none): ", interface::Colors::CYAN)
                );
                auto trimmed = utils::trim(proxy);
                if (!trimmed.empty()) {
                    anon_args.proxy_url = trimmed;
                }
            }
        }
        std::string proxy_url = resolve_proxy(anon_args);
        if (anon_args.tor_enabled) {
            std::cout << interface::c(std::string(interface::symbol("ok")) + " Tor routing active.", interface::Colors::SKY) << "\n";
            if (!proxy_url.empty()) {
                std::cout << interface::c("Proxy: " + proxy_url, interface::Colors::GREY) << "\n";
            }
        } else if (!proxy_url.empty()) {
            std::cout << interface::c(std::string(interface::symbol("ok")) + " Proxy enabled.", interface::Colors::SKY) << "\n";
            std::cout << interface::c("Proxy: " + proxy_url, interface::Colors::GREY) << "\n";
        } else {
            std::cout << interface::c(std::string(interface::symbol("warn")) + " No anonymization active.", interface::Colors::RED) << "\n";
        }
        return 0;
    }

    if (command == "show") {
        if (args.targets.empty()) {
            std::cout << "show plugins | show filters | show platforms | show modules\n";
            return 0;
        }
        auto target = utils::to_lower(args.targets[0]);
        if (target == "plugins") {
            extensions::PluginManager manager;
            manager.load_all(resolve_plugin_dir());
            std::cout << "Plugins loaded: " << manager.plugins().size() << "\n";
            for (const auto& plugin : manager.plugins()) {
                auto id = plugin->spec().id ? plugin->spec().id : "";
                auto version = plugin->spec().version ? plugin->spec().version : "";
                std::cout << "- " << id;
                if (version && *version) {
                    std::cout << " (" << version << ")";
                }
                std::cout << "\n";
            }
            return 0;
        }
        if (target == "filters") {
            extensions::FilterManager manager;
            manager.load_all(resolve_filter_dir());
            std::cout << "Filters loaded: " << manager.filters().size() << "\n";
            for (const auto& filter : manager.filters()) {
                auto id = filter->spec().id ? filter->spec().id : "";
                auto version = filter->spec().version ? filter->spec().version : "";
                std::cout << "- " << id;
                if (version && *version) {
                    std::cout << " (" << version << ")";
                }
                std::cout << "\n";
            }
            return 0;
        }
        if (target == "platforms") {
            auto platforms = load_platforms_safe();
            std::cout << "Platforms loaded: " << platforms.size() << "\n";
            for (const auto& platform : platforms) {
                std::cout << "- " << platform.name << "\n";
            }
            return 0;
        }
        if (target == "modules") {
            show_modules_inventory(args);
            return 0;
        }
        return 0;
    }

    if (command == "live") {
        if (args.targets.empty()) {
            std::cerr << interface::c(std::string(interface::symbol("error")) + " No target provided.", interface::Colors::RED) << "\n";
            return 1;
        }
        for (const auto& target : args.targets) {
            open_live_target(args, target, true);
        }
        interface::CliArgs live_args = args;
        live_args.live = true;
        maybe_wait_for_live(live_args);
        return 0;
    }

    if (command == "orchestrate") {
        if (args.targets.empty()) {
            std::cerr << interface::c(std::string(interface::symbol("error")) + " Orchestrate requires <profile|surface|fusion> <target>.", interface::Colors::RED) << "\n";
            return 1;
        }
        std::string mode = normalize_command_alias(args.targets[0]);
        interface::CliArgs forwarded = args;
        forwarded.command = mode;
        forwarded.targets.assign(args.targets.begin() + 1, args.targets.end());
        if (mode == "profile" || mode == "surface" || mode == "fusion") {
            if (forwarded.targets.empty()) {
                std::cerr << interface::c(std::string(interface::symbol("error")) + " Orchestrate requires a target.", interface::Colors::RED) << "\n";
                return 1;
            }
            return handle_command(forwarded);
        }
        std::cerr << interface::c(std::string(interface::symbol("warn")) + " Unknown orchestrate mode.", interface::Colors::RED) << "\n";
        return 1;
    }

    if (command == "quicktest") {
        if (args.list_templates) {
            show_quicktest_templates();
            return 0;
        }
        std::string template_id = args.template_id;
        if (template_id.empty()) {
            const auto& templates = quicktest_templates();
            if (args.seed > 0 && !templates.empty()) {
                template_id = templates[static_cast<size_t>(args.seed) % templates.size()].id;
            } else {
                template_id = "smoke";
            }
        }

        const auto* tpl = find_quicktest_template(template_id);
        if (!tpl) {
            std::cerr << interface::c(std::string(interface::symbol("warn")) + " Unknown quicktest template.", interface::Colors::RED) << "\n";
            show_quicktest_templates();
            return 1;
        }

        interface::CliArgs working = args;
        if (!working.html_output && !working.json_output && !working.text_output && !working.csv_output) {
            working.text_output = true;
        }
        if (working.live && !working.html_output) {
            working.html_output = true;
        }
        std::string proxy_url = resolve_proxy(working);

        ExecutionPolicy profile_policy = load_policy(working.profile_preset.empty() ? working.preset : working.profile_preset);
        ExecutionPolicy surface_policy = load_policy(working.surface_preset.empty() ? working.preset : working.surface_preset);
        if (working.concurrency > 0) {
            surface_policy.concurrency = working.concurrency;
        }
        int timeout_profile = working.timeout_ms > 0 ? working.timeout_ms : profile_policy.timeout_ms;
        int timeout_surface = working.timeout_ms > 0 ? working.timeout_ms : surface_policy.timeout_ms;
        int concurrency = working.concurrency > 0 ? working.concurrency : profile_policy.concurrency;

        std::vector<std::string> profile_targets;
        std::vector<std::string> surface_targets;
        std::string fusion_user;
        std::string fusion_domain;

        if (!working.targets.empty()) {
            if (tpl->mode == "profile" || tpl->mode == "smoke") {
                profile_targets = working.targets;
            } else if (tpl->mode == "surface") {
                surface_targets = working.targets;
            } else if (tpl->mode == "fusion" && working.targets.size() >= 2) {
                fusion_user = working.targets[0];
                fusion_domain = working.targets[1];
            }
        }

        bool do_profile = tpl->mode == "profile" || tpl->mode == "smoke" || tpl->mode == "fusion";
        bool do_surface = tpl->mode == "surface" || tpl->mode == "smoke" || tpl->mode == "fusion";
        bool do_fusion = tpl->mode == "fusion";

        if (do_profile && profile_targets.empty() && !tpl->username.empty()) {
            profile_targets.push_back(tpl->username);
        }
        if (do_surface && surface_targets.empty() && !tpl->domain.empty()) {
            surface_targets.push_back(tpl->domain);
        }
        if (do_fusion) {
            if (fusion_user.empty()) {
                fusion_user = !tpl->username.empty() ? tpl->username : (profile_targets.empty() ? "" : profile_targets.front());
            }
            if (fusion_domain.empty()) {
                fusion_domain = !tpl->domain.empty() ? tpl->domain : (surface_targets.empty() ? "" : surface_targets.front());
            }
        }

        std::vector<collect::PlatformConfig> platforms;
        if (do_profile || do_fusion) {
            platforms = load_platforms_safe();
            if (platforms.empty()) {
                return 1;
            }
        }

        Orchestrator orchestrator(std::move(platforms));

        if (do_profile && !profile_targets.empty()) {
            for (const auto& username : profile_targets) {
                auto payload = run_profile_flow(orchestrator, username, profile_policy, working, timeout_profile, concurrency, proxy_url);
                std::string key = reporting::sanitize_target(username);
                write_reports(working, payload, key);
                maybe_open_live(working, username);
            }
        }

        if (do_surface && !surface_targets.empty()) {
            for (const auto& domain : surface_targets) {
                auto payload = run_surface_flow(orchestrator, domain, surface_policy, working, timeout_surface, proxy_url);
                std::string key = reporting::sanitize_target(domain);
                write_reports(working, payload, key);
                maybe_open_live(working, domain);
            }
        }

        if (do_fusion && !fusion_user.empty() && !fusion_domain.empty()) {
            auto payload = run_fusion_flow(orchestrator, fusion_user, fusion_domain, profile_policy, surface_policy, working, timeout_profile, timeout_surface, concurrency, proxy_url);
            std::string combined = fusion_user + "@" + fusion_domain;
            std::string key = reporting::sanitize_target(combined);
            write_reports(working, payload, key);
            maybe_open_live(working, combined);
        }

        maybe_wait_for_live(working);
        return 0;
    }

    if (command == "wizard") {
        interface::CliArgs wizard_args = args;
        ensure_output_settings(wizard_args);
        if (wizard_args.live && !wizard_args.html_output) {
            wizard_args.html_output = true;
        }

        std::vector<std::string> profile_targets;
        std::vector<std::string> surface_targets;
        std::string fusion_user;
        std::string fusion_domain;

        if (wizard_args.profile_phase) {
            std::string input = interface::read_line(
                interface::c(std::string(interface::symbol("action")) + " Usernames (comma/space separated): ", interface::Colors::CYAN)
            );
            profile_targets = parse_target_list(input);
        }
        if (wizard_args.surface_phase) {
            std::string input = interface::read_line(
                interface::c(std::string(interface::symbol("action")) + " Domains (comma/space separated): ", interface::Colors::CYAN)
            );
            surface_targets = parse_target_list(input);
        }
        if (wizard_args.fusion_phase) {
            fusion_user = utils::trim(interface::read_line(
                interface::c(std::string(interface::symbol("action")) + " Fusion username: ", interface::Colors::CYAN)
            ));
            fusion_domain = utils::trim(interface::read_line(
                interface::c(std::string(interface::symbol("action")) + " Fusion domain: ", interface::Colors::CYAN)
            ));
        }

        bool do_profile = !profile_targets.empty();
        bool do_surface = !surface_targets.empty();
        bool do_fusion = !fusion_user.empty() && !fusion_domain.empty();

        if (!do_profile && !do_surface && !do_fusion) {
            std::cout << interface::c(std::string(interface::symbol("warn")) + " Wizard cancelled (no targets).", interface::Colors::RED) << "\n";
            return 0;
        }

        std::vector<collect::PlatformConfig> platforms;
        if (do_profile || do_fusion) {
            platforms = load_platforms_safe();
            if (platforms.empty()) {
                return 1;
            }
        }

        ExecutionPolicy profile_policy = load_policy(wizard_args.profile_preset.empty() ? wizard_args.preset : wizard_args.profile_preset);
        ExecutionPolicy surface_policy = load_policy(wizard_args.surface_preset.empty() ? wizard_args.preset : wizard_args.surface_preset);
        if (wizard_args.concurrency > 0) {
            surface_policy.concurrency = wizard_args.concurrency;
        }
        int timeout_profile = wizard_args.timeout_ms > 0 ? wizard_args.timeout_ms : profile_policy.timeout_ms;
        int timeout_surface = wizard_args.timeout_ms > 0 ? wizard_args.timeout_ms : surface_policy.timeout_ms;
        int concurrency = wizard_args.concurrency > 0 ? wizard_args.concurrency : profile_policy.concurrency;
        std::string proxy_url = resolve_proxy(wizard_args);

        Orchestrator orchestrator(std::move(platforms));

        if (do_profile) {
            for (const auto& username : profile_targets) {
                auto payload = run_profile_flow(orchestrator, username, profile_policy, wizard_args, timeout_profile, concurrency, proxy_url);
                std::string key = reporting::sanitize_target(username);
                write_reports(wizard_args, payload, key);
                maybe_open_live(wizard_args, username);
            }
        }

        if (do_surface) {
            for (const auto& domain : surface_targets) {
                auto payload = run_surface_flow(orchestrator, domain, surface_policy, wizard_args, timeout_surface, proxy_url);
                std::string key = reporting::sanitize_target(domain);
                write_reports(wizard_args, payload, key);
                maybe_open_live(wizard_args, domain);
            }
        }

        if (do_fusion) {
            auto payload = run_fusion_flow(orchestrator, fusion_user, fusion_domain, profile_policy, surface_policy, wizard_args, timeout_profile, timeout_surface, concurrency, proxy_url);
            std::string combined = fusion_user + "@" + fusion_domain;
            std::string key = reporting::sanitize_target(combined);
            write_reports(wizard_args, payload, key);
            maybe_open_live(wizard_args, combined);
        }
        maybe_wait_for_live(wizard_args);
        return 0;
    }

    if (command != "profile" && command != "surface" && command != "fusion") {
        std::cerr << interface::c(std::string(interface::symbol("warn")) + " Unknown command. Use help.", interface::Colors::SKY) << "\n";
        return 1;
    }

    if (args.targets.empty()) {
        std::cerr << interface::c(std::string(interface::symbol("error")) + " No target provided.", interface::Colors::RED) << "\n";
        return 1;
    }

    interface::CliArgs effective = args;
    if (!effective.html_output && !effective.json_output && !effective.text_output && !effective.csv_output) {
        effective.text_output = true;
    }
    if (effective.live && !effective.html_output) {
        effective.html_output = true;
    }

    if (command == "profile") {
        auto platforms = load_platforms_safe();
        if (platforms.empty()) {
            return 1;
        }
        ExecutionPolicy policy = load_policy(effective.profile_preset.empty() ? effective.preset : effective.profile_preset);
        int timeout = effective.timeout_ms > 0 ? effective.timeout_ms : policy.timeout_ms;
        int concurrency = effective.concurrency > 0 ? effective.concurrency : policy.concurrency;
        std::string proxy_url = resolve_proxy(effective);
        Orchestrator orchestrator(std::move(platforms));

        for (const auto& username : effective.targets) {
            auto payload = run_profile_flow(orchestrator, username, policy, effective, timeout, concurrency, proxy_url);
            std::string key = reporting::sanitize_target(username);
            write_reports(effective, payload, key);
            maybe_open_live(effective, username);
        }
        maybe_wait_for_live(effective);
        return 0;
    }

    if (command == "surface") {
        ExecutionPolicy policy = load_policy(effective.surface_preset.empty() ? effective.preset : effective.surface_preset);
        if (effective.concurrency > 0) {
            policy.concurrency = effective.concurrency;
        }
        int timeout = effective.timeout_ms > 0 ? effective.timeout_ms : policy.timeout_ms;
        std::string proxy_url = resolve_proxy(effective);
        Orchestrator orchestrator(std::vector<collect::PlatformConfig>{});

        for (const auto& domain : effective.targets) {
            auto payload = run_surface_flow(orchestrator, domain, policy, effective, timeout, proxy_url);
            std::string key = reporting::sanitize_target(domain);
            write_reports(effective, payload, key);
            maybe_open_live(effective, domain);
        }
        maybe_wait_for_live(effective);
        return 0;
    }

    if (command == "fusion") {
        if (effective.targets.size() < 2) {
            std::cerr << interface::c(std::string(interface::symbol("error")) + " Fusion requires <username> <domain>.", interface::Colors::RED) << "\n";
            return 1;
        }
        auto platforms = load_platforms_safe();
        if (platforms.empty()) {
            return 1;
        }
        ExecutionPolicy profile_policy = load_policy(effective.profile_preset.empty() ? effective.preset : effective.profile_preset);
        ExecutionPolicy surface_policy = load_policy(effective.surface_preset.empty() ? effective.preset : effective.surface_preset);
        if (effective.concurrency > 0) {
            surface_policy.concurrency = effective.concurrency;
        }
        int timeout_profile = effective.timeout_ms > 0 ? effective.timeout_ms : profile_policy.timeout_ms;
        int timeout_surface = effective.timeout_ms > 0 ? effective.timeout_ms : surface_policy.timeout_ms;
        int concurrency = effective.concurrency > 0 ? effective.concurrency : profile_policy.concurrency;
        std::string proxy_url = resolve_proxy(effective);

        const auto& username = effective.targets[0];
        const auto& domain = effective.targets[1];
        Orchestrator orchestrator(std::move(platforms));
        auto payload = run_fusion_flow(orchestrator, username, domain, profile_policy, surface_policy, effective, timeout_profile, timeout_surface, concurrency, proxy_url);
        std::string combined = username + "@" + domain;
        std::string key = reporting::sanitize_target(combined);
        write_reports(effective, payload, key);
        maybe_open_live(effective, combined);
        maybe_wait_for_live(effective);
        return 0;
    }

    return 0;
}

} // namespace

int run(int argc, char* argv[]) {
    auto args = interface::parse_args(argc, argv);
    if (args.prompt_mode) {
        std::string anonymity = collect::is_tor_running() ? "Tor Active" : "No Anonymization";
        interface::show_banner(anonymity);
        auto platforms = load_platforms_safe();
        extensions::PluginManager plugin_manager;
        plugin_manager.load_all(resolve_plugin_dir());
        extensions::FilterManager filter_manager;
        filter_manager.load_all(resolve_filter_dir());
        std::cout << interface::c(std::string(interface::symbol("feature")) + " Platforms: " + std::to_string(platforms.size()), interface::Colors::GREY) << "\n";
        std::cout << interface::c(std::string(interface::symbol("feature")) + " Plugins: " + std::to_string(plugin_manager.plugins().size()), interface::Colors::GREY) << "\n";
        std::cout << interface::c(std::string(interface::symbol("feature")) + " Filters: " + std::to_string(filter_manager.filters().size()), interface::Colors::GREY) << "\n";
        std::cout << interface::c(std::string(interface::symbol("feature")) + " Modules: " + std::to_string(modules::all_modules().size()), interface::Colors::GREY) << "\n";
        std::cout << interface::c(std::string(interface::symbol("action")) + " Type 'help' for commands.", interface::Colors::GREY) << "\n\n";
        int code = interface::run_prompt([](const interface::CliArgs& inner) {
            return handle_command(inner);
        });
        live_server_instance().stop();
        return code;
    }
    return handle_command(args);
}

} // namespace silicore

