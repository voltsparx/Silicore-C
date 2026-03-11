#include "core/engines/cpu_engine.h"

namespace silicore::engines {

CpuEngine::CpuEngine(size_t worker_count)
    : pool_(worker_count > 0 ? worker_count : 1) {}

std::vector<nlohmann::json> CpuEngine::run_batch(
    const std::vector<std::function<nlohmann::json()>>& tasks
) {
    std::vector<nlohmann::json> results;
    if (tasks.empty()) {
        return results;
    }

    std::vector<std::future<nlohmann::json>> futures;
    futures.reserve(tasks.size());
    for (const auto& task : tasks) {
        futures.push_back(pool_.submit(task));
    }

    results.reserve(tasks.size());
    for (auto& fut : futures) {
        results.push_back(fut.get());
    }
    return results;
}

} // namespace silicore::engines
