#pragma once

#include "core/engines/async_engine.h"
#include "core/engines/thread_engine.h"

#include <functional>
#include <nlohmann/json.hpp>
#include <vector>

namespace silicore::engines {

struct ParallelResult {
    std::vector<HttpResponse> async_results;
    std::vector<nlohmann::json> thread_results;
};

class ParallelEngine {
public:
    ParallelEngine(int async_concurrency, int thread_workers);

    ParallelResult run_hybrid(
        const std::vector<HttpRequest>& async_tasks,
        const std::vector<std::function<nlohmann::json()>>& cpu_tasks
    );

private:
    int async_concurrency_;
    ThreadPool thread_pool_;
};

} // namespace silicore::engines
