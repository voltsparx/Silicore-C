#pragma once

#include "plugins/plugin_api.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace silicore::plugins {

using json = nlohmann::json;

struct Metrics {
    int found = 0;
    int not_found = 0;
    int blocked = 0;
    int error = 0;
    int total = 0;
    int subdomain_count = 0;
    int https_status = 0;
    int http_status = 0;
    bool redirects_to_https = false;
    bool security_txt_present = false;
    bool robots_txt_present = false;
    std::string target;
    json domain;
    std::vector<std::string> subdomain_list;
    std::vector<std::string> profile_domains;
};

inline json parse_context(const PluginContext* ctx) {
    if (!ctx || !ctx->json_context || ctx->json_length <= 0) {
        return json::object();
    }
    try {
        return json::parse(std::string(ctx->json_context, ctx->json_length));
    } catch (const std::exception&) {
        return json::object();
    }
}

inline int count_status(const json& payload, const std::string& status) {
    if (!payload.contains("results") || !payload["results"].is_array()) {
        return 0;
    }
    int count = 0;
    for (const auto& entry : payload["results"]) {
        if (entry.value("status", "") == status) {
            count++;
        }
    }
    return count;
}

inline int total_results(const json& payload) {
    if (!payload.contains("results") || !payload["results"].is_array()) {
        return 0;
    }
    return static_cast<int>(payload["results"].size());
}

inline std::string target_value(const json& payload) {
    return payload.value("target", "");
}

inline json domain_result(const json& payload) {
    if (payload.contains("domain_result") && payload["domain_result"].is_object()) {
        return payload["domain_result"];
    }
    return json::object();
}

inline std::vector<std::string> subdomains(const json& payload) {
    auto dr = domain_result(payload);
    std::vector<std::string> out;
    if (dr.contains("subdomains") && dr["subdomains"].is_array()) {
        for (const auto& item : dr["subdomains"]) {
            if (item.is_string()) {
                out.push_back(item.get<std::string>());
            }
        }
    }
    return out;
}

inline std::string to_lower(std::string value) {
    for (auto& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

inline std::string extract_domain_from_url(const std::string& url) {
    auto start = url.find("://");
    size_t offset = start == std::string::npos ? 0 : start + 3;
    size_t end = url.find('/', offset);
    std::string host = url.substr(offset, end == std::string::npos ? std::string::npos : end - offset);
    auto at = host.find('@');
    if (at != std::string::npos) {
        host = host.substr(at + 1);
    }
    return host;
}

inline std::vector<std::string> unique_profile_domains(const json& payload) {
    std::unordered_set<std::string> seen;
    std::vector<std::string> out;
    if (!payload.contains("results") || !payload["results"].is_array()) {
        return out;
    }
    for (const auto& entry : payload["results"]) {
        if (!entry.is_object()) {
            continue;
        }
        if (entry.value("status", "") != "FOUND") {
            continue;
        }
        std::string url = entry.value("url", "");
        if (url.empty()) {
            continue;
        }
        auto host = extract_domain_from_url(url);
        auto lower = to_lower(host);
        if (lower.empty() || seen.count(lower)) {
            continue;
        }
        seen.insert(lower);
        out.push_back(host);
    }
    return out;
}

inline std::vector<std::string> extract_emails(const std::string& text) {
    std::vector<std::string> out;
    std::string current;
    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '@' || ch == '.' || ch == '_' || ch == '-' || ch == '+') {
            current.push_back(ch);
        } else {
            if (current.find('@') != std::string::npos && current.find('.') != std::string::npos) {
                out.push_back(current);
            }
            current.clear();
        }
    }
    if (current.find('@') != std::string::npos && current.find('.') != std::string::npos) {
        out.push_back(current);
    }
    return out;
}

inline PluginOutput make_output(const json& payload, int severity) {
    std::string dump = payload.dump();
    auto* buffer = new char[dump.size()];
    std::memcpy(buffer, dump.data(), dump.size());
    PluginOutput out;
    out.json_output = buffer;
    out.json_length = static_cast<int>(dump.size());
    out.severity = severity;
    return out;
}

inline void free_output(PluginOutput* out) {
    if (!out) {
        return;
    }
    delete[] out->json_output;
    out->json_output = nullptr;
    out->json_length = 0;
    out->severity = 0;
}

inline Metrics extract_metrics(const json& payload) {
    Metrics m;
    m.target = target_value(payload);
    m.found = count_status(payload, "FOUND");
    m.not_found = count_status(payload, "NOT_FOUND");
    m.blocked = count_status(payload, "BLOCKED");
    m.error = count_status(payload, "ERROR");
    m.total = total_results(payload);
    m.domain = domain_result(payload);
    if (m.domain.contains("https") && m.domain["https"].is_object()) {
        m.https_status = m.domain["https"].value("status", 0);
    }
    if (m.domain.contains("http") && m.domain["http"].is_object()) {
        m.http_status = m.domain["http"].value("status", 0);
        m.redirects_to_https = m.domain["http"].value("redirects_to_https", false);
    }
    m.security_txt_present = m.domain.value("security_txt_present", false);
    m.robots_txt_present = m.domain.value("robots_txt_present", false);
    m.subdomain_list = subdomains(payload);
    m.subdomain_count = static_cast<int>(m.subdomain_list.size());
    m.profile_domains = unique_profile_domains(payload);
    return m;
}

} // namespace silicore::plugins
