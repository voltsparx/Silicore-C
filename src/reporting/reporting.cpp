
#include "reporting/reporting.h"

#include "foundation/metadata.h"
#include "interface/symbols.h"
#include "utils/strings.h"
#include "utils/time.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace silicore::reporting {

namespace {

std::string normalize_severity(std::string value) {
    value = utils::trim(value);
    value = utils::to_upper(value);
    if (value == "CRITICAL" || value == "HIGH" || value == "MEDIUM" || value == "LOW" || value == "INFO") {
        return value;
    }
    return "INFO";
}

std::string severity_from_int(int value) {
    if (value >= 9) return "CRITICAL";
    if (value >= 7) return "HIGH";
    if (value >= 4) return "MEDIUM";
    if (value >= 2) return "LOW";
    return "INFO";
}

std::vector<json> safe_dict_rows(const json& value) {
    std::vector<json> rows;
    if (!value.is_array()) {
        return rows;
    }
    for (const auto& item : value) {
        if (item.is_object()) {
            rows.push_back(item);
        }
    }
    return rows;
}

json severity_breakdown(const std::vector<json>& rows) {
    json counts = {
        {"CRITICAL", 0},
        {"HIGH", 0},
        {"MEDIUM", 0},
        {"LOW", 0},
        {"INFO", 0},
    };
    for (const auto& row : rows) {
        std::string severity = normalize_severity(row.value("severity", "INFO"));
        int current = 0;
        if (counts.contains(severity) && counts[severity].is_number_integer()) {
            current = counts[severity].get<int>();
        }
        counts[severity] = current + 1;
    }
    return counts;
}

std::string compact_data_snapshot(const json& data, int max_items) {
    if (!data.is_object() || data.empty()) {
        return "-";
    }
    std::vector<std::string> keys;
    for (auto it = data.begin(); it != data.end(); ++it) {
        keys.push_back(it.key());
    }
    std::sort(keys.begin(), keys.end());
    std::vector<std::string> pairs;
    for (const auto& key : keys) {
        const auto& value = data.at(key);
        std::ostringstream oss;
        if (value.is_string()) {
            oss << key << "=" << value.get<std::string>();
        } else if (value.is_number_integer()) {
            oss << key << "=" << value.get<long long>();
        } else if (value.is_number_float()) {
            oss << key << "=" << value.get<double>();
        } else if (value.is_boolean()) {
            oss << key << "=" << (value.get<bool>() ? "true" : "false");
        } else if (value.is_array()) {
            oss << key << "[" << value.size() << "]";
        } else if (value.is_object()) {
            oss << key << "{" << value.size() << "}";
        } else if (value.is_null()) {
            oss << key << "=null";
        } else {
            oss << key << "=...";
        }
        pairs.push_back(oss.str());
        if (static_cast<int>(pairs.size()) >= max_items) {
            break;
        }
    }
    if (pairs.empty()) {
        return "-";
    }
    std::ostringstream out;
    for (size_t i = 0; i < pairs.size(); ++i) {
        if (i > 0) out << ", ";
        out << pairs[i];
    }
    return out.str();
}

std::string escape_html(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char ch : text) {
        switch (ch) {
            case '&': out.append("&amp;"); break;
            case '<': out.append("&lt;"); break;
            case '>': out.append("&gt;"); break;
            case '"': out.append("&quot;"); break;
            case '\'': out.append("&#x27;"); break;
            default: out.push_back(ch); break;
        }
    }
    return out;
}
std::string crypto_profile_html(const json& data) {
    if (!data.is_object()) {
        return "";
    }
    if (!data.contains("crypto_profile") || !data["crypto_profile"].is_object()) {
        return "";
    }
    const auto& profile = data["crypto_profile"];
    std::string kind = profile.value("crypto_kind", data.value("crypto_kind", "crypto"));
    if (kind.empty()) kind = "crypto";
    std::string operation = profile.value("operation", data.value("operation", "encrypt"));
    if (operation.empty()) operation = "encrypt";
    std::string output_encoding = profile.value("output_encoding", "base64");
    if (output_encoding.empty()) output_encoding = "base64";
    int max_items = 0;
    if (profile.contains("max_items") && profile["max_items"].is_number()) {
        max_items = profile["max_items"].get<int>();
    }
    bool strict_mode = profile.value("strict_mode", false);
    std::string sources = "-";
    if (profile.contains("source_fields") && profile["source_fields"].is_array()) {
        std::vector<std::string> items;
        for (const auto& item : profile["source_fields"]) {
            if (item.is_string()) {
                auto value = utils::trim(item.get<std::string>());
                if (!value.empty()) {
                    items.push_back(value);
                }
            }
        }
        if (!items.empty()) {
            std::ostringstream oss;
            for (size_t i = 0; i < items.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << items[i];
            }
            sources = oss.str();
        }
    }
    std::ostringstream oss;
    oss << "<p><strong>Crypto Config:</strong> "
        << "kind=" << escape_html(kind) << " | "
        << "operation=" << escape_html(operation) << " | "
        << "encoding=" << escape_html(output_encoding) << " | "
        << "max_items=" << escape_html(std::to_string(max_items)) << " | "
        << "strict=" << escape_html(strict_mode ? "true" : "false") << " | "
        << "sources=" << escape_html(sources)
        << "</p>";
    return oss.str();
}

std::vector<std::string> json_string_list(const json& value) {
    std::vector<std::string> out;
    if (!value.is_array()) {
        return out;
    }
    for (const auto& item : value) {
        if (item.is_string()) {
            out.push_back(item.get<std::string>());
        }
    }
    return out;
}

std::string json_as_string(const json& value, const std::string& fallback = "-") {
    if (value.is_null()) {
        return fallback;
    }
    if (value.is_string()) {
        return value.get<std::string>();
    }
    if (value.is_boolean()) {
        return value.get<bool>() ? "true" : "false";
    }
    if (value.is_number_integer()) {
        return std::to_string(value.get<long long>());
    }
    if (value.is_number_float()) {
        std::ostringstream oss;
        oss << value.get<double>();
        return oss.str();
    }
    return value.dump();
}

std::string join_list(const std::vector<std::string>& values, const std::string& delim) {
    if (values.empty()) {
        return "";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << delim;
        oss << values[i];
    }
    return oss.str();
}

std::string render_chip_list(const std::vector<std::string>& values, const std::string& empty_label, int max_items) {
    if (values.empty()) {
        return "<span class='muted'>" + escape_html(empty_label) + "</span>";
    }
    std::ostringstream oss;
    int count = 0;
    for (const auto& value : values) {
        if (count >= max_items) break;
        oss << "<span class='chip'>" << escape_html(value) << "</span>";
        ++count;
    }
    if (static_cast<int>(values.size()) > max_items) {
        oss << "<span class='chip chip-muted'>+" << (values.size() - max_items) << " more</span>";
    }
    return oss.str();
}

std::string status_badge(const std::string& status) {
    std::string color = "#8a8f98";
    if (status == "FOUND") {
        color = "#20d981";
    } else if (status == "BLOCKED" || status == "INVALID_USERNAME") {
        color = "#f5b949";
    } else if (status == "ERROR") {
        color = "#ff6d7a";
    }
    std::ostringstream oss;
    oss << "<span class='badge' style='background:" << color << ";'>"
        << escape_html(status) << "</span>";
    return oss.str();
}

std::string metric_card(const std::string& label, const std::string& value, const std::string& hint) {
    std::ostringstream oss;
    oss << "<div class='metric-card'>"
        << "<div class='metric-label'>" << escape_html(label) << "</div>"
        << "<div class='metric-value'>" << escape_html(value) << "</div>"
        << "<div class='metric-hint'>" << escape_html(hint) << "</div>"
        << "</div>";
    return oss.str();
}

std::string local_timestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto tt = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::vector<domain::ProfileEntity> parse_profile_entities(const json& results) {
    std::vector<domain::ProfileEntity> out;
    if (!results.is_array()) {
        return out;
    }
    for (const auto& row : results) {
        if (!row.is_object()) {
            continue;
        }
        domain::ProfileEntity entity;
        entity.platform = row.value("platform", "");
        entity.status = row.value("status", "");
        entity.profile_url = row.value("url", "");
        entity.context = row.value("context", "");
        entity.bio = row.value("bio", "");
        if (row.contains("confidence") && row["confidence"].is_number()) {
            entity.confidence = static_cast<float>(row["confidence"].get<double>() / 100.0);
        }
        if (row.contains("http_status") && row["http_status"].is_number_integer()) {
            entity.http_status = row["http_status"].get<int>();
        }
        if (row.contains("response_time_ms") && row["response_time_ms"].is_number_integer()) {
            entity.response_time_ms = row["response_time_ms"].get<int>();
        }
        if (row.contains("links")) {
            entity.links = json_string_list(row["links"]);
        }
        if (row.contains("mentions")) {
            entity.mentions = json_string_list(row["mentions"]);
        }
        if (row.contains("contacts") && row["contacts"].is_object()) {
            const auto& contacts = row["contacts"];
            if (contacts.contains("emails")) {
                entity.contacts.emails = json_string_list(contacts["emails"]);
            }
            if (contacts.contains("phones")) {
                entity.contacts.phones = json_string_list(contacts["phones"]);
            }
        }
        out.push_back(std::move(entity));
    }
    return out;
}
json build_plugin_json(const extensions::PluginResult& result) {
    json payload = json::object();
    json data = json::object();
    std::string summary = "No plugin summary.";
    std::vector<std::string> highlights;
    std::string severity = severity_from_int(result.severity);

    json parsed;
    if (!result.output_json.empty()) {
        try {
            parsed = json::parse(result.output_json);
        } catch (const std::exception&) {
            parsed = json::object();
        }
    }

    if (parsed.is_object()) {
        if (parsed.contains("summary") && parsed["summary"].is_string()) {
            summary = parsed["summary"].get<std::string>();
        }
        if (parsed.contains("severity")) {
            if (parsed["severity"].is_string()) {
                severity = normalize_severity(parsed["severity"].get<std::string>());
            } else if (parsed["severity"].is_number_integer()) {
                severity = severity_from_int(parsed["severity"].get<int>());
            }
        }
        if (parsed.contains("highlights") && parsed["highlights"].is_array()) {
            for (const auto& item : parsed["highlights"]) {
                if (item.is_string()) {
                    highlights.push_back(item.get<std::string>());
                }
            }
        }
        if (parsed.contains("data") && parsed["data"].is_object()) {
            data = parsed["data"];
        } else {
            data = parsed;
            data.erase("summary");
            data.erase("severity");
            data.erase("highlights");
        }
    }

    payload["id"] = result.id;
    payload["title"] = result.title.empty() ? result.id : result.title;
    payload["summary"] = summary;
    payload["severity"] = severity;
    payload["highlights"] = highlights;
    payload["data"] = data;
    return payload;
}

