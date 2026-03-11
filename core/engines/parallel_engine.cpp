#include "core/engines/parallel_engine.h"

#include <future>

namespace silicore::engines {

ParallelEngine::ParallelEngine(int async_concurrency, int thread_workers)
    : async_concurrency_(async_concurrency > 0 ? async_concurrency : 1),
      thread_pool_(thread_workers > 0 ? static_cast<size_t>(thread_workers) : 1) {}

ParallelResult ParallelEngine::run_hybrid(
    const std::vector<HttpRequest>& async_tasks,
    const std::vector<std::function<nlohmann::json()>>& cpu_tasks
) {
    ParallelResult result;

    auto async_future = std::async(std::launch::async, [&]() {
        return run_async_batch(async_tasks, async_concurrency_);
    });

    std::vector<std::future<nlohmann::json>> futures;
    futures.reserve(cpu_tasks.size());
    for (const auto& task : cpu_tasks) {
        futures.push_back(thread_pool_.submit(task));
    }

    for (auto& fut : futures) {
        result.thread_results.push_back(fut.get());
    }

    result.async_results = async_future.get();
    return result;
}

} // namespace silicore::engines
