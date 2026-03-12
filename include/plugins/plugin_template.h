#pragma once

#include "plugins/plugin_util.h"

#ifndef PLUGIN_ID
#error "PLUGIN_ID must be defined"
#endif
#ifndef PLUGIN_TITLE
#error "PLUGIN_TITLE must be defined"
#endif
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "1.0"
#endif
#ifndef PLUGIN_SCOPES
#define PLUGIN_SCOPES "profile,surface,fusion"
#endif

namespace silicore::plugins::detail {

inline int clamp_int(int value, int min_value, int max_value) {
    return value < min_value ? min_value : (value > max_value ? max_value : value);
}

inline json base_summary(const Metrics& m) {
    json out;
    out["target"] = m.target;
    out["found"] = m.found;
    out["not_found"] = m.not_found;
    out["blocked"] = m.blocked;
    out["error"] = m.error;
    out["total"] = m.total;
    out["subdomain_count"] = m.subdomain_count;
    return out;
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
    json out = base_summary(metrics);

#if defined(PLUGIN_MODE_ACCOUNT_RECOVERY)
    int exposure = metrics.found * 10;
    std::string level = metrics.found >= 8 ? "high" : (metrics.found >= 3 ? "medium" : "low");
    out["exposure_score"] = exposure;
    out["risk_level"] = level;
    out["notes"] = metrics.found == 0 ? "No exposed recovery paths detected." : "Detected potential recovery surface via found profiles.";

#elif defined(PLUGIN_MODE_CONTACT_LATTICE)
    out["contact_surface_domains"] = metrics.profile_domains;
    out["domain_count"] = static_cast<int>(metrics.profile_domains.size());
    out["signal_strength"] = metrics.total > 0 ? (metrics.found * 100) / metrics.total : 0;
    out["notes"] = metrics.profile_domains.empty() ? "No distinct profile domains observed." : "Derived contact lattice from profile URLs.";

#elif defined(PLUGIN_MODE_CROSS_PLATFORM)
    json activity = json::array();
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            if (!entry.is_object()) {
                continue;
            }
            if (entry.value("status", "") != "FOUND") {
                continue;
            }
            json item;
            item["platform"] = entry.value("platform", "-");
            item["latency_ms"] = entry.value("response_time_ms", 0);
            item["url"] = entry.value("url", "-");
            activity.push_back(item);
        }
    }
    out["active_platforms"] = metrics.found;
    out["activity"] = activity;
    out["notes"] = "Cross-platform activity timeline inferred from found profiles.";

#elif defined(PLUGIN_MODE_DOMAIN_TAKEOVER)
    int risk_score = metrics.subdomain_count / 5;
    if (metrics.http_status >= 400 || metrics.https_status >= 400) {
        risk_score += 5;
    }
    if (!metrics.redirects_to_https && metrics.http_status >= 200) {
        risk_score += 2;
    }
    risk_score = clamp_int(risk_score, 0, 10);
    out["risk_score"] = risk_score;
    out["risk_level"] = risk_score >= 7 ? "high" : (risk_score >= 4 ? "medium" : "low");
    out["http_status"] = metrics.http_status;
    out["https_status"] = metrics.https_status;
    out["notes"] = "Heuristic takeover risk based on surface signals.";

#elif defined(PLUGIN_MODE_EMAIL_PATTERN)
    std::string target = metrics.target;
    std::string local = target;
    std::string domain = metrics.domain.value("target", "");
    auto at = target.find('@');
    if (at != std::string::npos) {
        local = target.substr(0, at);
        domain = target.substr(at + 1);
    }
    if (domain.empty()) {
        domain = "<domain>";
    }
    std::vector<std::string> patterns;
    patterns.push_back(local + "@" + domain);
    patterns.push_back(local + ".{last}@" + domain);
    patterns.push_back("{first}." + local + "@" + domain);
    patterns.push_back("{first}{last}@" + domain);
    out["domain"] = domain;
    out["patterns"] = patterns;
    out["notes"] = "Common email patterns inferred from target.";

