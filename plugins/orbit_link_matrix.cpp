#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "orbit_link_matrix",
    "Orbit Link Matrix",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    json matrix = json::array();
    int unique_platforms = 0;
    std::unordered_set<std::string> seen;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            if (entry.value("status", "") != "FOUND") {
                continue;
            }
            json item;
            item["platform"] = entry.value("platform", "-");
            item["url"] = entry.value("url", "-");
            item["confidence"] = entry.value("confidence", 0);
            matrix.push_back(item);
            auto lower = silicore::plugins::to_lower(entry.value("platform", ""));
            if (!lower.empty() && !seen.count(lower)) {
                seen.insert(lower);
                unique_platforms++;
            }
        }
    }
    json out;
    out["matrix"] = matrix;
    out["unique_platforms"] = unique_platforms;
    out["notes"] = "Orbital link matrix built from confirmed profiles.";
    return out;
}

static int compute_severity(const json& out) {
    int count = out.value("unique_platforms", 0);
    if (count >= 10) return 5;
    if (count >= 3) return 3;
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
