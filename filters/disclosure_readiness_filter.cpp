#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "disclosure_readiness_filter",
    "Disclosure Readiness Filter",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    json out;
    out["ready"] = metrics.security_txt_present;
    out["notes"] = metrics.security_txt_present ? "Disclosure path present." : "Missing security.txt.";
    return out;
}

static int compute_severity(const json& out) {
    bool ready = out.value("ready", false);
    return ready ? 2 : 6;
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
