#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "threat_conductor",
    "Threat Conductor",
    "1.0",
    "fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    int score = metrics.found * 2 + metrics.subdomain_count / 3 + metrics.blocked * 2 + metrics.error * 3;
    if (!metrics.security_txt_present) {
        score += 3;
    }
    if (score < 0) score = 0;
    if (score > 20) score = 20;
    json out;
    out["threat_score"] = score;
    out["priority"] = score >= 12 ? "high" : (score >= 6 ? "medium" : "low");
    out["notes"] = "Threat conductor aggregated score.";
    return out;
}

static int compute_severity(const json& out) {
    std::string level = out.value("priority", "");
    if (level == "high") return 8;
    if (level == "medium") return 5;
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
