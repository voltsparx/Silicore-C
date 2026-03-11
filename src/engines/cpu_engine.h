#pragma once

#include "engines/thread_engine.h"

#include <functional>
#include <nlohmann/json.hpp>
#include <vector>

namespace silicore::engines {

class CpuEngine {
public:
    explicit CpuEngine(size_t worker_count);

    std::vector<nlohmann::json> run_batch(
        const std::vector<std::function<nlohmann::json()>>& tasks
    );

private:
    ThreadPool pool_;
};

} // namespace silicore::engines

