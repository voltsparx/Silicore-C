#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "subdomain_attack_path_filter",
    "Subdomain Attack Path Filter",
    "1.0",
    "surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    std::vector<std::string> risky;
    for (const auto& sub : metrics.subdomain_list) {
        auto lower = silicore::filters::to_lower(sub);
        if (lower.find("dev") != std::string::npos || lower.find("stage") != std::string::npos ||
            lower.find("test") != std::string::npos || lower.find("admin") != std::string::npos ||
            lower.find("internal") != std::string::npos || lower.find("beta") != std::string::npos) {
            risky.push_back(sub);
        }
    }
    json out;
    out["risky_subdomains"] = risky;
    out["risky_count"] = static_cast<int>(risky.size());
    return out;
}

static int compute_severity(const json& out) {
    int count = out.value("risky_count", 0);
    return count > 0 ? 5 : 2;
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
