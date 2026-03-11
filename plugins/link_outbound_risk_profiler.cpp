#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "link_outbound_risk_profiler",
    "Link Outbound Risk Profiler",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    int risky = 0;
    int total = 0;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            std::string url = entry.value("url", "");
            if (url.empty()) {
                continue;
            }
            total++;
            auto lower = silicore::plugins::to_lower(url);
            if (lower.find("redirect") != std::string::npos || lower.find("url=") != std::string::npos ||
                lower.find("next=") != std::string::npos || lower.find("dest=") != std::string::npos ||
                lower.find("target=") != std::string::npos) {
                risky++;
            }
        }
    }
    json out;
    out["risky_links"] = risky;
    out["total_links"] = total;
    out["risk_level"] = risky > 5 ? "high" : (risky > 0 ? "medium" : "low");
    out["notes"] = "Outbound link risk derived from profile URLs.";
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
