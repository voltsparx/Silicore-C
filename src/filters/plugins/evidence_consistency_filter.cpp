#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "evidence_consistency_filter",
    "Evidence Consistency Filter",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    bool consistent = metrics.redirects_to_https || (metrics.https_status >= 200 && metrics.https_status < 400);
    json out;
    out["consistent"] = consistent;
    out["http_status"] = metrics.http_status;
    out["https_status"] = metrics.https_status;
    return out;
}

static int compute_severity(const json& out) {
    bool consistent = out.value("consistent", false);
    return consistent ? 2 : 6;
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
