#pragma once

#include <string>

namespace silicore::engines {

enum class EngineStatus { Success, Failed, Timeout };

struct EngineResult {
    std::string name;
    EngineStatus status = EngineStatus::Success;
    std::string error;
    double execution_time = 0.0;

    bool ok() const { return status == EngineStatus::Success; }
};

} // namespace silicore::engines
