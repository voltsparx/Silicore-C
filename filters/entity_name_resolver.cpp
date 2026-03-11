#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "entity_name_resolver",
    "Entity Name Resolver",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    auto lower = silicore::filters::to_lower(metrics.target);
    std::string compact;
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : lower) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            compact.push_back(ch);
            current.push_back(ch);
        } else {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    json out;
    out["normalized"] = {lower, compact};
    out["tokens"] = tokens;
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
