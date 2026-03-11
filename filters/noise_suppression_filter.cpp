#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "noise_suppression_filter",
    "Noise Suppression Filter",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    std::unordered_set<std::string> unique_platforms;
    int total = 0;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            total++;
            unique_platforms.insert(entry.value("platform", ""));
        }
    }
    int duplicates = total - static_cast<int>(unique_platforms.size());
    json out;
    out["duplicates"] = duplicates;
    out["unique_platforms"] = static_cast<int>(unique_platforms.size());
    return out;
}

static int compute_severity(const json& out) {
    int dupes = out.value("duplicates", 0);
    return dupes > 0 ? 3 : 1;
}

extern "C" {

const FilterSpec* silicore_filter_spec(void) {
    return &SPEC;
}

FilterOutput silicore_filter_run(const FilterContext* ctx) {
    auto payload = silicore::filters::parse_context(ctx);
    auto out = build_output(payload);
    int severity = compute_severity(out);
    return silicore::filters::make_output(out, severity);
}

void silicore_filter_free_output(FilterOutput* out) {
    silicore::filters::free_output(out);
}

} // extern "C"