json build_filter_json(const extensions::FilterResult& result) {
    json payload = json::object();
    json data = json::object();
    std::string summary = "No filter summary.";
    std::vector<std::string> highlights;
    std::string severity = severity_from_int(result.severity);

    json parsed;
    if (!result.output_json.empty()) {
        try {
            parsed = json::parse(result.output_json);
        } catch (const std::exception&) {
            parsed = json::object();
        }
    }

    if (parsed.is_object()) {
        if (parsed.contains("summary") && parsed["summary"].is_string()) {
            summary = parsed["summary"].get<std::string>();
        }
        if (parsed.contains("severity")) {
            if (parsed["severity"].is_string()) {
                severity = normalize_severity(parsed["severity"].get<std::string>());
            } else if (parsed["severity"].is_number_integer()) {
                severity = severity_from_int(parsed["severity"].get<int>());
            }
        }
        if (parsed.contains("highlights") && parsed["highlights"].is_array()) {
            for (const auto& item : parsed["highlights"]) {
                if (item.is_string()) {
                    highlights.push_back(item.get<std::string>());
                }
            }
        }
        if (parsed.contains("data") && parsed["data"].is_object()) {
            data = parsed["data"];
        } else {
            data = parsed;
            data.erase("summary");
            data.erase("severity");
            data.erase("highlights");
        }
    }

    payload["id"] = result.id;
    payload["title"] = result.title.empty() ? result.id : result.title;
    payload["summary"] = summary;
    payload["severity"] = severity;
    payload["highlights"] = highlights;
    payload["data"] = data;
    return payload;
}

std::string csv_escape(const std::string& value) {
    bool needs_quotes = false;
    for (char ch : value) {
        if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r') {
            needs_quotes = true;
            break;
        }
    }
    if (!needs_quotes) {
        return value;
    }
    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (char ch : value) {
        if (ch == '"') {
            escaped.push_back('"');
            escaped.push_back('"');
        } else {
            escaped.push_back(ch);
        }
    }
    escaped.push_back('"');
    return escaped;
}

void write_csv_file(
    const std::filesystem::path& path,
    const std::vector<std::string>& header,
    const std::vector<std::vector<std::string>>& rows
) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return;
    }
    auto write_row = [&](const std::vector<std::string>& row) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                out << ',';
            }
            out << csv_escape(row[i]);
        }
        out << "\n";
    };
    write_row(header);
    for (const auto& row : rows) {
        write_row(row);
    }
}
std::string render_target_snapshot(const std::string& target, const analyze::TargetSnapshot& snapshot, int total_checks) {
    std::ostringstream bios;
    int count = 0;
    for (const auto& value : snapshot.bios) {
        if (count >= 4) break;
        std::string clipped = value.substr(0, 260);
        bios << "<li>" << escape_html(clipped) << "</li>";
        ++count;
    }
    if (count == 0) {
        bios << "<li>None</li>";
    }

    std::ostringstream oss;
    oss << "<section class='panel'>"
        << "<h3>Target Intelligence Snapshot</h3>"
        << "<p><strong>Target:</strong> " << escape_html(target) << "</p>"
        << "<p><strong>Checks Run:</strong> " << total_checks << " | "
        << "<strong>Found Profiles:</strong> " << snapshot.found_count << " | "
        << "<strong>Errored/Blocked:</strong> " << snapshot.error_count << " | "
        << "<strong>Coverage Ratio:</strong> " << snapshot.coverage_ratio << "</p>"
        << "<p><strong>Avg Found Confidence:</strong> " << snapshot.avg_found_confidence << " | "
        << "<strong>Avg Found RTT:</strong> " << snapshot.avg_found_response_time_ms << " ms | "
        << "<strong>Avg Error RTT:</strong> " << snapshot.avg_error_response_time_ms << " ms</p>"
        << "<p><strong>Status Breakdown:</strong> " << escape_html(json(snapshot.status_breakdown).dump()) << "</p>"
        << "<div class='chip-group'>"
        << "<h4>Found Platforms</h4>"
        << "<div>" << render_chip_list(snapshot.found_platforms, "None", 14) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>Emails</h4>"
        << "<div>" << render_chip_list(snapshot.emails, "None", 14) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>Email Domains</h4>"
        << "<div>" << render_chip_list(snapshot.email_domains, "None", 14) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>Phones</h4>"
        << "<div>" << render_chip_list(snapshot.phones, "None", 14) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>Names</h4>"
        << "<div>" << render_chip_list(snapshot.names, "None", 14) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>Mentions</h4>"
        << "<div>" << render_chip_list(snapshot.mentions, "None", 14) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>External Links</h4>"
        << "<div>" << render_chip_list(snapshot.external_links, "None", 14) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>External Link Domains</h4>"
        << "<div>" << render_chip_list(snapshot.external_link_domains, "None", 14) << "</div>"
        << "</div>"
        << "<h4>Bio Snippets</h4>"
        << "<ul>" << bios.str() << "</ul>"
        << "</section>";
    return oss.str();
}

std::string render_found_profile_table(const std::vector<domain::ProfileEntity>& rows) {
    std::ostringstream rendered;
    for (const auto& item : rows) {
        std::ostringstream links;
        int link_count = 0;
        for (const auto& link : item.links) {
            if (link_count >= 8) break;
            links << "<a href='" << escape_html(link) << "' target='_blank' rel='noreferrer'>"
                  << escape_html(link) << "</a>";
            if (link_count < 7 && link_count + 1 < static_cast<int>(item.links.size())) {
                links << "<br>";
            }
            ++link_count;
        }
        std::string link_block = links.str().empty() ? "-" : links.str();
        std::string bio = escape_html(item.bio.empty() ? "-" : item.bio);
        for (auto& ch : bio) {
            if (ch == '\n') {
                ch = '\r';
            }
        }
        std::string context = escape_html(item.context.empty() ? "-" : item.context);
        std::string profile_url = escape_html(item.profile_url);
        std::string account_link = "<a href='" + profile_url + "' target='_blank' rel='noreferrer'>" + profile_url + "</a>";
        std::ostringstream bio_fixed;
        for (size_t i = 0; i < bio.size(); ++i) {
            if (bio[i] == '\r') {
                bio_fixed << "<br>";
            } else {
                bio_fixed << bio[i];
            }
        }
        rendered << "<tr>"
                 << "<td>" << escape_html(item.platform.empty() ? "Unknown" : item.platform) << "</td>"
                 << "<td>" << static_cast<int>(item.confidence * 100.0f) << "%</td>"
                 << "<td>" << account_link << "</td>"
                 << "<td>" << escape_html(join_list(item.contacts.emails, ", ").empty() ? "-" : join_list(item.contacts.emails, ", ")) << "</td>"
                 << "<td>" << escape_html(join_list(item.contacts.phones, ", ").empty() ? "-" : join_list(item.contacts.phones, ", ")) << "</td>"
                 << "<td>" << escape_html(join_list(item.mentions, ", ").empty() ? "-" : join_list(item.mentions, ", ")) << "</td>"
                 << "<td>" << link_block << "</td>"
                 << "<td>" << bio_fixed.str() << "</td>"
                 << "<td>" << context << "</td>"
                 << "</tr>";
    }
    std::string out = rendered.str();
    if (out.empty()) {
        return "<tr><td colspan='9'>No FOUND profiles in this run.</td></tr>";
    }
    return out;
}