#elif defined(PLUGIN_MODE_HEADER_HARDENING)
    std::vector<std::string> required = {
        "strict-transport-security",
        "content-security-policy",
        "x-frame-options",
        "x-content-type-options",
        "referrer-policy"
    };
    std::vector<std::string> missing;
    std::vector<std::string> present;
    if (metrics.domain.contains("https") && metrics.domain["https"].is_object()) {
        const auto& headers = metrics.domain["https"]["headers"];
        if (headers.is_object()) {
            for (const auto& name : required) {
                if (headers.contains(name)) {
                    present.push_back(name);
                } else {
                    missing.push_back(name);
                }
            }
        } else {
            missing = required;
        }
    } else {
        missing = required;
    }
    out["present_headers"] = present;
    out["missing_headers"] = missing;
    out["score"] = required.empty() ? 0 : static_cast<int>((present.size() * 100) / required.size());
    out["notes"] = missing.empty() ? "Security headers look solid." : "Harden missing headers.";

#elif defined(PLUGIN_MODE_IDENTITY_FUSION)
    int score = metrics.found * 12;
    score += metrics.subdomain_count > 0 ? 10 : 0;
    score -= metrics.error * 5;
    score = clamp_int(score, 0, 100);
    out["identity_confidence"] = score;
    out["notes"] = score >= 70 ? "High confidence identity fusion." : "Moderate identity fusion signal.";

#elif defined(PLUGIN_MODE_LINK_OUTBOUND)
    int risky = 0;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            auto url = entry.value("url", "");
            auto lower = to_lower(url);
            if (lower.find("redirect") != std::string::npos || lower.find("url=") != std::string::npos) {
                risky++;
            }
        }
    }
    out["outbound_risky_links"] = risky;
    out["risk_level"] = risky > 5 ? "high" : (risky > 0 ? "medium" : "low");
    out["notes"] = "Outbound link risk derived from profile URLs.";

#elif defined(PLUGIN_MODE_MODULE_CAPABILITY)
    out["capabilities"] = {
        {"profile", true},
        {"surface", true},
        {"fusion", true},
        {"plugins", true},
        {"filters", true}
    };
    out["notes"] = "Capability matrix enumerated for Silicore-C.";

#elif defined(PLUGIN_MODE_ORBIT_LINK_MATRIX)
    json matrix = json::array();
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            if (entry.value("status", "") != "FOUND") {
                continue;
            }
            json item;
            item["platform"] = entry.value("platform", "-");
            item["url"] = entry.value("url", "-");
            matrix.push_back(item);
        }
    }
    out["matrix"] = matrix;
    out["notes"] = "Orbital link matrix built from confirmed profiles.";

#elif defined(PLUGIN_MODE_RDAP_LIFECYCLE)
    if (metrics.domain.contains("rdap") && metrics.domain["rdap"].is_object()) {
        const auto& rdap = metrics.domain["rdap"];
        out["rdap"] = {
            {"handle", rdap.value("handle", "")},
            {"registrar", rdap.value("registrar", "")},
            {"name_servers", rdap.value("name_servers", json::array())}
        };
        out["lifecycle_state"] = rdap.value("registrar", "").empty() ? "unknown" : "registered";
    } else {
        out["rdap"] = json::object();
        out["lifecycle_state"] = "unknown";
    }
    out["notes"] = "RDAP lifecycle snapshot.";

#elif defined(PLUGIN_MODE_SECURITY_TXT)
    std::string preview = metrics.domain.value("security_preview", "");
    auto emails = extract_emails(preview);
    out["security_txt_present"] = metrics.security_txt_present;
    out["contacts"] = emails;
    out["contact_count"] = static_cast<int>(emails.size());
    out["notes"] = metrics.security_txt_present ? "Security.txt detected." : "Security.txt missing.";

