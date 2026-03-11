#pragma once

#include "filters/filter_api.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

namespace silicore::filters {

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
};

inline json parse_context(const FilterContext* ctx) {
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

inline FilterOutput make_output(const json& payload, int severity) {
    std::string dump = payload.dump();
    auto* buffer = new char[dump.size()];
    std::memcpy(buffer, dump.data(), dump.size());
    FilterOutput out;
    out.json_output = buffer;
    out.json_length = static_cast<int>(dump.size());
    out.severity = severity;
    return out;
}

inline void free_output(FilterOutput* out) {
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
    m.target = payload.value("target", "");
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
    return m;
}

} // namespace silicore::filters
