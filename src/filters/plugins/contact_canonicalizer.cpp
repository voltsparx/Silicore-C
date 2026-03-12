#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "contact_canonicalizer",
    "Contact Canonicalizer",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    std::string preview = metrics.domain.value("security_preview", "");
    std::vector<std::string> contacts;
    std::string current;
    for (char ch : preview) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '@' || ch == '.' || ch == '_' || ch == '-' || ch == '+') {
            current.push_back(ch);
        } else {
            if (current.find('@') != std::string::npos) {
                contacts.push_back(current);
            }
            current.clear();
        }
    }
    if (current.find('@') != std::string::npos) {
        contacts.push_back(current);
    }
    std::unordered_set<std::string> seen;
    std::vector<std::string> unique;
    for (auto& c : contacts) {
        auto lower = silicore::filters::to_lower(c);
        if (!seen.count(lower)) {
            seen.insert(lower);
            unique.push_back(lower);
        }
    }
    json out;
    out["contacts"] = unique;
    out["contact_count"] = static_cast<int>(unique.size());
    return out;
}

static int compute_severity(const json& out) {
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
