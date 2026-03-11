#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "username_impersonation_probe",
    "Username Impersonation Probe",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
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
        base + "official",
        base + "-real",
        "real-" + base
    };
    std::string risk = base.size() <= 3 ? "high" : (metrics.found > 5 ? "medium" : "low");
    json out;
    out["username"] = base;
    out["variants"] = variants;
    out["risk_level"] = risk;
    out["notes"] = "Impersonation risk inferred from username surface.";
    return out;
}

static int compute_severity(const json& out) {
    std::string level = out.value("risk_level", "");
    if (level == "high") return 7;
    if (level == "medium") return 4;
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
