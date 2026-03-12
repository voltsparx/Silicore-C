#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "mailbox_provider_profiler",
    "Mailbox Provider Profiler",
    "1.0",
    "profile,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    std::string target = metrics.target;
    auto at = target.find('@');
    std::string domain = at == std::string::npos ? "" : target.substr(at + 1);
    auto lower = silicore::filters::to_lower(domain);
    std::string provider = "unknown";
    if (lower.find("gmail") != std::string::npos) provider = "gmail";
    else if (lower.find("outlook") != std::string::npos || lower.find("hotmail") != std::string::npos) provider = "microsoft";
    else if (lower.find("yahoo") != std::string::npos) provider = "yahoo";
    else if (lower.find("proton") != std::string::npos) provider = "proton";
    else if (lower.find("icloud") != std::string::npos || lower.find("me.com") != std::string::npos) provider = "apple";
    json out;
    out["provider"] = provider;
    out["domain"] = domain;
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