std::string render_error_table(const std::vector<domain::ProfileEntity>& rows) {
    std::ostringstream rendered;
    for (const auto& item : rows) {
        std::string profile_url = escape_html(item.profile_url);
        std::string account_link = "<a href='" + profile_url + "' target='_blank' rel='noreferrer'>" + profile_url + "</a>";
        rendered << "<tr>"
                 << "<td>" << escape_html(item.platform.empty() ? "Unknown" : item.platform) << "</td>"
                 << "<td>" << status_badge(item.status.empty() ? "ERROR" : item.status) << "</td>"
                 << "<td>" << account_link << "</td>"
                 << "<td>" << escape_html(item.http_status == 0 ? "-" : std::to_string(item.http_status)) << "</td>"
                 << "<td>" << escape_html(item.response_time_ms == 0 ? "-" : std::to_string(item.response_time_ms)) << "</td>"
                 << "<td>" << escape_html(item.context.empty() ? "-" : item.context) << "</td>"
                 << "</tr>";
    }
    std::string out = rendered.str();
    if (out.empty()) {
        return "<tr><td colspan='6'>No ERROR/BLOCKED websites in this run.</td></tr>";
    }
    return out;
}
std::string render_correlation(const json& correlation) {
    std::vector<std::string> sections;
    const std::vector<std::pair<std::string, std::string>> mapping = {
        {"shared_bios", "Shared Bios"},
        {"shared_emails", "Shared Emails"},
        {"shared_phones", "Shared Phones"},
        {"shared_links", "Shared Links"},
        {"shared_mentions", "Shared Mentions"},
    };
    for (const auto& entry : mapping) {
        if (!correlation.contains(entry.first) || !correlation[entry.first].is_object()) {
            continue;
        }
        const auto& payload = correlation[entry.first];
        if (payload.empty()) {
            continue;
        }
        std::ostringstream items;
        for (auto it = payload.begin(); it != payload.end(); ++it) {
            std::vector<std::string> platforms = json_string_list(it.value());
            items << "<li><strong>" << escape_html(it.key()) << "</strong> "
                  << "<span class='muted'>-> " << escape_html(join_list(platforms, ", ")) << "</span></li>";
        }
        std::ostringstream block;
        block << "<h4>" << escape_html(entry.second) << "</h4><ul>" << items.str() << "</ul>";
        sections.push_back(block.str());
    }

    if (correlation.contains("confidence_cluster_map") && correlation["confidence_cluster_map"].is_object()) {
        const auto& cluster_map = correlation["confidence_cluster_map"];
        std::string high = join_list(json_string_list(cluster_map.value("high", json::array())), ", ");
        std::string medium = join_list(json_string_list(cluster_map.value("medium", json::array())), ", ");
        std::string low = join_list(json_string_list(cluster_map.value("low", json::array())), ", ");
        std::ostringstream block;
        block << "<h4>Confidence Clusters</h4>"
              << "<ul>"
              << "<li><strong>High:</strong> " << escape_html(high.empty() ? "None" : high) << "</li>"
              << "<li><strong>Medium:</strong> " << escape_html(medium.empty() ? "None" : medium) << "</li>"
              << "<li><strong>Low:</strong> " << escape_html(low.empty() ? "None" : low) << "</li>"
              << "</ul>";
        sections.push_back(block.str());
    }

    if (sections.empty()) {
        return "<p class='muted'>No correlation overlaps were identified.</p>";
    }
    std::ostringstream out;
    for (const auto& section : sections) {
        out << section;
    }
    return out.str();
}

std::string render_domain_section(const json& domain_result) {
    if (!domain_result.is_object() || domain_result.empty()) {
        return "";
    }
    std::vector<std::string> subdomains = json_string_list(domain_result.value("subdomains", json::array()));
    std::ostringstream subdomain_items;
    int count = 0;
    for (const auto& item : subdomains) {
        if (count >= 40) break;
        subdomain_items << "<li>" << escape_html(item) << "</li>";
        ++count;
    }
    if (count == 0) {
        subdomain_items << "<li>None</li>";
    }

    std::vector<std::string> notes = json_string_list(domain_result.value("scan_notes", json::array()));
    std::ostringstream note_items;
    if (notes.empty()) {
        note_items << "<li>None</li>";
    } else {
        for (const auto& note : notes) {
            note_items << "<li>" << escape_html(note) << "</li>";
        }
    }

    json https_data = domain_result.value("https", json::object());
    json http_data = domain_result.value("http", json::object());
    json rdap = domain_result.value("rdap", json::object());
    std::string https_status = json_as_string(https_data.contains("status") ? https_data["status"] : json(nullptr), "-");
    std::string https_final = json_as_string(https_data.contains("final_url") ? https_data["final_url"] : json(nullptr), "");
    std::string http_status = json_as_string(http_data.contains("status") ? http_data["status"] : json(nullptr), "-");
    std::string http_final = json_as_string(http_data.contains("final_url") ? http_data["final_url"] : json(nullptr), "");
    std::string http_redirects = http_data.value("redirects_to_https", false) ? "true" : "false";

    std::string resolved = join_list(json_string_list(domain_result.value("resolved_addresses", json::array())), ", ");
    if (resolved.empty()) {
        resolved = "None";
    }

    std::ostringstream oss;
    oss << "<section class='panel'>"
        << "<h3>Domain Surface Intelligence</h3>"
        << "<p><strong>Target:</strong> " << escape_html(domain_result.value("target", "")) << "</p>"
        << "<p><strong>Resolved Addresses:</strong> " << escape_html(resolved) << "</p>"
        << "<p><strong>HTTPS:</strong> status=" << escape_html(https_status)
        << " final=" << escape_html(https_final) << "</p>"
        << "<p><strong>HTTP:</strong> status=" << escape_html(http_status)
        << " final=" << escape_html(http_final)
        << " redirects_to_https=" << escape_html(http_redirects) << "</p>"
        << "<p><strong>RDAP Handle:</strong> " << escape_html(rdap.value("handle", "-")) << "</p>"
        << "<h4>Subdomain Candidates</h4>"
        << "<ul>" << subdomain_items.str() << "</ul>"
        << "<h4>Collector Notes</h4>"
        << "<ul>" << note_items.str() << "</ul>"
        << "</section>";
    return oss.str();
}
std::string render_issues(const std::vector<json>& issues, const json& issue_summary) {
    if (issues.empty()) {
        return "<p class='muted'>No exposure findings were reported.</p>";
    }
    std::ostringstream rows;
    for (const auto& issue : issues) {
        rows << "<tr>"
             << "<td>" << escape_html(issue.value("severity", "LOW")) << "</td>"
             << "<td>" << escape_html(issue.value("scope", "-")) << "</td>"
             << "<td>" << escape_html(issue.value("title", "-")) << "</td>"
             << "<td>" << escape_html(issue.value("evidence", "-")) << "</td>"
             << "<td>" << escape_html(issue.value("recommendation", "-")) << "</td>"
             << "</tr>";
    }
    std::ostringstream oss;
    oss << "<p><strong>Risk Score:</strong> " << escape_html(std::to_string(issue_summary.value("risk_score", 0))) << "</p>"
        << "<p><strong>Severity Breakdown:</strong> " << escape_html(issue_summary.value("severity_breakdown", json::object()).dump()) << "</p>"
        << "<div class='table-wrap'>"
        << "<table>"
        << "<tr><th>Severity</th><th>Scope</th><th>Title</th><th>Evidence</th><th>Recommendation</th></tr>"
        << rows.str()
        << "</table>"
        << "</div>";
    return oss.str();
}

std::string render_plugins(const std::vector<json>& plugin_results, const std::vector<std::string>& plugin_errors) {
    if (plugin_results.empty() && plugin_errors.empty()) {
        return "<p class='muted'>No plugins were executed for this run.</p>";
    }
    std::ostringstream cards;
    for (const auto& plugin : plugin_results) {
        std::vector<std::string> highlights = json_string_list(plugin.value("highlights", json::array()));
        std::ostringstream highlight_html;
        int count = 0;
        for (const auto& item : highlights) {
            if (count >= 8) break;
            highlight_html << "<li>" << escape_html(item) << "</li>";
            ++count;
        }
        if (count == 0) {
            highlight_html << "<li>None</li>";
        }
        json data_payload = plugin.value("data", json::object());
        std::string payload_preview = compact_data_snapshot(data_payload, 6);
        std::string payload_json = escape_html(data_payload.dump(2));
        std::string crypto_html = crypto_profile_html(data_payload);
        std::string severity = normalize_severity(plugin.value("severity", "INFO"));
        cards << "<div class='subpanel'>"
              << "<h4>" << escape_html(plugin.value("title", plugin.value("id", "Plugin")))
              << " <span class='badge badge-inline'>" << escape_html(severity) << "</span></h4>"
              << "<p>" << escape_html(plugin.value("summary", "")) << "</p>"
              << crypto_html
              << "<p><strong>Data Snapshot:</strong> " << escape_html(payload_preview) << "</p>"
              << "<ul>" << highlight_html.str() << "</ul>"
              << "<details><summary>Raw plugin data payload</summary>"
              << "<pre>" << payload_json << "</pre>"
              << "</details>"
              << "</div>";
    }
    if (!plugin_errors.empty()) {
        cards << "<h4>Plugin Errors</h4><ul>";
        for (const auto& err : plugin_errors) {
            cards << "<li>" << escape_html(err) << "</li>";
        }
        cards << "</ul>";
    }
    return cards.str();
}

std::string render_filters(const std::vector<json>& filter_results, const std::vector<std::string>& filter_errors) {
    if (filter_results.empty() && filter_errors.empty()) {
        return "<p class='muted'>No filters were executed for this run.</p>";
    }
    std::ostringstream cards;
    for (const auto& row : filter_results) {
        std::vector<std::string> highlights = json_string_list(row.value("highlights", json::array()));
        std::ostringstream highlight_html;
        int count = 0;
        for (const auto& item : highlights) {
            if (count >= 8) break;
            highlight_html << "<li>" << escape_html(item) << "</li>";
            ++count;
        }
        if (count == 0) {
            highlight_html << "<li>None</li>";
        }
        json data_payload = row.value("data", json::object());
        std::string payload_preview = compact_data_snapshot(data_payload, 6);
        std::string payload_json = escape_html(data_payload.dump(2));
        std::string severity = normalize_severity(row.value("severity", "INFO"));
        cards << "<div class='subpanel'>"
              << "<h4>" << escape_html(row.value("title", row.value("id", "Filter")))
              << " <span class='badge badge-inline'>" << escape_html(severity) << "</span></h4>"
              << "<p>" << escape_html(row.value("summary", "")) << "</p>"
              << "<p><strong>Data Snapshot:</strong> " << escape_html(payload_preview) << "</p>"
              << "<ul>" << highlight_html.str() << "</ul>"
              << "<details><summary>Raw filter data payload</summary>"
              << "<pre>" << payload_json << "</pre>"
              << "</details>"
              << "</div>";
    }
    if (!filter_errors.empty()) {
        cards << "<h4>Filter Errors</h4><ul>";
        for (const auto& err : filter_errors) {
            cards << "<li>" << escape_html(err) << "</li>";
        }
        cards << "</ul>";
    }
    return cards.str();
}

