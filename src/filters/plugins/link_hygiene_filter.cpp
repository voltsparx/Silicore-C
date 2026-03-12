#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "link_hygiene_filter",
    "Link Hygiene Filter",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    int https_count = 0;
    int total = 0;
    if (payload.contains("results") && payload["results"].is_array()) {
        for (const auto& entry : payload["results"]) {
            std::string url = entry.value("url", "");
            if (url.empty()) {
                continue;
            }
            total++;
            if (url.rfind("https://", 0) == 0) {
                https_count++;
            }
        }
    }
    int ratio = total == 0 ? 0 : (https_count * 100) / total;
    json out;
    out["https_ratio"] = ratio;
    out["total_links"] = total;
    return out;
}

static int compute_severity(const json& out) {
    int ratio = out.value("https_ratio", 0);
    if (ratio < 50) return 6;
    if (ratio < 80) return 4;
    return 2;
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
