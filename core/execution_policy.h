#pragma once

#include <string_view>

namespace silicore {

struct ExecutionPolicy {
    std::string_view name;
    std::string_view engine_type;
    int max_workers;
    int timeout_ms;
    int retry_count;
    int enrichment_depth;
    int correlation_level;
    int concurrency;
};

const ExecutionPolicy& load_policy(std::string_view name);

} // namespace silicore
