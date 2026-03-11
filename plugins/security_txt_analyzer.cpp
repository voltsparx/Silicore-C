#include "plugins/plugin_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "security_txt_analyzer",
    "Security.txt Analyzer",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::plugins::extract_metrics(payload);
    std::string preview = metrics.domain.value("security_preview", "");
    auto emails = silicore::plugins::extract_emails(preview);
    json out;
    out["security_txt_present"] = metrics.security_txt_present;
    out["contacts"] = emails;
    out["contact_count"] = static_cast<int>(emails.size());
    out["notes"] = metrics.security_txt_present ? "Security.txt detected." : "Security.txt missing.";
    return out;
}

static int compute_severity(const json& out) {
    bool present = out.value("security_txt_present", false);
    return present ? 2 : 6;
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
