#include "engines/scheduler.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace silicore::engines {

double Scheduler::now_seconds() {
    using clock = std::chrono::system_clock;
    auto now = clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

void Scheduler::schedule_scan(const ScanTask& task, const std::string& target, double delay_seconds) {
    double run_at = now_seconds() + std::max(0.0, delay_seconds);
    std::lock_guard<std::mutex> guard(lock_);
    queue_.push_back({task, target, run_at});
}

std::vector<nlohmann::json> Scheduler::run_pending(double now_seconds_override) {
    double now = now_seconds_override >= 0.0 ? now_seconds_override : now_seconds();
    std::vector<Scheduled> pending;
    {
        std::lock_guard<std::mutex> guard(lock_);
        auto it = queue_.begin();
        while (it != queue_.end()) {
            if (it->run_at <= now) {
                pending.push_back(*it);
                it = queue_.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::vector<nlohmann::json> results;
    results.reserve(pending.size());
    for (const auto& item : pending) {
        nlohmann::json entry;
        entry["target"] = item.target;
        entry["scheduled_at"] = item.run_at;
        entry["ran_at"] = now;
        try {
            entry["result"] = item.task(item.target);
            entry["ok"] = true;
        } catch (const std::exception& exc) {
            entry["ok"] = false;
            entry["error"] = exc.what();
        }
        results.push_back(std::move(entry));
    }
    return results;
}

nlohmann::json Scheduler::merge_results(const nlohmann::json& left, const nlohmann::json& right) {
    if (!left.is_object() || !right.is_object()) {
        return right;
    }
    nlohmann::json merged = left;
    for (auto it = right.begin(); it != right.end(); ++it) {
        if (merged.contains(it.key()) && merged[it.key()].is_object() && it.value().is_object()) {
            merged[it.key()] = merge_results(merged[it.key()], it.value());
        } else {
            merged[it.key()] = it.value();
        }
    }
    return merged;
}

std::string Scheduler::send_alert(const std::string& target, const nlohmann::json& risk_payload) {
    std::ostringstream out;
    out << "alert target=" << target;
    if (risk_payload.contains("risk_score") && risk_payload["risk_score"].is_number()) {
        out << " risk_score=" << risk_payload["risk_score"].get<int>();
    }
    return out.str();
}

} // namespace silicore::engines

