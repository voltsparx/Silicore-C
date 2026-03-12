#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "sensitive_lexicon_guard",
    "Sensitive Lexicon Guard",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    std::vector<std::string> hits;
    std::string preview = silicore::filters::to_lower(metrics.domain.value("security_preview", ""));
    for (const auto& key : {"password", "token", "secret", "apikey", "credential"}) {
        if (preview.find(key) != std::string::npos) {
            hits.push_back(key);
        }
    }
    json out;
    out["hits"] = hits;
    out["hit_count"] = static_cast<int>(hits.size());
    return out;
}

static int compute_severity(const json& out) {
    int count = out.value("hit_count", 0);
    return count > 0 ? 6 : 1;
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
