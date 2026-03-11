#include "core/engines/stabilizer_engine.h"

#include <algorithm>

namespace silicore::engines {

StabilizerEngine::StabilizerEngine(StabilizerPolicy policy) : policy_(policy) {}

int StabilizerEngine::adjust_concurrency(int base_concurrency, const EngineHealthSnapshot& snapshot) const {
    int adjusted = base_concurrency;
    if (snapshot.failure_rate > policy_.failure_rate_threshold || snapshot.average_response_time > policy_.latency_threshold_ms) {
        adjusted = static_cast<int>(base_concurrency * policy_.backoff_factor);
    } else if (snapshot.total_results > 0 && snapshot.failure_rate < (policy_.failure_rate_threshold * 0.5)) {
        adjusted = static_cast<int>(base_concurrency * policy_.recovery_factor);
    }

    adjusted = std::max(policy_.min_concurrency, adjusted);
    adjusted = std::min(policy_.max_concurrency, adjusted);
    return adjusted;
}

} // namespace silicore::engines
