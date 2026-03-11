#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "surface_transport_stability_probe",
    "Surface Transport Stability Probe",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    bool https_ok = metrics.https_status >= 200 && metrics.https_status < 400;
    bool stable = https_ok && metrics.redirects_to_https;
    json out;
    out["https_status"] = metrics.https_status;
    out["http_status"] = metrics.http_status;
    out["redirects_to_https"] = metrics.redirects_to_https;
    out["transport_stable"] = stable;
    out["notes"] = stable ? "Surface transport stable." : "Transport stability issues detected.";
    return out;
}

static int compute_severity(const json& out) {
    bool stable = out.value("transport_stable", false);
    return stable ? 1 : 6;
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
