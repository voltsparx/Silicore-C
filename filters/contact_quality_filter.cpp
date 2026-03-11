#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "contact_quality_filter",
    "Contact Quality Filter",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    std::string preview = metrics.domain.value("security_preview", "");
    int contact_count = 0;
    std::string current;
    for (char ch : preview) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '@' || ch == '.' || ch == '_' || ch == '-' || ch == '+') {
            current.push_back(ch);
        } else {
            if (current.find('@') != std::string::npos) {
                contact_count++;
            }
            current.clear();
        }
    }
    if (current.find('@') != std::string::npos) {
        contact_count++;
    }
    json out;
    out["security_txt_present"] = metrics.security_txt_present;
    out["contact_count"] = contact_count;
    out["quality"] = (metrics.security_txt_present && contact_count > 0) ? "high" : "low";
    return out;
}

static int compute_severity(const json& out) {
    std::string quality = out.value("quality", "low");
    return quality == "high" ? 2 : 5;
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
