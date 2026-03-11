#include "core/engines/engine_base.h"

#include <future>

namespace silicore::engines {

EngineBase::EngineBase(std::shared_ptr<EngineHealthMonitor> monitor)
    : monitor_(monitor ? monitor : std::make_shared<EngineHealthMonitor>()) {}

EngineHealthMonitor& EngineBase::monitor() {
    return *monitor_;
}

EngineHealthSnapshot EngineBase::health_snapshot() const {
    return monitor_->snapshot();
}

std::vector<EngineResult> EngineBase::run_detailed(
    const std::vector<std::function<nlohmann::json()>>& tasks,
    std::chrono::milliseconds timeout
) {
    std::vector<EngineResult> results;
    results.reserve(tasks.size());

    int index = 0;
    for (const auto& task : tasks) {
        EngineResult result;
        result.name = "task-" + std::to_string(++index);
        monitor_->begin();
        auto start = std::chrono::steady_clock::now();
        try {
            if (timeout.count() > 0) {
                auto future = std::async(std::launch::async, task);
                if (future.wait_for(timeout) != std::future_status::ready) {
                    result.status = EngineStatus::Timeout;
                    result.error = "timeout";
                } else {
                    future.get();
                    result.status = EngineStatus::Success;
                }
            } else {
                task();
                result.status = EngineStatus::Success;
            }
        } catch (const std::exception& exc) {
            result.status = EngineStatus::Failed;
            result.error = exc.what();
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start
        );
        result.execution_time = static_cast<double>(elapsed.count()) / 1000.0;
        monitor_->end();
        monitor_->record(result);
        results.push_back(result);
    }

    return results;
}

} // namespace silicore::engines