#elif defined(PLUGIN_MODE_SIGNAL_FUSION)
    int score = metrics.found * 10 + metrics.subdomain_count * 2 - metrics.error * 5;
    score = clamp_int(score, 0, 100);
    out["signal_score"] = score;
    out["notes"] = "Signal fusion score computed from multi-lane evidence.";

#elif defined(PLUGIN_MODE_SUBDOMAIN_RISK)
    std::vector<std::string> risky = keyword_bucket(metrics.subdomain_list, {"dev", "stage", "test", "admin", "internal", "beta"});
    out["risky_subdomains"] = risky;
    out["risky_count"] = static_cast<int>(risky.size());
    out["notes"] = "Risk atlas built from subdomain keywords.";

#elif defined(PLUGIN_MODE_SURFACE_TRANSPORT)
    bool https_ok = metrics.https_status >= 200 && metrics.https_status < 400;
    bool transport_ok = https_ok && metrics.redirects_to_https;
    out["https_status"] = metrics.https_status;
    out["http_status"] = metrics.http_status;
    out["redirects_to_https"] = metrics.redirects_to_https;
    out["transport_stable"] = transport_ok;
    out["notes"] = transport_ok ? "Surface transport stable." : "Transport stability issues detected.";

#elif defined(PLUGIN_MODE_THREAT_CONDUCTOR)
    int score = metrics.found * 2 + metrics.subdomain_count / 3 + metrics.blocked * 2 + metrics.error * 3;
    if (!metrics.security_txt_present) {
        score += 3;
    }
    score = clamp_int(score, 0, 20);
    out["threat_score"] = score;
    out["priority"] = score >= 12 ? "high" : (score >= 6 ? "medium" : "low");
    out["notes"] = "Threat conductor aggregated score.";

#elif defined(PLUGIN_MODE_USERNAME_IMPERSONATION)
    std::string base = metrics.target;
    auto at = base.find('@');
    if (at != std::string::npos) {
        base = base.substr(0, at);
    }
    std::vector<std::string> variants = {
        base,
        base + "_",
        base + ".",
        base + "1",
        base + "_official",
        base + "official"
    };
    std::string risk = base.size() <= 3 ? "high" : (metrics.found > 5 ? "medium" : "low");
    out["username"] = base;
    out["variants"] = variants;
    out["risk_level"] = risk;
    out["notes"] = "Impersonation risk inferred from username surface.";

#else
    out["notes"] = "Plugin output not configured.";
#endif

    return out;
}

inline int default_severity(const json& output) {
    if (output.contains("risk_level")) {
        auto level = output.value("risk_level", "");
        if (level == "high") return 8;
        if (level == "medium") return 5;
        if (level == "low") return 2;
    }
    if (output.contains("priority")) {
        auto level = output.value("priority", "");
        if (level == "high") return 8;
        if (level == "medium") return 5;
        if (level == "low") return 2;
    }
    if (output.contains("threat_score")) {
        return clamp_int(output.value("threat_score", 0) / 2, 0, 10);
    }
    return 3;
}

} // namespace silicore::plugins::detail

static const PluginSpec SPEC = {
    PLUGIN_ID,
    PLUGIN_TITLE,
    PLUGIN_VERSION,
    PLUGIN_SCOPES
};

extern "C" {

const PluginSpec* silicore_plugin_spec(void) {
    return &SPEC;
}

PluginOutput silicore_plugin_run(const PluginContext* ctx) {
    auto payload = silicore::plugins::parse_context(ctx);
    auto out = silicore::plugins::detail::build_output(payload);
    int severity = silicore::plugins::detail::default_severity(out);
    return silicore::plugins::make_output(out, severity);
}

void silicore_plugin_free_output(PluginOutput* out) {
    silicore::plugins::free_output(out);
}

} // extern "C"

#undef PLUGIN_ID
#undef PLUGIN_TITLE
#undef PLUGIN_VERSION
#undef PLUGIN_SCOPES
