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
    if (status == "BLOCKED") return Colors::SKY;
    if (status == "INVALID_USERNAME") return Colors::SKY;
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
    const json* fusion_result,
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
    if (fusion_result) {
        payload["fusion_result"] = *fusion_result;
    } else {
        payload["fusion_result"] = nullptr;
    }

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

    out << c(std::string(symbol("major")) + " " + foundation::PROJECT_NAME + " Report", Colors::SKY_DARK) << "\n";
    out << c(std::string(48, '='), Colors::SKY_DARK) << "\n";
    out << c(std::string(symbol("action")) + " Target: " + payload.value("target", "-"), Colors::CYAN) << "\n";
    if (payload.contains("metadata")) {
        const auto& meta = payload["metadata"];
        out << c(std::string(symbol("feature")) + " Mode: " + meta.value("mode", "-"), Colors::CYAN) << "\n";
        out << c(std::string(symbol("feature")) + " Generated: " + meta.value("generated_at_utc", "-"), Colors::GREY) << "\n";
    }

    if (payload.contains("summary")) {
        const auto& summary = payload["summary"];
        out << c(std::string(symbol("minor")) + " Summary", Colors::SKY_DARK) << "\n";
        out << c("Found: ", Colors::GREEN) << summary.value("found_count", 0)
            << c(" | Not Found: ", Colors::GREY) << summary.value("not_found_count", 0)
            << c(" | Blocked: ", Colors::SKY) << summary.value("blocked_count", 0)
            << c(" | Error: ", Colors::RED) << summary.value("error_count", 0) << "\n";
    }

    if (payload.contains("results") && payload["results"].is_array()) {
        out << "\n" << c(std::string(symbol("major")) + " Profiles", Colors::SKY_DARK) << "\n";
        for (const auto& entry : payload["results"]) {
            std::string status = entry.value("status", "-");
            out << c(std::string(symbol("bullet")) + " " + entry.value("platform", "-") + ": ", Colors::CYAN)
                << c(status, status_color(status))
                << c(" (" + entry.value("url", "-") + ")", Colors::GREY) << "\n";
        }
    }

    if (payload.contains("domain_result") && payload["domain_result"].is_object()) {
        const auto& dr = payload["domain_result"];
        out << "\n" << c(std::string(symbol("major")) + " Domain Surface", Colors::SKY_DARK) << "\n";
        out << c(std::string(symbol("bullet")) + " Resolved: ", Colors::CYAN)
            << utils::join(dr.value("resolved_addresses", std::vector<std::string>{}), ", ") << "\n";
        out << c(std::string(symbol("bullet")) + " Subdomains: ", Colors::CYAN)
            << dr.value("subdomains", json::array()).size() << "\n";
        if (dr.contains("rdap") && dr["rdap"].is_object()) {
            out << c(std::string(symbol("bullet")) + " RDAP Handle: ", Colors::CYAN)
                << dr["rdap"].value("handle", "-") << "\n";
        }
        out << c(std::string(symbol("bullet")) + " Robots.txt: ", Colors::CYAN)
            << (dr.value("robots_txt_present", false) ? "yes" : "no") << "\n";
        out << c(std::string(symbol("bullet")) + " Security.txt: ", Colors::CYAN)
            << (dr.value("security_txt_present", false) ? "yes" : "no") << "\n";
    }

    if (payload.contains("fusion_result") && payload["fusion_result"].is_object()) {
        const auto& fr = payload["fusion_result"];
        out << "\n" << c(std::string(symbol("major")) + " Fusion", Colors::SKY_DARK) << "\n";
        out << c(std::string(symbol("bullet")) + " Confidence: ", Colors::CYAN)
            << fr.value("confidence_score", 0) << "\n";
        if (fr.contains("profile") && fr["profile"].is_object()) {
            out << c(std::string(symbol("bullet")) + " Found Profiles: ", Colors::CYAN)
                << fr["profile"].value("found_profiles", 0) << "\n";
        }
        if (fr.contains("domain") && fr["domain"].is_object()) {
            out << c(std::string(symbol("bullet")) + " Subdomains: ", Colors::CYAN)
                << fr["domain"].value("subdomain_count", 0) << "\n";
        }
        if (fr.contains("anomalies") && fr["anomalies"].is_array() && !fr["anomalies"].empty()) {
            out << c(std::string(symbol("bullet")) + " Anomalies: ", Colors::CYAN);
            bool first = true;
            for (const auto& item : fr["anomalies"]) {
                if (!item.is_string()) {
                    continue;
                }
                if (!first) {
                    out << ", ";
                }
                out << item.get<std::string>();
                first = false;
            }
            out << "\n";
        }
    }

    if (payload.contains("plugins") && payload["plugins"].is_array() && !payload["plugins"].empty()) {
        out << "\n" << c(std::string(symbol("major")) + " Plugins", Colors::SKY_DARK) << "\n";
        for (const auto& plugin : payload["plugins"]) {
            std::string title = plugin.value("title", plugin.value("id", "-"));
            out << c(std::string(symbol("bullet")) + " " + title, Colors::CYAN);
            if (plugin.contains("version")) {
                out << c(" v" + plugin.value("version", ""), Colors::GREY);
            }
            out << c(" (severity " + std::to_string(plugin.value("severity", 0)) + ")", Colors::GREY) << "\n";
            if (plugin.contains("output")) {
                out << c("  output: ", Colors::GREY) << plugin["output"].dump() << "\n";
            } else if (plugin.contains("output_raw")) {
                out << c("  output: ", Colors::GREY) << plugin.value("output_raw", "") << "\n";
            }
        }
    }

    return out.str();
}