std::string render_extension_overview(
    const std::vector<json>& issues,
    const json& issue_summary,
    const std::vector<json>& plugin_results,
    const std::vector<std::string>& plugin_errors,
    const std::vector<json>& filter_results,
    const std::vector<std::string>& filter_errors
) {
    auto issue_breakdown = severity_breakdown(issues);
    auto plugin_breakdown = severity_breakdown(plugin_results);
    auto filter_breakdown = severity_breakdown(filter_results);

    std::ostringstream cards;
    cards << metric_card("Risk Score", std::to_string(issue_summary.value("risk_score", 0)), "exposure model")
          << metric_card("Issues", std::to_string(issues.size()), "critical=" + std::to_string(issue_breakdown.value("CRITICAL", 0)))
          << metric_card("Plugins", std::to_string(plugin_results.size()), "errors=" + std::to_string(plugin_errors.size()))
          << metric_card("Filters", std::to_string(filter_results.size()), "errors=" + std::to_string(filter_errors.size()));

    std::ostringstream oss;
    oss << "<section class='panel'>"
        << "<h3>Extension Signal Overview</h3>"
        << "<div class='metrics'>" << cards.str() << "</div>"
        << "<p><strong>Issue Severity:</strong> " << escape_html(issue_breakdown.dump()) << "</p>"
        << "<p><strong>Plugin Severity:</strong> " << escape_html(plugin_breakdown.dump()) << "</p>"
        << "<p><strong>Filter Severity:</strong> " << escape_html(filter_breakdown.dump()) << "</p>"
        << "</section>";
    return oss.str();
}
std::string render_intelligence_bundle(const json& intelligence_bundle) {
    if (!intelligence_bundle.is_object() || intelligence_bundle.empty()) {
        return "<p class='muted'>No intelligence scoring bundle was generated for this run.</p>";
    }

    json metadata = intelligence_bundle.value("metadata", json::object());
    json confidence_distribution = intelligence_bundle.value("confidence_distribution", json::object());
    json risk_summary = intelligence_bundle.value("risk_summary", json::object());
    json facets = intelligence_bundle.value("entity_facets", json::object());
    json scored_entities = intelligence_bundle.value("scored_entities", json::array());
    json correlation_summary = intelligence_bundle.value("correlation_summary", json::object());
    json guidance = intelligence_bundle.value("execution_guidance", json::object());
    json actions = guidance.value("actions", json::array());

    auto scored_contacts = facets.value("scored_contacts", json::array());

    std::ostringstream contact_rows;
    int contact_count = 0;
    for (const auto& item : safe_dict_rows(scored_contacts)) {
        if (contact_count >= 28) break;
        contact_rows << "<tr>"
                     << "<td>" << escape_html(item.value("kind", "-")) << "</td>"
                     << "<td>" << escape_html(item.value("value", "-")) << "</td>"
                     << "<td>" << escape_html(std::to_string(item.value("score_percent", 0))) << "%</td>"
                     << "<td>" << escape_html(std::to_string(item.value("supporting_entities", 0))) << "</td>"
                     << "<td>" << escape_html(item.value("risk_level", "LOW")) << "</td>"
                     << "</tr>";
        ++contact_count;
    }
    if (contact_count == 0) {
        contact_rows << "<tr><td colspan='5'>No contact/name scoring rows.</td></tr>";
    }

    std::ostringstream entity_rows;
    int entity_count = 0;
    for (const auto& item : safe_dict_rows(scored_entities)) {
        if (entity_count >= 34) break;
        entity_rows << "<tr>"
                    << "<td>" << escape_html(std::to_string(item.value("rank", 0))) << "</td>"
                    << "<td>" << escape_html(item.value("entity_type", "-")) << "</td>"
                    << "<td>" << escape_html(item.value("value", "-")) << "</td>"
                    << "<td>" << escape_html(item.value("source", "-")) << "</td>"
                    << "<td>" << escape_html(std::to_string(item.value("confidence_percent", 0))) << "%</td>"
                    << "<td>" << escape_html(item.value("risk_level", "LOW")) << "</td>"
                    << "<td>" << escape_html(std::to_string(item.value("relationship_count", 0))) << "</td>"
                    << "</tr>";
        ++entity_count;
    }
    if (entity_count == 0) {
        entity_rows << "<tr><td colspan='7'>No scored entities.</td></tr>";
    }

    json reason_breakdown = correlation_summary.value("reason_breakdown", json::object());
    std::ostringstream reason_items;
    int reason_count = 0;
    for (auto it = reason_breakdown.begin(); it != reason_breakdown.end() && reason_count < 12; ++it, ++reason_count) {
        reason_items << "<li><strong>" << escape_html(it.key()) << "</strong>: "
                     << escape_html(it.value().dump()) << "</li>";
    }
    if (reason_count == 0) {
        reason_items << "<li>None</li>";
    }

    std::ostringstream guidance_items;
    int action_count = 0;
    for (const auto& item : safe_dict_rows(actions)) {
        if (action_count >= 8) break;
        guidance_items << "<li>[" << escape_html(item.value("priority", "P3")) << "] "
                       << escape_html(item.value("title", "Action"))
                       << "<br><span class='muted'>" << escape_html(item.value("rationale", "-")) << "</span>"
                       << "<br><span class='muted'>Hint: " << escape_html(item.value("command_hint", "-")) << "</span>"
                       << "</li>";
        ++action_count;
    }
    if (action_count == 0) {
        guidance_items << "<li>None</li>";
    }

    std::ostringstream oss;
    oss << "<p><strong>Entities:</strong> " << escape_html(std::to_string(metadata.value("entity_count", 0)))
        << " | <strong>Evidence:</strong> " << escape_html(std::to_string(metadata.value("evidence_count", 0)))
        << " | <strong>Links:</strong> " << escape_html(std::to_string(correlation_summary.value("link_count", 0))) << "</p>"
        << "<p><strong>Confidence Distribution:</strong> "
        << "high=" << escape_html(std::to_string(confidence_distribution.value("high", 0))) << " "
        << "medium=" << escape_html(std::to_string(confidence_distribution.value("medium", 0))) << " "
        << "low=" << escape_html(std::to_string(confidence_distribution.value("low", 0))) << "</p>"
        << "<p><strong>Risk Summary:</strong> " << escape_html(risk_summary.dump()) << "</p>"
        << "<div class='chip-group'>"
        << "<h4>Emails</h4>"
        << "<div>" << render_chip_list(json_string_list(facets.value("emails", json::array())), "None", 18) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>Phones</h4>"
        << "<div>" << render_chip_list(json_string_list(facets.value("phones", json::array())), "None", 18) << "</div>"
        << "</div>"
        << "<div class='chip-group'>"
        << "<h4>Names</h4>"
        << "<div>" << render_chip_list(json_string_list(facets.value("names", json::array())), "None", 18) << "</div>"
        << "</div>"
        << "<h4>Top Contact / Name Signals</h4>"
        << "<div class='table-wrap'>"
        << "<table>"
        << "<tr><th>Kind</th><th>Value</th><th>Score</th><th>Support</th><th>Risk</th></tr>"
        << contact_rows.str()
        << "</table>"
        << "</div>"
        << "<h4>Top Scored Entities</h4>"
        << "<div class='table-wrap'>"
        << "<table>"
        << "<tr><th>Rank</th><th>Type</th><th>Value</th><th>Source</th><th>Confidence</th><th>Risk</th><th>Links</th></tr>"
        << entity_rows.str()
        << "</table>"
        << "</div>"
        << "<h4>Correlation Reasons</h4>"
        << "<ul>" << reason_items.str() << "</ul>"
        << "<h4>Explainable Guidance</h4>"
        << "<ul>" << guidance_items.str() << "</ul>";
    return oss.str();
}

json build_payload_summary(const json& payload) {
    auto results = safe_dict_rows(payload.value("results", json::array()));
    std::vector<json> found;
    std::vector<json> errors;
    for (const auto& row : results) {
        std::string status = row.value("status", "");
        if (status == "FOUND") {
            found.push_back(row);
        } else if (status == "ERROR" || status == "BLOCKED") {
            errors.push_back(row);
        }
    }
    auto issues = safe_dict_rows(payload.value("issues", json::array()));
    auto plugins = safe_dict_rows(payload.value("plugins", json::array()));
    auto filters = safe_dict_rows(payload.value("filters", json::array()));

    json summary;
    summary["result_count"] = static_cast<int>(results.size());
    summary["found_count"] = static_cast<int>(found.size());
    summary["error_or_blocked_count"] = static_cast<int>(errors.size());
    summary["issue_count"] = static_cast<int>(issues.size());
    summary["plugin_count"] = static_cast<int>(plugins.size());
    summary["filter_count"] = static_cast<int>(filters.size());
    summary["plugin_error_count"] = static_cast<int>(payload.value("plugin_errors", json::array()).size());
    summary["filter_error_count"] = static_cast<int>(payload.value("filter_errors", json::array()).size());
    summary["issue_severity"] = severity_breakdown(issues);
    summary["plugin_severity"] = severity_breakdown(plugins);
    summary["filter_severity"] = severity_breakdown(filters);
    return summary;
}

} // namespace
json build_profile_results_json(const std::vector<domain::ProfileEntity>& profiles) {
    json results = json::array();
    for (const auto& profile : profiles) {
        json row;
        row["platform"] = profile.platform;
        row["status"] = profile.status;
        row["confidence"] = static_cast<int>(std::lround(profile.confidence * 100.0f));
        if (profile.http_status > 0) {
            row["http_status"] = profile.http_status;
        } else {
            row["http_status"] = nullptr;
        }
        if (profile.response_time_ms > 0) {
            row["response_time_ms"] = profile.response_time_ms;
        } else {
            row["response_time_ms"] = nullptr;
        }
        row["url"] = profile.profile_url;
        row["context"] = profile.context;
        row["bio"] = profile.bio;
        row["links"] = profile.links;
        row["mentions"] = profile.mentions;
        row["contacts"] = {
            {"emails", profile.contacts.emails},
            {"phones", profile.contacts.phones},
        };
        results.push_back(row);
    }
    return results;
}

