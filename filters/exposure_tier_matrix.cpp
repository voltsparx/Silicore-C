#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "exposure_tier_matrix",
    "Exposure Tier Matrix",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    int score = metrics.found * 2 + metrics.subdomain_count / 10;
    std::string tier = score >= 10 ? "high" : (score >= 5 ? "medium" : "low");
    json out;
    out["tier"] = tier;
    out["score"] = score;
    return out;
}

static int compute_severity(const json& out) {
    std::string tier = out.value("tier", "low");
    if (tier == "high") return 6;
    if (tier == "medium") return 4;
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
