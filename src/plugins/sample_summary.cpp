#include "plugins/plugin_api.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <string>

using json = nlohmann::json;

static const PluginSpec SPEC = {
    "sample_summary",
    "Sample Summary",
    "1.0",
    "profile,surface"
};

extern "C" {

const PluginSpec* silicore_plugin_spec(void) {
    return &SPEC;
}

PluginOutput silicore_plugin_run(const PluginContext* ctx) {
    json payload;
    try {
        payload = json::parse(std::string(ctx->json_context, ctx->json_length));
    } catch (const std::exception&) {
        payload = json::object();
    }

    int found = 0;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            if (entry.value("status", "") == "FOUND") {
                found++;
            }
        }
    }

    json out;
    out["found_profiles"] = found;
    out["summary"] = "sample plugin summary";

    std::string result = out.dump();
    char* buffer = new char[result.size() + 1];
    std::copy(result.begin(), result.end(), buffer);
    buffer[result.size()] = '\0';

    PluginOutput output;
    output.json_output = buffer;
    output.json_length = static_cast<int>(result.size());
    output.severity = found > 0 ? 1 : 0;
    return output;
}

void silicore_plugin_free_output(PluginOutput* out) {
    delete[] out->json_output;
    out->json_output = nullptr;
    out->json_length = 0;
}

} // extern "C"
