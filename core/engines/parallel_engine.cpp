#include "core/engines/parallel_engine.h"

#include <future>

namespace silicore::engines {

ParallelEngine::ParallelEngine(int async_concurrency, int thread_workers, int cpu_workers)
    : async_concurrency_(async_concurrency > 0 ? async_concurrency : 1),
      blocking_pool_(thread_workers > 0 ? static_cast<size_t>(thread_workers) : 1),
      cpu_engine_(cpu_workers > 0 ? static_cast<size_t>(cpu_workers) : 1) {}

ParallelResult ParallelEngine::run_hybrid(
    const std::vector<HttpRequest>& async_tasks,
    const std::vector<std::function<nlohmann::json()>>& blocking_tasks,
    const std::vector<std::function<nlohmann::json()>>& cpu_tasks
) {
    ParallelResult result;

    auto async_future = std::async(std::launch::async, [&]() {
        return run_async_batch(async_tasks, async_concurrency_);
    });

    std::vector<std::future<nlohmann::json>> blocking_futures;
    blocking_futures.reserve(blocking_tasks.size());
    for (const auto& task : blocking_tasks) {
        blocking_futures.push_back(blocking_pool_.submit(task));
    }

    auto cpu_future = std::async(std::launch::async, [&]() {
        return cpu_engine_.run_batch(cpu_tasks);
    });

    for (auto& fut : blocking_futures) {
        result.blocking_results.push_back(fut.get());
    }

    result.cpu_results = cpu_future.get();

    result.async_results = async_future.get();
    return result;
}

} // namespace silicore::engines
