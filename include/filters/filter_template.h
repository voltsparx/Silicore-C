#pragma once

#include "filters/filter_util.h"

#ifndef FILTER_ID
#error "FILTER_ID must be defined"
#endif
#ifndef FILTER_TITLE
#error "FILTER_TITLE must be defined"
#endif
#ifndef FILTER_VERSION
#define FILTER_VERSION "1.0"
#endif
#ifndef FILTER_SCOPES
#define FILTER_SCOPES "profile,surface,fusion"
#endif

namespace silicore::filters::detail {

inline int clamp_int(int value, int min_value, int max_value) {
    return value < min_value ? min_value : (value > max_value ? max_value : value);
}

inline std::vector<std::string> keyword_bucket(const std::vector<std::string>& items, const std::vector<std::string>& keywords) {
    std::vector<std::string> out;
    for (const auto& item : items) {
        auto lower = to_lower(item);
        for (const auto& key : keywords) {
            if (lower.find(key) != std::string::npos) {
                out.push_back(item);
                break;
            }
        }
    }
    return out;
}

inline json build_output(const json& payload) {
    auto metrics = extract_metrics(payload);
    json out;
    out["target"] = metrics.target;

#if defined(FILTER_MODE_ANOMALY_DETECTION)
    json anomalies = json::array();
    int total = metrics.total == 0 ? 1 : metrics.total;
    if ((metrics.error * 100) / total > 20) {
        anomalies.push_back("high_error_rate");
    }
    if ((metrics.blocked * 100) / total > 30) {
        anomalies.push_back("high_block_rate");
    }
    if (metrics.subdomain_count > 100) {
        anomalies.push_back("large_subdomain_surface");
    }
    out["anomalies"] = anomalies;

#elif defined(FILTER_MODE_CONTACT_CANONICALIZER)
    std::string preview = metrics.domain.value("security_preview", "");
    std::vector<std::string> contacts;
    std::string current;
    for (char ch : preview) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '@' || ch == '.' || ch == '_' || ch == '-' || ch == '+') {
            current.push_back(ch);
        } else {
            if (current.find('@') != std::string::npos) {
                contacts.push_back(current);
            }
            current.clear();
        }
    }
    if (current.find('@') != std::string::npos) {
        contacts.push_back(current);
    }
    out["contacts"] = contacts;

#elif defined(FILTER_MODE_CONTACT_QUALITY)
    out["security_txt_present"] = metrics.security_txt_present;
    out["quality"] = metrics.security_txt_present ? "high" : "low";

#elif defined(FILTER_MODE_DISCLOSURE_READINESS)
    out["ready"] = metrics.security_txt_present;
    out["notes"] = metrics.security_txt_present ? "Disclosure path present." : "Missing security.txt.";

#elif defined(FILTER_MODE_ENTITY_NAME_RESOLVER)
    auto lower = to_lower(metrics.target);
    std::string compact;
    for (char ch : lower) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            compact.push_back(ch);
        }
    }
    out["normalized"] = {lower, compact};

#elif defined(FILTER_MODE_EVIDENCE_CONSISTENCY)
    bool consistent = metrics.redirects_to_https || (metrics.https_status >= 200 && metrics.https_status < 400);
    out["consistent"] = consistent;
    out["http_status"] = metrics.http_status;
    out["https_status"] = metrics.https_status;

#elif defined(FILTER_MODE_EXPOSURE_TIER)
    int score = metrics.found * 2 + metrics.subdomain_count / 10;
    std::string tier = score >= 10 ? "high" : (score >= 5 ? "medium" : "low");
    out["tier"] = tier;
    out["score"] = score;

#elif defined(FILTER_MODE_LINK_HYGIENE)
    int https_count = 0;
    int total = 0;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            std::string url = entry.value("url", "");
            if (url.empty()) {
                continue;
            }
            total++;
            if (url.rfind("https://", 0) == 0) {
                https_count++;
            }
        }
    }
    out["https_ratio"] = total == 0 ? 0 : (https_count * 100) / total;

#elif defined(FILTER_MODE_MAILBOX_PROVIDER)
    std::string target = metrics.target;
    auto at = target.find('@');
    std::string domain = at == std::string::npos ? "" : target.substr(at + 1);
    auto lower = to_lower(domain);
    std::string provider = "unknown";
    if (lower.find("gmail") != std::string::npos) provider = "gmail";
    else if (lower.find("outlook") != std::string::npos || lower.find("hotmail") != std::string::npos) provider = "microsoft";
    else if (lower.find("yahoo") != std::string::npos) provider = "yahoo";
    out["provider"] = provider;

