#include "filters/filter_util.h"

#include <string>
#include <unordered_set>
#include <vector>

using json = silicore::filters::json;

static const FilterSpec SPEC = {
    "anomaly_detection_filter",
    "Anomaly Detection Filter",
    "1.0",
    "profile,surface,fusion"
};

static json build_output(const json& payload) {
    auto metrics = silicore::filters::extract_metrics(payload);
    json anomalies = json::array();
    int total = metrics.total == 0 ? 1 : metrics.total;
    if ((metrics.error * 100) / total > 20) {
        anomalies.push_back("high_error_rate");
    }
    if ((metrics.blocked * 100) / total > 30) {
        anomalies.push_back("high_block_rate");
    }
    if (metrics.subdomain_count > 100) {
        anomalies.push_back("large_subdomain_surface");
    }
    if (metrics.found == 0 && metrics.total > 0) {
        anomalies.push_back("no_presence");
    }
    json out;
    out["target"] = metrics.target;
    out["anomalies"] = anomalies;
    out["error_rate"] = (metrics.error * 100) / total;
    out["blocked_rate"] = (metrics.blocked * 100) / total;
    return out;
}

static int compute_severity(const json& out) {
    int count = out.value("anomalies", json::array()).size();
    if (count >= 2) return 6;
    if (count >= 1) return 4;
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
