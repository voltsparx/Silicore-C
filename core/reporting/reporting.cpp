#include "core/reporting/reporting.h"

#include "core/interface/colors.h"
#include "core/interface/symbols.h"
#include "core/foundation/metadata.h"
#include "core/utils/strings.h"
#include "core/utils/time.h"

#include <fstream>
#include <sstream>
#include <cctype>

namespace silicore::reporting {

namespace {

json build_plugin_json(const std::vector<extensions::PluginResult>& plugins) {
    json out = json::array();
    for (const auto& plugin : plugins) {
        json entry;
        entry["id"] = plugin.id;
        entry["title"] = plugin.title;
        entry["version"] = plugin.version;
        entry["severity"] = plugin.severity;
        if (!plugin.output_json.empty()) {
            try {
                entry["output"] = json::parse(plugin.output_json);
            } catch (const std::exception&) {
                entry["output_raw"] = plugin.output_json;
            }
        }
        out.push_back(entry);
    }
    return out;
}

const char* status_color(const std::string& status) {
    using namespace silicore::interface;
    if (status == "FOUND") return Colors::GREEN;
    if (status == "NOT_FOUND") return Colors::GREY;
    if (status == "BLOCKED") return Colors::YELLOW;
    if (status == "INVALID_USERNAME") return Colors::YELLOW;
    if (status == "ERROR") return Colors::RED;
    return Colors::GREY;
}

} // namespace

json build_profile_results_json(const std::vector<domain::ProfileEntity>& profiles) {
    json out = json::array();
    for (const auto& profile : profiles) {
        json entry;
        entry["platform"] = profile.platform;
        entry["status"] = profile.status;
        entry["url"] = profile.profile_url;
        entry["confidence"] = static_cast<int>(profile.confidence * 100.0f);
        entry["http_status"] = profile.http_status;
        entry["response_time_ms"] = profile.response_time_ms;
        if (!profile.context.empty()) {
            entry["context"] = profile.context;
        }
        out.push_back(entry);
    }
    return out;
}

json build_domain_result_json(const collect::DomainScanResult& result) {
    json out;
    out["target"] = result.target_domain;
    out["resolved_addresses"] = result.resolved_addresses;
    out["https"] = {
        {"status", result.https.status},
        {"final_url", result.https.final_url},
        {"headers", result.https.headers},
    };
    out["http"] = {
        {"status", result.http.status},
        {"final_url", result.http.final_url},
        {"redirects_to_https", result.http_redirects_to_https},
    };
    out["subdomains"] = result.subdomains;
    out["rdap"] = {
        {"handle", result.rdap.handle},
        {"registrar", result.rdap.registrar},
        {"name_servers", result.rdap.name_servers},
    };
    out["robots_txt_present"] = result.robots_txt_present;
    out["robots_preview"] = result.robots_preview;
    out["security_txt_present"] = result.security_txt_present;
    out["security_preview"] = result.security_preview;
    out["scan_notes"] = result.scan_notes;
    return out;
}

json build_report_payload(
    const std::string& target,
    const std::vector<domain::ProfileEntity>& profiles,
    const collect::DomainScanResult* domain_result,
    const std::vector<extensions::PluginResult>& plugins,
    const std::string& mode
) {
    json payload;
    payload["metadata"] = {
        {"generated_at_utc", utils::utc_timestamp()},
        {"mode", mode},
        {"framework", std::string(foundation::PROJECT_NAME) + " v" + foundation::VERSION},
    };

    payload["target"] = target;
    payload["results"] = build_profile_results_json(profiles);
    if (domain_result) {
        payload["domain_result"] = build_domain_result_json(*domain_result);
    } else {
        payload["domain_result"] = nullptr;
    }
    payload["plugins"] = build_plugin_json(plugins);

    int found = 0;
    int not_found = 0;
    int blocked = 0;
    int error = 0;
    for (const auto& profile : profiles) {
        if (profile.status == "FOUND") found++;
        else if (profile.status == "NOT_FOUND") not_found++;
        else if (profile.status == "BLOCKED") blocked++;
        else error++;
    }

    payload["summary"] = {
        {"found_count", found},
        {"not_found_count", not_found},
        {"blocked_count", blocked},
        {"error_count", error},
        {"total_results", static_cast<int>(profiles.size())},
    };

    return payload;
}

std::string render_cli_report(const json& payload) {
    std::ostringstream out;
    using namespace silicore::interface;

    out << c(std::string(symbol("major")) + " " + foundation::PROJECT_NAME + " Report", Colors::BLUE) << "\n";
    out << c(std::string(48, '='), Colors::BLUE) << "\n";
    out << c(std::string(symbol("action")) + " Target: " + payload.value("target", "-"), Colors::CYAN) << "\n";
    if (payload.contains("metadata")) {
        const auto& meta = payload["metadata"];
        out << c(std::string(symbol("feature")) + " Mode: " + meta.value("mode", "-"), Colors::CYAN) << "\n";
        out << c(std::string(symbol("feature")) + " Generated: " + meta.value("generated_at_utc", "-"), Colors::GREY) << "\n";
    }

    if (payload.contains("summary")) {
        const auto& summary = payload["summary"];
        out << c(std::string(symbol("minor")) + " Summary", Colors::BLUE) << "\n";
        out << c("Found: ", Colors::GREEN) << summary.value("found_count", 0)
            << c(" | Not Found: ", Colors::GREY) << summary.value("not_found_count", 0)
            << c(" | Blocked: ", Colors::YELLOW) << summary.value("blocked_count", 0)
            << c(" | Error: ", Colors::RED) << summary.value("error_count", 0) << "\n";
    }

    if (payload.contains("results") && payload["results"].is_array()) {
        out << "\n" << c(std::string(symbol("major")) + " Profiles", Colors::BLUE) << "\n";
        for (const auto& entry : payload["results"]) {
            std::string status = entry.value("status", "-");
            out << c(std::string(symbol("bullet")) + " " + entry.value("platform", "-") + ": ", Colors::CYAN)
                << c(status, status_color(status))
                << c(" (" + entry.value("url", "-") + ")", Colors::GREY) << "\n";
        }
    }

    if (payload.contains("domain_result") && payload["domain_result"].is_object()) {
        auto dr = payload["domain_result"];
        out << "\n" << c(std::string(symbol("major")) + " Domain Surface", Colors::BLUE) << "\n";
        out << c(std::string(symbol("bullet")) + " Resolved: ", Colors::CYAN)
            << utils::join(dr.value("resolved_addresses", std::vector<std::string>{}), ", ") << "\n";
        out << c(std::string(symbol("bullet")) + " Subdomains: ", Colors::CYAN)
            << dr.value("subdomains", json::array()).size() << "\n";
        out << c(std::string(symbol("bullet")) + " RDAP Handle: ", Colors::CYAN)
            << dr["rdap"].value("handle", "-") << "\n";
        out << c(std::string(symbol("bullet")) + " Robots.txt: ", Colors::CYAN)
            << (dr.value("robots_txt_present", false) ? "yes" : "no") << "\n";
        out << c(std::string(symbol("bullet")) + " Security.txt: ", Colors::CYAN)
            << (dr.value("security_txt_present", false) ? "yes" : "no") << "\n";
    }

    if (payload.contains("plugins") && payload["plugins"].is_array() && !payload["plugins"].empty()) {
        out << "\n" << c(std::string(symbol("major")) + " Plugins", Colors::BLUE) << "\n";
        for (const auto& entry : payload["plugins"]) {
            out << c(std::string(symbol("feature")) + " " + entry.value("id", "-"), Colors::YELLOW)
                << c(" (severity " + std::to_string(entry.value("severity", 0)) + ")", Colors::GREY) << "\n";
        }
    }

    return out.str();
}

std::string render_html_report(const json& payload) {
    std::ostringstream out;
    out << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Silicore-C Report</title>";
    out << "<style>";
    out << "body{font-family:\"Segoe UI\",sans-serif;margin:24px;background:#f7f9fc;color:#0e1a2b;}";
    out << ".card{border:1px solid #d7e0ec;background:#fff;border-radius:10px;padding:14px;margin-bottom:12px;}";
    out << "table{width:100%;border-collapse:collapse;}th,td{border-bottom:1px solid #e1e8f0;padding:8px;text-align:left;}";
    out << "th{background:#f1f5fb;} .muted{color:#58657a;}";
    out << "</style></head><body>";
    out << "<h1>Silicore-C Report</h1>";
    out << "<div class='card'><strong>Target:</strong> " << payload.value("target", "-") << "</div>";

    if (payload.contains("summary")) {
        const auto& summary = payload["summary"];
        out << "<div class='card'><strong>Summary:</strong> Found " << summary.value("found_count", 0)
            << ", Not Found " << summary.value("not_found_count", 0)
            << ", Blocked " << summary.value("blocked_count", 0)
            << ", Error " << summary.value("error_count", 0) << "</div>";
    }

    if (payload.contains("results") && payload["results"].is_array()) {
        out << "<div class='card'><h2>Profiles</h2><ul>";
        for (const auto& entry : payload["results"]) {
            out << "<li>" << entry.value("platform", "-") << " : " << entry.value("status", "-")
                << " (<a href='" << entry.value("url", "-") << "'>link</a>)</li>";
        }
        out << "</ul></div>";
    }

    if (payload.contains("domain_result") && payload["domain_result"].is_object()) {
        auto dr = payload["domain_result"];
        out << "<div class='card'><h2>Domain Surface</h2>";
        out << "<div><strong>Resolved:</strong> " << utils::join(dr.value("resolved_addresses", std::vector<std::string>{}), ", ") << "</div>";
        out << "<div><strong>Subdomains:</strong> " << dr.value("subdomains", json::array()).size() << "</div>";
        out << "<div><strong>RDAP Handle:</strong> " << dr["rdap"].value("handle", "-") << "</div>";
        out << "</div>";
    }

    out << "</body></html>";
    return out.str();
}

void write_json_report(const json& payload, const std::filesystem::path& out_path) {
    std::filesystem::create_directories(out_path.parent_path());
    std::ofstream handle(out_path);
    handle << payload.dump(2);
}

void write_text_report(const std::string& text, const std::filesystem::path& out_path) {
    std::filesystem::create_directories(out_path.parent_path());
    std::ofstream handle(out_path);
    handle << text;
}

std::string sanitize_target(const std::string& target) {
    std::string out;
    for (char c : target) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            out.push_back(static_cast<char>(std::tolower(c)));
        } else {
            out.push_back('_');
        }
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }
    return out.empty() ? "target" : out;
}

} // namespace silicore::reporting
