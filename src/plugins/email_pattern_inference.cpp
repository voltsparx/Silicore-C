#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "email_pattern_inference",
    "Email Pattern Inference",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    std::string target = metrics.target;
    std::string local = target;
    std::string domain = metrics.domain.value("target", "");
    auto at = target.find('@');
    if (at != std::string::npos) {
        local = target.substr(0, at);
        domain = target.substr(at + 1);
    }
    if (domain.empty()) {
        domain = "<domain>";
    }
    json out;
    out["target"] = target;
    out["domain"] = domain;
    out["patterns"] = json::array({
        local + "@" + domain,
        local + ".{last}@" + domain,
        "{first}." + local + "@" + domain,
        "{first}{last}@" + domain,
        "{first}_" + local + "@" + domain
    });
    out["notes"] = "Common email patterns inferred from target.";
    return out;
}

static int compute_severity(const json& out) {
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