#elif defined(FILTER_MODE_MODULE_ROUTER)
    std::string mode = payload.value("metadata", json::object()).value("mode", "");
    if (mode == "profile") {
        out["recommended"] = {"contact_quality_filter", "triage_priority_filter"};
    } else if (mode == "surface") {
        out["recommended"] = {"subdomain_attack_path_filter", "takeover_priority_filter"};
    } else {
        out["recommended"] = {"signal_lane_fusion", "triage_priority_filter"};
    }

#elif defined(FILTER_MODE_NOISE_SUPPRESSION)
    std::unordered_set<std::string> unique_platforms;
    int total = 0;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            total++;
            unique_platforms.insert(entry.value("platform", ""));
        }
    }
    out["duplicates"] = total - static_cast<int>(unique_platforms.size());

#elif defined(FILTER_MODE_PII_SIGNAL)
    std::string preview = metrics.domain.value("security_preview", "");
    int email_count = 0;
    std::string current;
    for (char ch : preview) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '@' || ch == '.' || ch == '_' || ch == '-' || ch == '+') {
            current.push_back(ch);
        } else {
            if (current.find('@') != std::string::npos) {
                email_count++;
            }
            current.clear();
        }
    }
    if (current.find('@') != std::string::npos) {
        email_count++;
    }
    out["email_signals"] = email_count;

#elif defined(FILTER_MODE_SENSITIVE_LEXICON)
    std::vector<std::string> hits;
    std::string preview = to_lower(metrics.domain.value("security_preview", ""));
    for (const auto& key : {"password", "token", "secret", "apikey", "credential"}) {
        if (preview.find(key) != std::string::npos) {
            hits.push_back(key);
        }
    }
    out["hits"] = hits;

#elif defined(FILTER_MODE_SIGNAL_LANE_FUSION)
    int score = metrics.found + metrics.blocked - metrics.error;
    out["lane_score"] = score;

#elif defined(FILTER_MODE_SUBDOMAIN_ATTACK_PATH)
    auto risky = keyword_bucket(metrics.subdomain_list, {"dev", "stage", "test", "admin", "internal", "beta"});
    out["risky_subdomains"] = risky;
    out["risky_count"] = static_cast<int>(risky.size());

#elif defined(FILTER_MODE_TAKEOVER_PRIORITY)
    int score = metrics.subdomain_count / 5 + (metrics.http_status >= 400 ? 3 : 0);
    out["priority"] = score >= 8 ? "high" : (score >= 4 ? "medium" : "low");

#elif defined(FILTER_MODE_TRIAGE_PRIORITY)
    int score = metrics.found * 2 + metrics.subdomain_count / 5 + metrics.error * 2;
    out["priority"] = score >= 10 ? "high" : (score >= 5 ? "medium" : "low");
    out["score"] = score;

#else
    out["notes"] = "Filter output not configured.";
#endif

    return out;
}

inline int default_severity(const json& output) {
    if (output.contains("priority")) {
        auto level = output.value("priority", "");
        if (level == "high") return 8;
        if (level == "medium") return 5;
        if (level == "low") return 2;
    }
    if (output.contains("tier")) {
        auto level = output.value("tier", "");
        if (level == "high") return 7;
        if (level == "medium") return 4;
        if (level == "low") return 2;
    }
    return 3;
}

} // namespace silicore::filters::detail

static const FilterSpec SPEC = {
    FILTER_ID,
    FILTER_TITLE,
    FILTER_VERSION,
    FILTER_SCOPES
};

extern "C" {

const FilterSpec* silicore_filter_spec(void) {
    return &SPEC;
}

FilterOutput silicore_filter_run(const FilterContext* ctx) {
    auto payload = silicore::filters::parse_context(ctx);
    auto out = silicore::filters::detail::build_output(payload);
    int severity = silicore::filters::detail::default_severity(out);
    return silicore::filters::make_output(out, severity);
}

void silicore_filter_free_output(FilterOutput* out) {
    silicore::filters::free_output(out);
}

} // extern "C"

#undef FILTER_ID
#undef FILTER_TITLE
#undef FILTER_VERSION
#undef FILTER_SCOPES