json build_domain_result_json(const collect::DomainScanResult& result) {
    json domain;
    domain["target"] = result.target_domain;
    domain["resolved_addresses"] = result.resolved_addresses;
    domain["https"] = {
        {"status", result.https.status},
        {"final_url", result.https.final_url},
        {"elapsed_ms", result.https.elapsed_ms},
        {"headers", result.https.headers},
    };
    domain["http"] = {
        {"status", result.http.status},
        {"final_url", result.http.final_url},
        {"elapsed_ms", result.http.elapsed_ms},
        {"redirects_to_https", result.http_redirects_to_https},
        {"headers", result.http.headers},
    };
    domain["subdomains"] = result.subdomains;
    domain["rdap"] = {
        {"handle", result.rdap.handle},
        {"registrar", result.rdap.registrar},
        {"name_servers", result.rdap.name_servers},
    };
    domain["robots_txt_present"] = result.robots_txt_present;
    domain["robots_preview"] = result.robots_preview;
    domain["security_txt_present"] = result.security_txt_present;
    domain["security_preview"] = result.security_preview;
    domain["scan_notes"] = result.scan_notes;
    return domain;
}

json build_report_payload(const ReportInputs& input) {
    std::string target = utils::trim(input.target);
    std::string target_key = sanitize_target(target);
    if (target.empty()) {
        target = target_key;
    }

    json payload;
    payload["metadata"] = {
        {"generated_at_utc", utils::utc_timestamp()},
        {"mode", input.mode},
        {"framework", foundation::framework_signature()},
    };
    payload["target"] = target;
    payload["target_key"] = target_key;
    payload["results"] = build_profile_results_json(input.profiles);
    if (input.domain_result) {
        payload["domain_result"] = build_domain_result_json(*input.domain_result);
    } else {
        payload["domain_result"] = nullptr;
    }

    json correlation;
    correlation["shared_bios"] = input.correlation.shared_bios;
    correlation["shared_emails"] = input.correlation.shared_emails;
    correlation["shared_phones"] = input.correlation.shared_phones;
    correlation["shared_links"] = input.correlation.shared_links;
    correlation["shared_mentions"] = input.correlation.shared_mentions;
    correlation["confidence_clusters"] = input.correlation.confidence_clusters;
    correlation["confidence_cluster_map"] = input.correlation.confidence_cluster_map;
    correlation["status_distribution"] = input.correlation.status_distribution;
    correlation["response_time_stats"] = {
        {"min_ms", input.correlation.response_time_stats.min_ms >= 0 ? json(input.correlation.response_time_stats.min_ms) : json(nullptr)},
        {"max_ms", input.correlation.response_time_stats.max_ms >= 0 ? json(input.correlation.response_time_stats.max_ms) : json(nullptr)},
        {"avg_ms", input.correlation.response_time_stats.avg_ms >= 0 ? json(input.correlation.response_time_stats.avg_ms) : json(nullptr)},
    };
    correlation["average_confidence"] = input.correlation.average_confidence;
    correlation["identity_overlap_score"] = input.correlation.identity_overlap_score;
    payload["correlation"] = correlation;

    json issues = json::array();
    for (const auto& issue : input.issues) {
        issues.push_back({
            {"scope", issue.scope},
            {"severity", issue.severity},
            {"title", issue.title},
            {"evidence", issue.evidence},
            {"recommendation", issue.recommendation},
        });
    }
    payload["issues"] = issues;

    json issue_summary;
    issue_summary["total"] = input.issue_summary.total;
    issue_summary["severity_breakdown"] = input.issue_summary.severity_breakdown;
    issue_summary["risk_score"] = input.issue_summary.risk_score;
    payload["issue_summary"] = issue_summary;

    json plugins = json::array();
    for (const auto& plugin : input.plugins) {
        plugins.push_back(build_plugin_json(plugin));
    }
    payload["plugins"] = plugins;
    payload["plugin_errors"] = input.plugin_errors;

    json filters = json::array();
    for (const auto& filter : input.filters) {
        filters.push_back(build_filter_json(filter));
    }
    payload["filters"] = filters;
    payload["filter_errors"] = input.filter_errors;

    payload["fused_intel"] = input.fused_intel.is_null() ? json::object() : input.fused_intel;
    payload["fusion_graph"] = input.fusion_graph.is_null() ? json::object() : input.fusion_graph;
    payload["intelligence_bundle"] = input.intelligence_bundle.is_null() ? json::object() : input.intelligence_bundle;
    payload["narrative"] = input.narrative;

    payload["summary"] = build_payload_summary(payload);
    return payload;
}
std::string render_cli_report(const json& payload) {
    json metadata = payload.value("metadata", json::object());
    json summary = payload.value("summary", json::object());
    std::vector<std::string> lines;

    lines.push_back("Framework: " + metadata.value("framework", "-"));
    lines.push_back("Generated UTC: " + metadata.value("generated_at_utc", "-"));
    lines.push_back("Mode: " + metadata.value("mode", "-"));
    lines.push_back("Target: " + payload.value("target", "-"));
    if (payload.contains("target_key")) {
        lines.push_back("Storage Key: " + payload.value("target_key", ""));
    }
    lines.push_back("");

    if (!summary.empty()) {
        lines.push_back("[Artifact Summary]");
        lines.push_back(
            "- results=" + std::to_string(summary.value("result_count", 0)) +
            " found=" + std::to_string(summary.value("found_count", 0)) +
            " errors=" + std::to_string(summary.value("error_or_blocked_count", 0))
        );
        lines.push_back(
            "- issues=" + std::to_string(summary.value("issue_count", 0)) +
            " plugins=" + std::to_string(summary.value("plugin_count", 0)) +
            " filters=" + std::to_string(summary.value("filter_count", 0))
        );
        lines.push_back("- issue_severity=" + summary.value("issue_severity", json::object()).dump());
        lines.push_back("- plugin_severity=" + summary.value("plugin_severity", json::object()).dump());
        lines.push_back("- filter_severity=" + summary.value("filter_severity", json::object()).dump());
        lines.push_back("");
    }

    auto results = parse_profile_entities(payload.value("results", json::array()));
    auto snapshot = analyze::summarize_target_intel(results);
    auto found_rows = analyze::found_profile_rows(results);
    auto error_rows = analyze::error_profile_rows(results);

    lines.push_back("[Found Social Profiles]");
    if (found_rows.empty()) {
        lines.push_back("- none");
    } else {
        for (const auto& row : found_rows) {
            lines.push_back(
                "- " + (row.platform.empty() ? std::string("Unknown") : row.platform) +
                ": FOUND (" + std::to_string(static_cast<int>(row.confidence * 100.0f)) + "%) -> " + row.profile_url
            );
        }
    }
    lines.push_back("");

    lines.push_back("[Errored / Blocked Websites]");
    if (error_rows.empty()) {
        lines.push_back("- none");
    } else {
        for (const auto& row : error_rows) {
            lines.push_back(
                "- " + (row.platform.empty() ? std::string("Unknown") : row.platform) + ": " + row.status +
                " http=" + (row.http_status == 0 ? std::string("-") : std::to_string(row.http_status)) +
                " reason=" + (row.context.empty() ? std::string("-") : row.context)
            );
        }
    }
    lines.push_back("");

    lines.push_back("[Target Intelligence Snapshot]");
    lines.push_back("- total_results: " + std::to_string(snapshot.total_results));
    lines.push_back("- coverage_ratio: " + std::to_string(snapshot.coverage_ratio));
    lines.push_back("- avg_found_confidence: " + std::to_string(snapshot.avg_found_confidence));
    lines.push_back("- avg_found_response_time_ms: " + std::to_string(snapshot.avg_found_response_time_ms));
    lines.push_back("- avg_error_response_time_ms: " + std::to_string(snapshot.avg_error_response_time_ms));
    lines.push_back("- status_breakdown: " + json(snapshot.status_breakdown).dump());
    lines.push_back("- found_platforms: " + (snapshot.found_platforms.empty() ? std::string("none") : join_list(snapshot.found_platforms, ", ")));
    lines.push_back("- profile_links: " + (snapshot.profile_links.empty() ? std::string("none") : join_list(snapshot.profile_links, ", ")));
    lines.push_back("- emails: " + (snapshot.emails.empty() ? std::string("none") : join_list(snapshot.emails, ", ")));
    lines.push_back("- email_domains: " + (snapshot.email_domains.empty() ? std::string("none") : join_list(snapshot.email_domains, ", ")));
    lines.push_back("- phones: " + (snapshot.phones.empty() ? std::string("none") : join_list(snapshot.phones, ", ")));
    lines.push_back("- names: " + (snapshot.names.empty() ? std::string("none") : join_list(snapshot.names, ", ")));
    lines.push_back("- mentions: " + (snapshot.mentions.empty() ? std::string("none") : join_list(snapshot.mentions, ", ")));
    lines.push_back("- external_links: " + (snapshot.external_links.empty() ? std::string("none") : join_list(snapshot.external_links, ", ")));
    lines.push_back("- external_link_domains: " + (snapshot.external_link_domains.empty() ? std::string("none") : join_list(snapshot.external_link_domains, ", ")));
    lines.push_back("");

    json domain_result = payload.value("domain_result", json::object());
    if (domain_result.is_object() && !domain_result.empty()) {
        lines.push_back("[Domain Surface]");
        lines.push_back("- target: " + domain_result.value("target", "-"));
        lines.push_back("- resolved_addresses: " + join_list(json_string_list(domain_result.value("resolved_addresses", json::array())), ", "));
        lines.push_back("- subdomains: " + std::to_string(domain_result.value("subdomains", json::array()).size()));
        lines.push_back("");
    }

    json correlation = payload.value("correlation", json::object());
    if (correlation.is_object() && !correlation.empty()) {
        lines.push_back("[Correlation]");
        lines.push_back("- identity_overlap_score: " + std::to_string(correlation.value("identity_overlap_score", 0)));
        lines.push_back("- shared_links: " + std::to_string(correlation.value("shared_links", json::object()).size()));
        lines.push_back("- shared_emails: " + std::to_string(correlation.value("shared_emails", json::object()).size()));
        lines.push_back("- shared_phones: " + std::to_string(correlation.value("shared_phones", json::object()).size()));
        lines.push_back("");
    }

    auto issues = safe_dict_rows(payload.value("issues", json::array()));
    lines.push_back("[Exposure Findings]");
    if (issues.empty()) {
        lines.push_back("- none");
    } else {
        for (const auto& issue : issues) {
            lines.push_back(
                "- [" + issue.value("severity", "LOW") + "] " + issue.value("title", "Issue") +
                " (scope=" + issue.value("scope", "-") + ") evidence=" + issue.value("evidence", "-")
            );
        }
    }
    lines.push_back("");

    auto plugin_rows = safe_dict_rows(payload.value("plugins", json::array()));
    auto plugin_errors = json_string_list(payload.value("plugin_errors", json::array()));
    lines.push_back("[Plugin Intelligence]");
    if (plugin_rows.empty() && plugin_errors.empty()) {
        lines.push_back("- none");
    } else {
        for (const auto& row : plugin_rows) {
            lines.push_back(
                "- [" + normalize_severity(row.value("severity", "INFO")) + "] " +
                row.value("title", row.value("id", "plugin")) + ": " + row.value("summary", "") +
                " (data: " + compact_data_snapshot(row.value("data", json::object()), 4) + ")"
            );
        }
        for (const auto& err : plugin_errors) {
            lines.push_back("- " + interface::symbol("error") + " " + err);
        }
    }
    lines.push_back("");

    auto filter_rows = safe_dict_rows(payload.value("filters", json::array()));
    auto filter_errors = json_string_list(payload.value("filter_errors", json::array()));
    lines.push_back("[Filter Intelligence]");
    if (filter_rows.empty() && filter_errors.empty()) {
        lines.push_back("- none");
    } else {
        for (const auto& row : filter_rows) {
            lines.push_back(
                "- [" + normalize_severity(row.value("severity", "INFO")) + "] " +
                row.value("title", row.value("id", "filter")) + ": " + row.value("summary", "") +
                " (data: " + compact_data_snapshot(row.value("data", json::object()), 4) + ")"
            );
        }
        for (const auto& err : filter_errors) {
            lines.push_back("- " + interface::symbol("error") + " " + err);
        }
    }
    lines.push_back("");
    json fused_intel = payload.value("fused_intel", json::object());
    json fusion_graph = payload.value("fusion_graph", json::object());
    if (fused_intel.is_object() && !fused_intel.empty()) {
        lines.push_back("[Fusion Intelligence]");
        lines.push_back("- confidence_score: " + json_as_string(fused_intel.contains("confidence_score") ? fused_intel["confidence_score"] : json(nullptr), "-"));
        lines.push_back("- anomalies: " + join_list(json_string_list(fused_intel.value("anomalies", json::array())), ", "));
        if (fused_intel.contains("risk") && fused_intel["risk"].is_object()) {
            lines.push_back("- risk_score: " + json_as_string(fused_intel["risk"].contains("risk_score") ? fused_intel["risk"]["risk_score"] : json(nullptr), "-"));
        }
        lines.push_back(
            "- graph_nodes: " + std::to_string(fusion_graph.value("nodes", json::array()).size()) +
            " graph_edges: " + std::to_string(fusion_graph.value("edges", json::array()).size())
        );
        lines.push_back("");
    }

    json intelligence_bundle = payload.value("intelligence_bundle", json::object());
    if (intelligence_bundle.is_object() && !intelligence_bundle.empty()) {
        json metadata = intelligence_bundle.value("metadata", json::object());
        json facets = intelligence_bundle.value("entity_facets", json::object());
        json confidence_distribution = intelligence_bundle.value("confidence_distribution", json::object());
        json risk_summary = intelligence_bundle.value("risk_summary", json::object());
        json scored_entities = intelligence_bundle.value("scored_entities", json::array());
        json guidance = intelligence_bundle.value("execution_guidance", json::object());

        lines.push_back("[Intelligence Scoring]");
        lines.push_back(
            "- entities: " + std::to_string(metadata.value("entity_count", 0)) +
            " evidence: " + std::to_string(metadata.value("evidence_count", 0))
        );
        lines.push_back(
            "- confidence_distribution: high=" + std::to_string(confidence_distribution.value("high", 0)) +
            " medium=" + std::to_string(confidence_distribution.value("medium", 0)) +
            " low=" + std::to_string(confidence_distribution.value("low", 0))
        );
        lines.push_back("- risk_summary: " + risk_summary.dump());
        lines.push_back("- emails: " + join_list(json_string_list(facets.value("emails", json::array())), ", "));
        lines.push_back("- phones: " + join_list(json_string_list(facets.value("phones", json::array())), ", "));
        lines.push_back("- names: " + join_list(json_string_list(facets.value("names", json::array())), ", "));
        if (!scored_entities.empty()) {
            lines.push_back("- top_scored_entities:");
            int limit = 0;
            for (const auto& row : safe_dict_rows(scored_entities)) {
                if (limit++ >= 12) break;
                lines.push_back(
                    "  - " + row.value("entity_type", "-") + " " + row.value("value", "-") +
                    " (" + std::to_string(row.value("confidence_percent", 0)) + "%, risk=" +
                    row.value("risk_level", "-") + ")"
                );
            }
        }
        if (guidance.contains("actions") && guidance["actions"].is_array()) {
            lines.push_back("- guidance:");
            int limit = 0;
            for (const auto& item : safe_dict_rows(guidance["actions"])) {
                if (limit++ >= 6) break;
                lines.push_back(
                    "  - [" + item.value("priority", "P3") + "] " + item.value("title", "Action") +
                    ": " + item.value("rationale", "-")
                );
                lines.push_back("    hint=" + item.value("command_hint", "-"));
            }
        }
        lines.push_back("");
    }

    std::string narrative = utils::trim(payload.value("narrative", ""));
    lines.push_back("[Nano AI Brief]");
    lines.push_back("- " + (narrative.empty() ? std::string("No narrative generated.") : narrative));
    lines.push_back("");

    std::ostringstream out;
    for (size_t i = 0; i < lines.size(); ++i) {
        out << lines[i];
        if (i + 1 < lines.size()) {
            out << "\n";
        }
    }
    return out.str() + "\n";
}
std::string render_html_report(const json& payload) {
    std::string display_target = utils::trim(payload.value("target", ""));
    std::string target_display = display_target.empty() ? sanitize_target(payload.value("target_key", "target")) : display_target;

    auto results = parse_profile_entities(payload.value("results", json::array()));
    auto found_rows = analyze::found_profile_rows(results);
    auto error_rows = analyze::error_profile_rows(results);
    auto focus_rows = analyze::focused_profile_rows(results);
    auto snapshot = analyze::summarize_target_intel(results);

    json correlation = payload.value("correlation", json::object());
    auto issues = safe_dict_rows(payload.value("issues", json::array()));
    json issue_summary = payload.value("issue_summary", json::object());
    auto plugin_results = safe_dict_rows(payload.value("plugins", json::array()));
    auto plugin_errors = json_string_list(payload.value("plugin_errors", json::array()));
    auto filter_results = safe_dict_rows(payload.value("filters", json::array()));
    auto filter_errors = json_string_list(payload.value("filter_errors", json::array()));
    json intelligence_bundle = payload.value("intelligence_bundle", json::object());
    std::string narrative = payload.value("narrative", "");

    int overlap_score = correlation.value("identity_overlap_score", 0);
    std::ostringstream metrics;
    metrics << metric_card("Mode", utils::to_upper(payload.value("metadata", json::object()).value("mode", "")), "workflow")
            << metric_card("Target", target_display, "entity")
            << metric_card("Platforms Checked", std::to_string(results.size()), "total websites queried")
            << metric_card("Found Profiles", std::to_string(found_rows.size()), "confirmed social profiles")
            << metric_card("Errors/Blocked", std::to_string(error_rows.size()), "sites requiring retry")
            << metric_card("Visible Rows", std::to_string(focus_rows.size()), "found + error rows")
            << metric_card("Overlap Score", std::to_string(overlap_score), "identity correlation")
            << metric_card("Risk Score", std::to_string(issue_summary.value("risk_score", 0)), "exposure signal");

    std::ostringstream html;
    html << R"HTML(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>)HTML" << escape_html(foundation::PROJECT_NAME) << " v" << escape_html(foundation::VERSION)
         << " Report - " << escape_html(target_display) << R"HTML(</title>
      <style>
        :root {
          --bg:#070b12;
          --panel:#101927;
          --panel-2:#162438;
          --text:#edf3fb;
          --muted:#a0b0c2;
          --accent:#27d89a;
          --accent-2:#5ea9ff;
          --line:#2c4258;
          --shadow:0 14px 40px rgba(0, 0, 0, 0.42);
        }
        * { box-sizing: border-box; }
        body {
          margin:0;
          font-family: "Avenir Next", "Trebuchet MS", "Segoe UI", sans-serif;
          color: var(--text);
          background:
            radial-gradient(circle at 12% -6%, rgba(39,216,154,0.24) 0%, rgba(39,216,154,0) 35%),
            radial-gradient(circle at 88% -15%, rgba(94,169,255,0.24) 0%, rgba(94,169,255,0) 40%),
            linear-gradient(145deg, #070b12 0%, #0a111a 45%, #070b12 100%);
          min-height: 100vh;
          padding: 20px;
        }
        .shell { max-width: 1300px; margin: 0 auto; }
        .header {
          background: linear-gradient(130deg, rgba(39,216,154,0.17), rgba(94,169,255,0.14));
          border: 1px solid var(--line);
          border-radius: 18px;
          padding: 20px 22px;
          margin-bottom: 16px;
          box-shadow: var(--shadow);
          backdrop-filter: blur(6px);
        }
        .quick-nav {
          display: flex;
          flex-wrap: wrap;
          gap: 8px;
          margin-bottom: 14px;
        }
        .quick-nav a {
          border: 1px solid var(--line);
          border-radius: 999px;
          padding: 6px 12px;
          background: rgba(255,255,255,0.03);
          color: var(--text);
          font-size: 0.82rem;
          text-decoration: none;
        }
        .quick-nav a:hover {
          border-color: var(--accent-2);
          transform: translateY(-1px);
        }
        .header h1 { margin: 0 0 8px 0; font-size: 1.75rem; }
        .muted { color: var(--muted); }
        .metrics {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(170px, 1fr));
          gap: 12px;
          margin-bottom: 16px;
        }
        .metric-card {
          background: linear-gradient(180deg, rgba(255,255,255,0.05), rgba(255,255,255,0.02));
          border: 1px solid var(--line);
          border-radius: 14px;
          padding: 12px;
          box-shadow: inset 0 1px 0 rgba(255,255,255,0.06);
        }
        .metric-label {
          color: var(--muted);
          font-size: 0.76rem;
          text-transform: uppercase;
          letter-spacing: 0.08em;
        }
        .metric-value { font-size: 1.34rem; font-weight: 800; margin-top: 4px; }
        .metric-hint { color: var(--muted); font-size: 0.78rem; margin-top: 4px; }
        .panel {
          background: linear-gradient(180deg, rgba(18,31,48,0.88), rgba(16,25,39,0.95));
          border: 1px solid var(--line);
          border-radius: 14px;
          padding: 14px;
          margin-bottom: 14px;
          box-shadow: var(--shadow);
        }
        .subpanel {
          background: rgba(255,255,255,0.02);
          border: 1px solid rgba(255,255,255,0.07);
          border-radius: 10px;
          padding: 10px 12px;
          margin-bottom: 10px;
        }
        .badge {
          display: inline-block;
          padding: 4px 10px;
          border-radius: 999px;
          color: #0d1117;
          font-weight: 700;
          letter-spacing: 0.03em;
        }
        .badge-inline {
          background: rgba(94,169,255,0.22);
          border: 1px solid rgba(94,169,255,0.45);
          color: var(--text);
          margin-left: 8px;
          padding: 2px 8px;
          font-size: 0.74rem;
          vertical-align: middle;
        }
        .chip-group { margin-top: 12px; }
        .chip-group h4 { margin: 0 0 6px 0; }
        .chip {
          display: inline-block;
          border: 1px solid rgba(255,255,255,0.16);
          border-radius: 999px;
          padding: 4px 9px;
          margin: 4px 6px 0 0;
          font-size: 0.82rem;
          background: rgba(255,255,255,0.03);
          color: var(--text);
        }
        .chip-muted { color: var(--muted); }
        .table-wrap { overflow-x: auto; border-radius: 10px; }
        table { width: 100%; border-collapse: collapse; min-width: 980px; }
        th, td {
          border-bottom: 1px solid var(--line);
          padding: 9px;
          text-align: left;
          vertical-align: top;
          font-size: 0.92rem;
        }
        th {
          color: #c4d3e4;
          background: rgba(255,255,255,0.03);
          position: sticky;
          top: 0;
        }
        a { color: var(--accent-2); text-decoration: none; }
        a:hover { text-decoration: underline; }
        ul { padding-left: 20px; }
        details {
          margin-top: 8px;
          border: 1px solid rgba(255,255,255,0.1);
          border-radius: 8px;
          padding: 6px 8px;
          background: rgba(7,11,18,0.38);
        }
        summary {
          cursor: pointer;
          color: #c7dbef;
          font-weight: 600;
        }
        pre {
          white-space: pre-wrap;
          word-break: break-word;
          max-height: 240px;
          overflow: auto;
          background: rgba(0, 0, 0, 0.35);
          border: 1px solid rgba(255,255,255,0.12);
          border-radius: 8px;
          padding: 8px;
          color: #d6e4f3;
          font-size: 0.8rem;
        }
        .brief {
          background: rgba(39,216,154,0.09);
          border: 1px solid rgba(39,216,154,0.38);
          border-left: 4px solid var(--accent);
          border-radius: 8px;
          padding: 10px 12px;
        }
        footer { margin-top: 16px; color: var(--muted); font-size: 0.84rem; }
        @media (max-width: 760px) {
          body { padding: 12px; }
          .header h1 { font-size: 1.35rem; }
          table { min-width: 760px; }
        }
      </style>
    </head>
    <body>
      <div class="shell">
        <div class="header">
          <h1>)HTML" << escape_html(foundation::PROJECT_NAME) << " v" << escape_html(foundation::VERSION)
         << " Intelligence Report</h1>"
         << "<div class='muted'><strong>Target:</strong> " << escape_html(target_display)
         << " | <strong>Generated:</strong> " << local_timestamp()
         << " | <strong>Framework:</strong> " << escape_html(foundation::framework_signature()) << "</div>"
         << "</div>"
         << R"HTML(

        <div class="quick-nav">
          <a href="#overview">Overview</a>
          <a href="#profiles">Profiles</a>
          <a href="#errors">Errors</a>
          <a href="#correlation">Correlation</a>
          <a href="#exposure">Exposure</a>
          <a href="#plugins">Plugins</a>
          <a href="#filters">Filters</a>
          <a href="#intelligence">Intelligence</a>
        </div>

        <div class="metrics">)HTML" << metrics.str() << R"HTML(</div>

        <div id="overview">)HTML" << render_target_snapshot(target_display, snapshot, static_cast<int>(results.size())) << R"HTML(</div>

        )HTML" << render_extension_overview(issues, issue_summary, plugin_results, plugin_errors, filter_results, filter_errors) << R"HTML(

        <section class="panel" id="profiles">
          <h3>Found Social Media Profiles</h3>
          <div class="table-wrap">
            <table>
              <tr>
                <th>Platform</th><th>Confidence</th><th>Profile Link</th><th>Emails</th><th>Phones</th>
                <th>Mentions</th><th>Extracted Links</th><th>Bio</th><th>Context</th>
              </tr>
              )HTML" << render_found_profile_table(found_rows) << R"HTML(
            </table>
          </div>
        </section>

        <section class="panel" id="errors">
          <h3>Errored / Blocked Websites</h3>
          <div class="table-wrap">
            <table>
              <tr><th>Platform</th><th>Status</th><th>Profile Link</th><th>HTTP</th><th>RTT (ms)</th><th>Reason</th></tr>
              )HTML" << render_error_table(error_rows) << R"HTML(
            </table>
          </div>
        </section>

        <section class="panel" id="correlation">
          <h3>Correlation Engine</h3>
          )HTML" << render_correlation(correlation) << R"HTML(
        </section>

        )HTML" << render_domain_section(payload.value("domain_result", json::object())) << R"HTML(

        <section class="panel" id="exposure">
          <h3>Exposure & Vulnerability Signals</h3>
          )HTML" << render_issues(issues, issue_summary) << R"HTML(
        </section>

        <section class="panel" id="plugins">
          <h3>Plugin Intelligence</h3>
          )HTML" << render_plugins(plugin_results, plugin_errors) << R"HTML(
        </section>

        <section class="panel" id="filters">
          <h3>Filter Intelligence</h3>
          )HTML" << render_filters(filter_results, filter_errors) << R"HTML(
        </section>

        <section class="panel" id="intelligence">
          <h3>Intelligence Scoring & Guidance</h3>
          )HTML" << render_intelligence_bundle(intelligence_bundle) << R"HTML(
        </section>

        <section class="panel">
          <h3>Nano AI Narrative</h3>
          <div class="brief">)HTML" << escape_html(narrative.empty() ? std::string("No narrative generated for this run.") : narrative) << R"HTML(</div>
        </section>

        <footer>
          Generated by )HTML" << escape_html(foundation::PROJECT_NAME) << " v" << escape_html(foundation::VERSION)
         << " | Developed by " << escape_html(foundation::AUTHOR) << R"HTML(
        </footer>
      </div>
    </body>
    </html>
    )HTML";

    return html.str();
}
std::string render_csv_report(const json& payload) {
    json metadata = payload.value("metadata", json::object());
    std::string mode = utils::trim(metadata.value("mode", ""));
    std::string target_name = utils::trim(payload.value("target", ""));
    if (target_name.empty()) {
        target_name = payload.value("target_key", "target");
    }
    std::string risk_score = "";
    if (payload.contains("issue_summary") && payload["issue_summary"].is_object()) {
        risk_score = std::to_string(payload["issue_summary"].value("risk_score", 0));
    }

    std::ostringstream out;
    std::vector<std::string> header = {
        "Target", "Mode", "Platform", "Status", "Confidence", "HTTP", "RTT_MS", "Bio", "Emails",
        "Phones", "ExtractedLinks", "Mentions", "URL", "Context", "RiskScore"
    };

    auto write_row = [&](const std::vector<std::string>& row) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) out << ',';
            out << csv_escape(row[i]);
        }
        out << "\n";
    };

    write_row(header);
    auto results = safe_dict_rows(payload.value("results", json::array()));
    for (const auto& row : results) {
        json contacts = row.value("contacts", json::object());
        std::vector<std::string> emails = json_string_list(contacts.value("emails", json::array()));
        std::vector<std::string> phones = json_string_list(contacts.value("phones", json::array()));
        std::vector<std::string> links = json_string_list(row.value("links", json::array()));
        std::vector<std::string> mentions = json_string_list(row.value("mentions", json::array()));
        std::string http_status = row.contains("http_status") ? json_as_string(row["http_status"], "") : "";
        std::string rtt = row.contains("response_time_ms") ? json_as_string(row["response_time_ms"], "") : "";
        write_row({
            target_name,
            mode,
            row.value("platform", ""),
            row.value("status", ""),
            std::to_string(row.value("confidence", 0)),
            http_status,
            rtt,
            row.value("bio", ""),
            join_list(emails, "; "),
            join_list(phones, "; "),
            join_list(links, "; "),
            join_list(mentions, "; "),
            row.value("url", ""),
            row.value("context", ""),
            risk_score,
        });
    }
    return out.str();
}

