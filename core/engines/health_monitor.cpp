#include "core/engines/health_monitor.h"

#include <algorithm>

namespace silicore::engines {

EngineHealthMonitor::EngineHealthMonitor(size_t latency_window)
    : window_(std::max<size_t>(10, latency_window)) {}

void EngineHealthMonitor::begin() {
    std::lock_guard<std::mutex> guard(lock_);
    active_tasks_++;
}

void EngineHealthMonitor::end() {
    std::lock_guard<std::mutex> guard(lock_);
    active_tasks_ = std::max(0, active_tasks_ - 1);
}

void EngineHealthMonitor::record(const EngineResult& result) {
    std::lock_guard<std::mutex> guard(lock_);
    latencies_.push_back(std::max(0.0, result.execution_time));
    if (latencies_.size() > window_) {
        latencies_.pop_front();
    }
    total_results_++;
    if (!result.ok()) {
        failed_results_++;
        auto key = result.name.empty() ? "unknown" : result.name;
        failure_counts_[key] = failure_counts_[key] + 1;
    }
}

EngineHealthSnapshot EngineHealthMonitor::snapshot() const {
    std::lock_guard<std::mutex> guard(lock_);
    double avg = 0.0;
    if (!latencies_.empty()) {
        double sum = 0.0;
        for (double v : latencies_) {
            sum += v;
        }
        avg = sum / static_cast<double>(latencies_.size());
    }
    EngineHealthSnapshot snap;
    snap.active_tasks = active_tasks_;
    snap.failed_engines = 0;
    for (const auto& item : failure_counts_) {
        snap.failed_engines += item.second;
    }
    snap.average_response_time = avg;
    snap.total_results = total_results_;
    snap.failed_results = failed_results_;
    snap.failure_rate = total_results_ > 0 ? static_cast<double>(failed_results_) / static_cast<double>(total_results_) : 0.0;
    snap.engine_failure_counts = failure_counts_;
    return snap;
}

} // namespace silicore::engines
