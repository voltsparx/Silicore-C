#pragma once

#include "engines/engine_result.h"
#include "engines/health_monitor.h"

#include <chrono>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

namespace silicore::engines {

class EngineBase {
public:
    explicit EngineBase(std::shared_ptr<EngineHealthMonitor> monitor = nullptr);
    virtual ~EngineBase() = default;

    virtual std::vector<nlohmann::json> run(const std::vector<std::function<nlohmann::json()>>& tasks) = 0;

    std::vector<EngineResult> run_detailed(
        const std::vector<std::function<nlohmann::json()>>& tasks,
        std::chrono::milliseconds timeout
    );

    EngineHealthSnapshot health_snapshot() const;

protected:
    EngineHealthMonitor& monitor();

private:
    std::shared_ptr<EngineHealthMonitor> monitor_;
};

} // namespace silicore::engines

