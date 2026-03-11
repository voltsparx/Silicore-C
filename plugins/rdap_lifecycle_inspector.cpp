#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "rdap_lifecycle_inspector",
    "RDAP Lifecycle Inspector",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    json out;
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
    return out;
}

static int compute_severity(const json& out) {
    std::string state = out.value("lifecycle_state", "unknown");
    if (state == "unknown") return 3;
    return 1;
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
