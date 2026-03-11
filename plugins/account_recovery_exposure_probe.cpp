#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "account_recovery_exposure_probe",
    "Account Recovery Exposure Probe",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    json out;
    out["target"] = metrics.target;
    out["found_profiles"] = metrics.found;
    out["exposure_score"] = metrics.found * 10;
    if (metrics.found >= 8) {
        out["risk_level"] = "high";
    } else if (metrics.found >= 3) {
        out["risk_level"] = "medium";
    } else {
        out["risk_level"] = "low";
    }
    out["notes"] = metrics.found == 0 ? "No exposed recovery paths detected." : "Potential recovery surface detected.";
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
