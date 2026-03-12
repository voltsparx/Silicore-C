#pragma once

#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace silicore::engines {

class Scheduler {
public:
    using ScanTask = std::function<nlohmann::json(const std::string&)>;

    void schedule_scan(const ScanTask& task, const std::string& target, double delay_seconds);
    std::vector<nlohmann::json> run_pending(double now_seconds = -1.0);

    static nlohmann::json merge_results(const nlohmann::json& left, const nlohmann::json& right);
    static std::string send_alert(const std::string& target, const nlohmann::json& risk_payload);

private:
    struct Scheduled {
        ScanTask task;
        std::string target;
        double run_at = 0.0;
    };

    static double now_seconds();

    std::vector<Scheduled> queue_;
    std::mutex lock_;
};

} // namespace silicore::engines
