#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "signal_fusion_core",
    "Signal Fusion Core",
    "1.0",
    "fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    int score = metrics.found * 10 + metrics.subdomain_count * 2 - metrics.error * 5 + metrics.blocked;
    if (score < 0) score = 0;
    if (score > 100) score = 100;
    json out;
    out["signal_score"] = score;
    out["notes"] = "Signal fusion score computed from multi-lane evidence.";
    return out;
}

static int compute_severity(const json& out) {
    int score = out.value("signal_score", 0);
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
