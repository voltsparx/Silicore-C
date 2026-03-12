#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "header_hardening_probe",
    "Header Hardening Probe",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    std::vector<std::string> required = {
        "strict-transport-security",
        "content-security-policy",
        "x-frame-options",
        "x-content-type-options",
        "referrer-policy"
    };
    std::vector<std::string> present;
    std::vector<std::string> missing;
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
    json out;
    out["target"] = metrics.target;
    out["present_headers"] = present;
    out["missing_headers"] = missing;
    out["score"] = required.empty() ? 0 : static_cast<int>((present.size() * 100) / required.size());
    out["notes"] = missing.empty() ? "Security headers look solid." : "Harden missing headers.";
    return out;
}

static int compute_severity(const json& out) {
    int score = out.value("score", 0);
    if (score >= 80) return 2;
    if (score >= 50) return 4;
    return 7;
}

extern "C" {

const PluginSpec* silicore_plugin_spec(void) {
    return &SPEC;
}

PluginOutput silicore_plugin_run(const PluginContext* ctx) {
    auto payload = silicore::plugins::parse_context(ctx);
    auto out = build_output(payload);
    int severity = compute_severity(out);
    return silicore::plugins::make_output(out, severity);
}

void silicore_plugin_free_output(PluginOutput* out) {
    silicore::plugins::free_output(out);
}

} // extern "C"
