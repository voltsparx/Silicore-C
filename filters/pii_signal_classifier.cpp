#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "pii_signal_classifier",
    "PII Signal Classifier",
    "1.0",
    "profile,surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    std::string preview = metrics.domain.value("security_preview", "");
    int email_count = 0;
    std::string current;
    for (char ch : preview) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '@' || ch == '.' || ch == '_' || ch == '-' || ch == '+') {
            current.push_back(ch);
        } else {
            if (current.find('@') != std::string::npos) {
                email_count++;
            }
            current.clear();
        }
    }
    if (current.find('@') != std::string::npos) {
        email_count++;
    }
    json out;
    out["email_signals"] = email_count;
    return out;
}

static int compute_severity(const json& out) {
    int count = out.value("email_signals", 0);
    return count > 0 ? 4 : 1;
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
