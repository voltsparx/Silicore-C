#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "cross_platform_activity_timeline",
    "Cross-Platform Activity Timeline",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    json activity = json::array();
    int total_latency = 0;
    int count = 0;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            if (entry.value("status", "") != "FOUND") {
                continue;
            }
            json item;
            item["platform"] = entry.value("platform", "-");
            item["url"] = entry.value("url", "-");
            item["latency_ms"] = entry.value("response_time_ms", 0);
            activity.push_back(item);
            total_latency += entry.value("response_time_ms", 0);
            count++;
        }
    }
    json out;
    out["target"] = metrics.target;
    out["active_platforms"] = metrics.found;
    out["avg_latency_ms"] = count > 0 ? total_latency / count : 0;
    out["activity"] = activity;
    out["notes"] = "Cross-platform activity timeline inferred from found profiles.";
    return out;
}

static int compute_severity(const json& out) {
    int active = out.value("active_platforms", 0);
    if (active >= 10) return 6;
    if (active >= 3) return 4;
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