void write_csv_reports(const json& payload, const std::filesystem::path& cli_path) {
    auto csv_path = cli_path;
    csv_path.replace_extension(".csv");

    json metadata = payload.value("metadata", json::object());
    std::string mode = utils::trim(metadata.value("mode", ""));
    std::string target_name = utils::trim(payload.value("target", ""));
    if (target_name.empty()) {
        target_name = payload.value("target_key", "target");
    }
    std::string risk_score = "";
    if (payload.contains("issue_summary") && payload["issue_summary"].is_object()) {
        risk_score = std::to_string(payload["issue_summary"].value("risk_score", 0));
    }

    std::vector<std::vector<std::string>> result_rows;
    for (const auto& row : safe_dict_rows(payload.value("results", json::array()))) {
        json contacts = row.value("contacts", json::object());
        std::vector<std::string> emails = json_string_list(contacts.value("emails", json::array()));
        std::vector<std::string> phones = json_string_list(contacts.value("phones", json::array()));
        std::vector<std::string> links = json_string_list(row.value("links", json::array()));
        std::vector<std::string> mentions = json_string_list(row.value("mentions", json::array()));
        std::string http_status = row.contains("http_status") ? json_as_string(row["http_status"], "") : "";
        std::string rtt = row.contains("response_time_ms") ? json_as_string(row["response_time_ms"], "") : "";
        result_rows.push_back({
            target_name,
            mode,
            row.value("platform", ""),
            row.value("status", ""),
            std::to_string(row.value("confidence", 0)),
            http_status,
            rtt,
            row.value("bio", ""),
            join_list(emails, "; "),
            join_list(phones, "; "),
            join_list(links, "; "),
            join_list(mentions, "; "),
            row.value("url", ""),
            row.value("context", ""),
            risk_score,
        });
    }

    write_csv_file(
        csv_path,
        {
            "Target", "Mode", "Platform", "Status", "Confidence", "HTTP", "RTT_MS", "Bio", "Emails",
            "Phones", "ExtractedLinks", "Mentions", "URL", "Context", "RiskScore"
        },
        result_rows
    );

    auto issues_path = csv_path;
    issues_path.replace_extension(".issues.csv");
    std::vector<std::vector<std::string>> issue_rows;
    for (const auto& row : safe_dict_rows(payload.value("issues", json::array()))) {
        issue_rows.push_back({
            target_name,
            mode,
            row.value("severity", ""),
            row.value("scope", ""),
            row.value("title", ""),
            row.value("evidence", ""),
            row.value("recommendation", ""),
        });
    }
    write_csv_file(
        issues_path,
        {"Target", "Mode", "Severity", "Scope", "Title", "Evidence", "Recommendation"},
        issue_rows
    );

    auto plugins_path = csv_path;
    plugins_path.replace_extension(".plugins.csv");
    std::vector<std::vector<std::string>> plugin_rows;
    for (const auto& row : safe_dict_rows(payload.value("plugins", json::array()))) {
        plugin_rows.push_back({
            target_name,
            mode,
            row.value("id", ""),
            row.value("title", ""),
            row.value("severity", ""),
            row.value("summary", ""),
            join_list(json_string_list(row.value("highlights", json::array())), "; "),
            row.value("data", json::object()).dump(),
        });
    }
    write_csv_file(
        plugins_path,
        {"Target", "Mode", "PluginId", "Title", "Severity", "Summary", "Highlights", "DataJson"},
        plugin_rows
    );

    auto filters_path = csv_path;
    filters_path.replace_extension(".filters.csv");
    std::vector<std::vector<std::string>> filter_rows;
    for (const auto& row : safe_dict_rows(payload.value("filters", json::array()))) {
        filter_rows.push_back({
            target_name,
            mode,
            row.value("id", ""),
            row.value("title", ""),
            row.value("severity", ""),
            row.value("summary", ""),
            join_list(json_string_list(row.value("highlights", json::array())), "; "),
            row.value("data", json::object()).dump(),
        });
    }
    write_csv_file(
        filters_path,
        {"Target", "Mode", "FilterId", "Title", "Severity", "Summary", "Highlights", "DataJson"},
        filter_rows
    );

    json intelligence_bundle = payload.value("intelligence_bundle", json::object());
    auto entities_path = csv_path;
    entities_path.replace_extension(".intel-entities.csv");
    std::vector<std::vector<std::string>> entity_rows;
    for (const auto& row : safe_dict_rows(intelligence_bundle.value("scored_entities", json::array()))) {
        entity_rows.push_back({
            target_name,
            mode,
            std::to_string(row.value("rank", 0)),
            row.value("entity_type", ""),
            row.value("value", ""),
            row.value("source", ""),
            std::to_string(row.value("confidence_percent", 0)),
            row.value("risk_level", ""),
            std::to_string(row.value("relationship_count", 0)),
        });
    }
    write_csv_file(
        entities_path,
        {"Target", "Mode", "Rank", "EntityType", "Value", "Source", "ConfidencePercent", "RiskLevel", "Links"},
        entity_rows
    );

    auto contacts_path = csv_path;
    contacts_path.replace_extension(".intel-contacts.csv");
    std::vector<std::vector<std::string>> contact_rows;
    json facets = intelligence_bundle.value("entity_facets", json::object());
    for (const auto& row : safe_dict_rows(facets.value("scored_contacts", json::array()))) {
        contact_rows.push_back({
            target_name,
            mode,
            row.value("kind", ""),
            row.value("value", ""),
            std::to_string(row.value("score_percent", 0)),
            std::to_string(row.value("supporting_entities", 0)),
            row.value("risk_level", ""),
        });
    }
    write_csv_file(
        contacts_path,
        {"Target", "Mode", "Kind", "Value", "ScorePercent", "SupportingEntities", "RiskLevel"},
        contact_rows
    );
}

