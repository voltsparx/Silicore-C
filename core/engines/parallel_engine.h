#pragma once

#include "core/engines/async_engine.h"
#include "core/engines/cpu_engine.h"
#include "core/engines/thread_engine.h"

#include <functional>
#include <nlohmann/json.hpp>
#include <vector>

namespace silicore::engines {

struct ParallelResult {
    std::vector<HttpResponse> async_results;
    std::vector<nlohmann::json> blocking_results;
    std::vector<nlohmann::json> cpu_results;
};

class ParallelEngine {
public:
    ParallelEngine(int async_concurrency, int thread_workers, int cpu_workers);

    ParallelResult run_hybrid(
        const std::vector<HttpRequest>& async_tasks,
        const std::vector<std::function<nlohmann::json()>>& blocking_tasks,
        const std::vector<std::function<nlohmann::json()>>& cpu_tasks
    );

private:
    int async_concurrency_;
    ThreadPool blocking_pool_;
    CpuEngine cpu_engine_;
};

} // namespace silicore::engines
