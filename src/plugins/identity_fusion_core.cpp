#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "identity_fusion_core",
    "Identity Fusion Core",
    "1.0",
    "fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    int score = metrics.found * 12 + metrics.subdomain_count * 2 - metrics.error * 5;
    if (metrics.security_txt_present) {
        score += 5;
    }
    if (score < 0) score = 0;
    if (score > 100) score = 100;
    json out;
    out["target"] = metrics.target;
    out["identity_confidence"] = score;
    out["found_profiles"] = metrics.found;
    out["subdomain_count"] = metrics.subdomain_count;
    out["notes"] = score >= 70 ? "High confidence identity fusion." : (score >= 40 ? "Moderate identity fusion signal." : "Low identity fusion signal.");
    return out;
}

static int compute_severity(const json& out) {
    int score = out.value("identity_confidence", 0);
    if (score >= 70) return 6;
    if (score >= 40) return 4;
    return 2;
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
