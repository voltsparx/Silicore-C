#include "runner.h"

#include "collect/platform_schema.h"
#include "collect/anonymity.h"
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

#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>

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
            std::cout << interface::c("Start and configure Tor now? (y/N): ", interface::Colors::GREY);
            std::string reply;
            std::getline(std::cin, reply);
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

struct OutputPaths {
    std::filesystem::path cli_path;
    std::filesystem::path html_path;
    std::filesystem::path json_path;
};

OutputPaths resolve_output_paths(const interface::CliArgs& args, const std::string& target_key) {
    auto root = resolve_output_root(args) / "output";
    OutputPaths paths;
    paths.cli_path = root / "cli" / (target_key + ".txt");
    paths.html_path = root / "html" / (target_key + ".html");
    paths.json_path = root / "data" / target_key / "results.json";
    return paths;
}

void print_help() {
    using namespace interface;
    std::cout << c(std::string(symbol("major")) + " " + foundation::PROJECT_NAME + " v" + foundation::VERSION, Colors::SKY_DARK) << "\n";
    std::cout << c(std::string(symbol("action")) + " Commands:", Colors::CYAN) << "\n";
    std::cout << c("  profile <username> [--preset fast|balanced|deep|max] [--timeout ms] [--concurrency n] [--proxy url] [--tor] [--txt] [--html] [--json] [--out dir] [--plugins a,b] [--all-plugins] [--filters a,b] [--all-filters]", Colors::GREY) << "\n";
    std::cout << c("  surface <domain> [--preset fast|balanced|deep|max] [--timeout ms] [--proxy url] [--tor] [--txt] [--html] [--json] [--out dir] [--plugins a,b] [--all-plugins] [--filters a,b] [--all-filters]", Colors::GREY) << "\n";
    std::cout << c("  fusion <username> <domain> [--preset fast|balanced|deep|max] [--timeout ms] [--concurrency n] [--proxy url] [--tor] [--txt] [--html] [--json] [--out dir] [--plugins a,b] [--all-plugins] [--filters a,b] [--all-filters]", Colors::GREY) << "\n";
    std::cout << c("  about | --about", Colors::GREY) << "\n";
    std::cout << c("  explain | --explain", Colors::GREY) << "\n";
    std::cout << c("  show plugins", Colors::GREY) << "\n";
    std::cout << c("  show filters", Colors::GREY) << "\n";
    std::cout << c("  show platforms", Colors::GREY) << "\n";
    std::cout << c("  help", Colors::GREY) << "\n";
    std::cout << c(std::string(symbol("feature")) + " Outputs:", Colors::CYAN) << "\n";
    std::cout << c("  output/data/<target>/results.json | output/html/<target>.html | output/cli/<target>.txt", Colors::GREY) << "\n";
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

std::vector<extensions::PluginResult> run_plugins(
    const interface::CliArgs& args,
    const std::string& scope,
    const reporting::json& context_json
) {
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
int handle_command(const interface::CliArgs& args) {
    std::string command = utils::to_lower(args.command);
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

    if (command == "show") {
        if (args.targets.empty()) {
            std::cout << "show plugins | show platforms\n";
            return 0;
        }
        auto target = utils::to_lower(args.targets[0]);
        if (target == "plugins") {
            extensions::PluginManager manager;
            manager.load_all(resolve_plugin_dir());
            std::cout << "Plugins loaded: " << manager.plugins().size() << "\n";
            for (const auto& plugin : manager.plugins()) {
                std::cout << "- " << plugin->spec().id << " (" << plugin->spec().version << ")\n";
            }
            return 0;
        }
        if (target == "filters") {
            extensions::FilterManager manager;
            manager.load_all(resolve_filter_dir());
            std::cout << "Filters loaded: " << manager.filters().size() << "\n";
            for (const auto& filter : manager.filters()) {
                std::cout << "- " << filter->spec().id << " (" << filter->spec().version << ")\n";
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

    auto platforms = load_platforms_safe();
    if (platforms.empty() && (args.command == "profile" || args.command == "fusion")) {
        return 1;
    }

    auto policy = load_policy(args.preset);
    int timeout = args.timeout_ms > 0 ? args.timeout_ms : policy.timeout_ms;
    int concurrency = args.concurrency > 0 ? args.concurrency : policy.concurrency;
    auto proxy_url = resolve_proxy(args);

    Orchestrator orchestrator(std::move(platforms));

    if (command == "fusion") {
        if (args.targets.size() < 2) {
            std::cerr << interface::c(std::string(interface::symbol("error")) + " Fusion requires <username> <domain>.", interface::Colors::RED) << "\n";
            return 1;
        }
        const auto& username = args.targets[0];
        const auto& domain = args.targets[1];

        auto profile_future = std::async(std::launch::async, [&]() {
            return orchestrator.run_profile(username, policy, timeout, concurrency, proxy_url);
        });
        auto surface_future = std::async(std::launch::async, [&]() {
            return orchestrator.run_surface(domain, policy, timeout, proxy_url);
        });

        auto profile = profile_future.get();
        auto surface = surface_future.get();

        auto profile_payload = reporting::build_report_payload(username, profile.scan_result.profiles, nullptr, {}, nullptr, {}, "profile");
        auto surface_payload = reporting::build_report_payload(domain, {}, &surface.scan_result, {}, nullptr, {}, "surface");

        engines::FusionEngine fusion_engine;
        auto fused = fusion_engine.fuse_profile_domain(profile_payload, surface_payload);
        fused["graph"] = fusion_engine.generate_graph(fused);

        std::string combined_target = username + "@" + domain;
        auto context_json = reporting::build_report_payload(combined_target, profile.scan_result.profiles, &surface.scan_result, {}, &fused, {}, "fusion");
        auto plugins = run_plugins(args, "fusion", context_json);
        auto filters = run_filters(args, "fusion", context_json);
        auto payload = reporting::build_report_payload(combined_target, profile.scan_result.profiles, &surface.scan_result, plugins, &fused, filters, "fusion");

        std::string key = reporting::sanitize_target(combined_target);
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
        return 0;
    }

    if (command == "profile") {
        const auto& username = args.targets[0];
        auto profile = orchestrator.run_profile(username, policy, timeout, concurrency, proxy_url);
        auto context_json = reporting::build_report_payload(username, profile.scan_result.profiles, nullptr, {}, nullptr, {}, "profile");
        auto plugins = run_plugins(args, "profile", context_json);
        auto filters = run_filters(args, "profile", context_json);
        auto payload = reporting::build_report_payload(username, profile.scan_result.profiles, nullptr, plugins, nullptr, filters, "profile");

        std::string key = reporting::sanitize_target(username);
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
        return 0;
    }

    if (command == "surface") {
        const auto& domain = args.targets[0];
        auto surface = orchestrator.run_surface(domain, policy, timeout, proxy_url);
        auto context_json = reporting::build_report_payload(domain, {}, &surface.scan_result, {}, nullptr, {}, "surface");
        auto plugins = run_plugins(args, "surface", context_json);
        auto filters = run_filters(args, "surface", context_json);
        auto payload = reporting::build_report_payload(domain, {}, &surface.scan_result, plugins, nullptr, filters, "surface");

        std::string key = reporting::sanitize_target(domain);
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
        return 0;
    }

    return 0;
}

} // namespace

int run(int argc, char* argv[]) {
    auto args = interface::parse_args(argc, argv);
    if (args.prompt_mode) {
        interface::show_banner("No Anonymization");
        return interface::run_prompt([](const interface::CliArgs& inner) {
            return handle_command(inner);
        });
    }
    return handle_command(args);
}

} // namespace silicore

