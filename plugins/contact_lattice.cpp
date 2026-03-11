#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "contact_lattice",
    "Contact Lattice",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    auto domains = silicore::plugins::unique_profile_domains(payload);
    json out;
    out["target"] = metrics.target;
    out["contact_surface_domains"] = domains;
    out["domain_count"] = static_cast<int>(domains.size());
    out["signal_strength"] = metrics.total > 0 ? (metrics.found * 100) / metrics.total : 0;
    out["notes"] = domains.empty() ? "No distinct profile domains observed." : "Derived contact lattice from profile URLs.";
    return out;
}

static int compute_severity(const json& out) {
    int strength = out.value("signal_strength", 0);
    if (strength >= 70) return 6;
    if (strength >= 30) return 4;
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
