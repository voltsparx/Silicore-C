#pragma once

#include "engines/health_monitor.h"

namespace silicore::engines {

struct StabilizerPolicy {
    int min_concurrency = 10;
    int max_concurrency = 500;
    double failure_rate_threshold = 0.2;
    double latency_threshold_ms = 1500.0;
    double backoff_factor = 0.7;
    double recovery_factor = 1.1;
};

class StabilizerEngine {
public:
    explicit StabilizerEngine(StabilizerPolicy policy = {});

    int adjust_concurrency(int base_concurrency, const EngineHealthSnapshot& snapshot) const;

private:
    StabilizerPolicy policy_;
};

} // namespace silicore::engines

