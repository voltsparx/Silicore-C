#include "core/runner.h"

#include "core/collect/platform_schema.h"
#include "core/extensions/plugin_loader.h"
#include "core/interface/cli_parser.h"
#include "core/interface/prompt.h"
#include "core/execution_policy.h"
#include "core/orchestrator.h"
#include "core/reporting/reporting.h"
#include "core/utils/strings.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace silicore {

namespace {

std::string resolve_proxy(const interface::CliArgs& args) {
    if (args.tor_enabled) {
        return "socks5h://127.0.0.1:9050";
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

void print_help() {
    std::cout << "Silicore-C v1.0\n";
    std::cout << "Commands:\n";
    std::cout << "  profile <username> [--preset fast|balanced|deep|max] [--timeout ms] [--concurrency n] [--proxy url] [--tor] [--html] [--json] [--plugins a,b] [--all-plugins]\n";
    std::cout << "  surface <domain> [--preset fast|balanced|deep|max] [--timeout ms] [--proxy url] [--tor] [--html] [--json] [--plugins a,b] [--all-plugins]\n";
    std::cout << "  show plugins\n";
    std::cout << "  show platforms\n";
    std::cout << "  help\n";
}

std::vector<collect::PlatformConfig> load_platforms_safe() {
    try {
        return collect::load_platforms("platforms");
    } catch (const std::exception& exc) {
        std::cerr << "Platform load error: " << exc.what() << "\n";
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

int handle_command(const interface::CliArgs& args) {
    std::string command = utils::to_lower(args.command);
    if (command.empty() || command == "help") {
        print_help();
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

    if (command != "profile" && command != "surface") {
        std::cerr << "Unknown command. Use help.\n";
        return 1;
    }

    if (args.targets.empty()) {
        std::cerr << "No target provided.\n";
        return 1;
    }

    auto platforms = load_platforms_safe();
    if (platforms.empty() && args.command == "profile") {
        return 1;
    }

    auto policy = load_policy(args.preset);
    int timeout = args.timeout_ms > 0 ? args.timeout_ms : policy.timeout_ms;
    int concurrency = args.concurrency > 0 ? args.concurrency : policy.concurrency;
    auto proxy_url = resolve_proxy(args);

    Orchestrator orchestrator(std::move(platforms));

    if (command == "profile") {
        const auto& username = args.targets[0];
        auto profile = orchestrator.run_profile(username, policy, timeout, concurrency, proxy_url);
        auto context_json = reporting::build_report_payload(username, profile.scan_result.profiles, nullptr, {}, "profile");
        auto plugins = run_plugins(args, "profile", context_json);
        auto payload = reporting::build_report_payload(username, profile.scan_result.profiles, nullptr, plugins, "profile");

        std::string key = reporting::sanitize_target(username);
        auto out_dir = std::filesystem::path("output") / key;
        auto cli_report = reporting::render_cli_report(payload);
        std::cout << cli_report;
        reporting::write_text_report(cli_report, out_dir / "report.txt");
        if (args.json_output) {
            reporting::write_json_report(payload, out_dir / "results.json");
        }
        if (args.html_output) {
            auto html = reporting::render_html_report(payload);
            reporting::write_text_report(html, out_dir / "report.html");
        }
        return 0;
    }

    if (command == "surface") {
        const auto& domain = args.targets[0];
        auto surface = orchestrator.run_surface(domain, policy, timeout, proxy_url);
        auto context_json = reporting::build_report_payload(domain, {}, &surface.scan_result, {}, "surface");
        auto plugins = run_plugins(args, "surface", context_json);
        auto payload = reporting::build_report_payload(domain, {}, &surface.scan_result, plugins, "surface");

        std::string key = reporting::sanitize_target(domain);
        auto out_dir = std::filesystem::path("output") / key;
        auto cli_report = reporting::render_cli_report(payload);
        std::cout << cli_report;
        reporting::write_text_report(cli_report, out_dir / "report.txt");
        if (args.json_output) {
            reporting::write_json_report(payload, out_dir / "results.json");
        }
        if (args.html_output) {
            auto html = reporting::render_html_report(payload);
            reporting::write_text_report(html, out_dir / "report.html");
        }
        return 0;
    }

    return 0;
}

} // namespace

int run(int argc, char* argv[]) {
    auto args = interface::parse_args(argc, argv);
    if (args.prompt_mode) {
        return interface::run_prompt([](const interface::CliArgs& inner) {
            return handle_command(inner);
        });
    }
    return handle_command(args);
}

} // namespace silicore
