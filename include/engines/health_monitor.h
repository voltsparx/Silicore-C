#pragma once

#include "engines/engine_result.h"

#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

namespace silicore::engines {

struct EngineHealthSnapshot {
    int active_tasks = 0;
    int failed_engines = 0;
    double average_response_time = 0.0;
    int total_results = 0;
    int failed_results = 0;
    double failure_rate = 0.0;
    std::unordered_map<std::string, int> engine_failure_counts;
};

class EngineHealthMonitor {
public:
    explicit EngineHealthMonitor(size_t latency_window = 500);

    void begin();
    void end();
    void record(const EngineResult& result);
    EngineHealthSnapshot snapshot() const;

private:
    size_t window_;
    mutable std::mutex lock_;
    std::deque<double> latencies_;
    std::unordered_map<std::string, int> failure_counts_;
    int active_tasks_ = 0;
    int total_results_ = 0;
    int failed_results_ = 0;
};

} // namespace silicore::engines