void write_json_report(const json& payload, const std::filesystem::path& out_path) {
    std::filesystem::create_directories(out_path.parent_path());
    std::ofstream out(out_path, std::ios::binary);
    if (!out) {
        return;
    }
    out << payload.dump(4);
    out.close();

    if (out_path.filename() == "results.json") {
        auto parent = out_path.parent_path();
        if (parent.parent_path().filename() == "data") {
            auto output_root = parent.parent_path().parent_path();
            auto legacy_path = output_root / parent.filename() / "results.json";
            std::filesystem::create_directories(legacy_path.parent_path());
            std::ofstream legacy(legacy_path, std::ios::binary);
            if (legacy) {
                legacy << payload.dump(4);
            }
        }
    }
}

void write_text_report(const std::string& text, const std::filesystem::path& out_path) {
    std::filesystem::create_directories(out_path.parent_path());
    std::ofstream out(out_path, std::ios::binary);
    if (!out) {
        return;
    }
    out << text;
}

std::string sanitize_target(const std::string& target) {
    std::string value = utils::trim(target);
    if (value.empty()) {
        return "target";
    }
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_' || ch == '.') {
            normalized.push_back(ch);
        } else {
            normalized.push_back('_');
        }
    }
    while (!normalized.empty() && (normalized.front() == '.' || normalized.front() == '_')) {
        normalized.erase(normalized.begin());
    }
    while (!normalized.empty() && (normalized.back() == '.' || normalized.back() == '_')) {
        normalized.pop_back();
    }
    return normalized.empty() ? "target" : normalized;
}

} // namespace silicore::reporting
