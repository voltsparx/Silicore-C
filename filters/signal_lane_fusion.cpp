#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "signal_lane_fusion",
    "Signal Lane Fusion",
    "1.0",
    "fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    int lane_score = metrics.found + metrics.blocked - metrics.error;
    json out;
    out["lane_score"] = lane_score;
    return out;
}

static int compute_severity(const json& out) {
    return 3;
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
