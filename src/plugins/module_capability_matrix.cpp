#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "module_capability_matrix",
    "Module Capability Matrix",
    "1.0",
    "profile,surface,fusion"
};

static json build_output(const json& payload) {
    json out;
    out["capabilities"] = {
        {"profile", true},
        {"surface", true},
        {"fusion", true},
        {"plugins", true},
        {"filters", true}
    };
    out["notes"] = "Capability matrix enumerated for Silicore-C.";
    return out;
}

static int compute_severity(const json& out) {
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
