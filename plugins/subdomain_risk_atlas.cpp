#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "subdomain_risk_atlas",
    "Subdomain Risk Atlas",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    std::vector<std::string> risky;
    for (const auto& sub : metrics.subdomain_list) {
        auto lower = silicore::plugins::to_lower(sub);
        if (lower.find("dev") != std::string::npos || lower.find("stage") != std::string::npos ||
            lower.find("test") != std::string::npos || lower.find("admin") != std::string::npos ||
            lower.find("internal") != std::string::npos || lower.find("beta") != std::string::npos) {
            risky.push_back(sub);
        }
    }
    json out;
    out["subdomain_count"] = metrics.subdomain_count;
    out["risky_subdomains"] = risky;
    out["risky_count"] = static_cast<int>(risky.size());
    out["notes"] = "Risk atlas built from subdomain keywords.";
    return out;
}

static int compute_severity(const json& out) {
    int count = out.value("risky_count", 0);
    if (count >= 10) return 7;
    if (count >= 3) return 5;
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
