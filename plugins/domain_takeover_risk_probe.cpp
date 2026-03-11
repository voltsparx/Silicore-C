#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "domain_takeover_risk_probe",
    "Domain Takeover Risk Probe",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    int risk_score = metrics.subdomain_count / 5;
    if (metrics.http_status >= 400) {
        risk_score += 3;
    }
    if (metrics.https_status >= 400) {
        risk_score += 3;
    }
    if (!metrics.redirects_to_https && metrics.http_status >= 200) {
        risk_score += 2;
    }
    if (risk_score > 10) risk_score = 10;
    if (risk_score < 0) risk_score = 0;
    json out;
    out["target"] = metrics.target;
    out["subdomain_count"] = metrics.subdomain_count;
    out["http_status"] = metrics.http_status;
    out["https_status"] = metrics.https_status;
    out["redirects_to_https"] = metrics.redirects_to_https;
    out["risk_score"] = risk_score;
    out["risk_level"] = risk_score >= 7 ? "high" : (risk_score >= 4 ? "medium" : "low");
    out["notes"] = "Heuristic takeover risk based on surface signals.";
    return out;
}

static int compute_severity(const json& out) {
    std::string level = out.value("risk_level", "");
    if (level == "high") return 8;
    if (level == "medium") return 5;
    if (level == "low") return 2;
    return 3;
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
