#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "module_filter_router",
    "Module Filter Router",
    "1.0",
    "profile,surface,fusion"
};

static json build_output(const json& payload) {
    std::string mode = payload.value("metadata", json::object()).value("mode", "");
    json out;
    if (mode == "profile") {
        out["recommended"] = {"contact_quality_filter", "triage_priority_filter"};
    } else if (mode == "surface") {
        out["recommended"] = {"subdomain_attack_path_filter", "takeover_priority_filter"};
    } else {
        out["recommended"] = {"signal_lane_fusion", "triage_priority_filter"};
    }
    return out;
}

static int compute_severity(const json& out) {
    return 1;
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