std::string render_html_report(const json& payload) {
    auto html_escape = [](const std::string& input) {
        std::string out;
        out.reserve(input.size());
        for (char ch : input) {
            switch (ch) {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                case '\'': out += "&#39;"; break;
                default: out.push_back(ch); break;
            }
        }
        return out;
    };

    std::ostringstream out;
    out << "<!doctype html>";
    out << "<html lang='en'><head>";
    out << "<meta charset='utf-8'>";
    out << "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    out << "<title>" << foundation::PROJECT_NAME << " Report</title>";
    out << "<style>";
    out << "body{margin:0;font-family:Arial,Helvetica,sans-serif;background:#0f1720;color:#e2e8f0;}";
    out << ".wrap{max-width:1100px;margin:0 auto;padding:32px;}";
    out << "h1,h2{margin:0 0 8px 0;}";
    out << ".header{padding:16px 20px;background:#162231;border-radius:12px;border:1px solid #1f2f44;}";
    out << ".meta{color:#9fb3c8;font-size:14px;}";
    out << ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px;margin-top:16px;}";
    out << ".card{background:#162231;border:1px solid #1f2f44;border-radius:12px;padding:16px;}";
    out << ".label{color:#9fb3c8;font-size:12px;text-transform:uppercase;letter-spacing:.08em;}";
    out << ".value{font-size:20px;margin-top:6px;color:#87ceeb;}";
    out << "table{width:100%;border-collapse:collapse;margin-top:12px;font-size:14px;}";
    out << "th,td{text-align:left;padding:10px;border-bottom:1px solid #1f2f44;}";
    out << "th{color:#9fb3c8;font-weight:600;}";
    out << ".badge{display:inline-block;padding:2px 8px;border-radius:999px;background:#1f2f44;color:#e2e8f0;font-size:12px;}";
    out << ".section{margin-top:24px;}";
    out << ".accent{color:#87ceeb;}";
    out << "</style></head><body><div class='wrap'>";

    std::string target = payload.value("target", "-");
    std::string mode = "-";
    std::string generated = "-";
    if (payload.contains("metadata")) {
        const auto& meta = payload["metadata"];
        mode = meta.value("mode", "-");
        generated = meta.value("generated_at_utc", "-");
    }
    out << "<div class='header'>";
    out << "<h1>" << foundation::PROJECT_NAME << " Report</h1>";
    out << "<div class='meta'>Target: <span class='accent'>" << html_escape(target) << "</span>";
    out << " | Mode: " << html_escape(mode);
    out << " | Generated: " << html_escape(generated) << "</div>";
    out << "</div>";

    if (payload.contains("summary")) {
        const auto& summary = payload["summary"];
        out << "<div class='grid'>";
        out << "<div class='card'><div class='label'>Found</div><div class='value'>"
            << summary.value("found_count", 0) << "</div></div>";
        out << "<div class='card'><div class='label'>Not Found</div><div class='value'>"
            << summary.value("not_found_count", 0) << "</div></div>";
        out << "<div class='card'><div class='label'>Blocked</div><div class='value'>"
            << summary.value("blocked_count", 0) << "</div></div>";
        out << "<div class='card'><div class='label'>Errors</div><div class='value'>"
            << summary.value("error_count", 0) << "</div></div>";
        out << "</div>";
    }

    if (payload.contains("results") && payload["results"].is_array()) {
        out << "<div class='section'>";
        out << "<h2>Profiles</h2>";
        out << "<table><thead><tr><th>Platform</th><th>Status</th><th>URL</th><th>Confidence</th><th>HTTP</th><th>RT (ms)</th></tr></thead><tbody>";
        for (const auto& entry : payload["results"]) {
            out << "<tr>";
            out << "<td>" << html_escape(entry.value("platform", "-")) << "</td>";
            out << "<td><span class='badge'>" << html_escape(entry.value("status", "-")) << "</span></td>";
            out << "<td>" << html_escape(entry.value("url", "-")) << "</td>";
            out << "<td>" << entry.value("confidence", 0) << "</td>";
            out << "<td>" << entry.value("http_status", 0) << "</td>";
            out << "<td>" << entry.value("response_time_ms", 0) << "</td>";
            out << "</tr>";
        }
        out << "</tbody></table></div>";
    }

    if (payload.contains("domain_result") && payload["domain_result"].is_object()) {
        const auto& dr = payload["domain_result"];
        out << "<div class='section'><h2>Domain Surface</h2>";
        out << "<div class='card'>";
        out << "<div><strong>Resolved:</strong> "
            << html_escape(utils::join(dr.value("resolved_addresses", std::vector<std::string>{}), ", "))
            << "</div>";
        out << "<div><strong>Subdomains:</strong> " << dr.value("subdomains", json::array()).size() << "</div>";
        if (dr.contains("rdap") && dr["rdap"].is_object()) {
            out << "<div><strong>RDAP Handle:</strong> " << html_escape(dr["rdap"].value("handle", "-")) << "</div>";
            out << "<div><strong>Registrar:</strong> " << html_escape(dr["rdap"].value("registrar", "-")) << "</div>";
        }
        out << "<div><strong>Robots.txt:</strong> " << (dr.value("robots_txt_present", false) ? "yes" : "no") << "</div>";
        out << "<div><strong>Security.txt:</strong> " << (dr.value("security_txt_present", false) ? "yes" : "no") << "</div>";
        out << "</div></div>";
    }

    if (payload.contains("fusion_result") && payload["fusion_result"].is_object()) {
        const auto& fr = payload["fusion_result"];
        out << "<div class='section'><h2>Fusion Summary</h2>";
        out << "<div class='card'>";
        out << "<div><strong>Confidence:</strong> " << fr.value("confidence_score", 0) << "</div>";
        if (fr.contains("profile") && fr["profile"].is_object()) {
            out << "<div><strong>Found Profiles:</strong> " << fr["profile"].value("found_profiles", 0) << "</div>";
        }
        if (fr.contains("domain") && fr["domain"].is_object()) {
            out << "<div><strong>Subdomains:</strong> " << fr["domain"].value("subdomain_count", 0) << "</div>";
        }
        if (fr.contains("anomalies") && fr["anomalies"].is_array() && !fr["anomalies"].empty()) {
            out << "<div><strong>Anomalies:</strong> ";
            bool first = true;
            for (const auto& item : fr["anomalies"]) {
                if (!item.is_string()) {
                    continue;
                }
                if (!first) {
                    out << ", ";
                }
                out << html_escape(item.get<std::string>());
                first = false;
            }
            out << "</div>";
        }
        out << "</div></div>";
    }

    if (payload.contains("plugins") && payload["plugins"].is_array() && !payload["plugins"].empty()) {
        out << "<div class='section'><h2>Plugins</h2>";
        for (const auto& plugin : payload["plugins"]) {
            out << "<div class='card'>";
            out << "<div><strong>" << html_escape(plugin.value("title", plugin.value("id", "-"))) << "</strong>";
            if (plugin.contains("version")) {
                out << " <span class='badge'>v" << html_escape(plugin.value("version", "")) << "</span>";
            }
            out << " <span class='badge'>severity " << plugin.value("severity", 0) << "</span></div>";
            if (plugin.contains("output")) {
                out << "<pre>" << html_escape(plugin["output"].dump(2)) << "</pre>";
            } else if (plugin.contains("output_raw")) {
                out << "<pre>" << html_escape(plugin.value("output_raw", "")) << "</pre>";
            }
            out << "</div>";
        }
        out << "</div>";
    }

    out << "</div></body></html>";
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

