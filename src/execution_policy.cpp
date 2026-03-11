#include "execution_policy.h"

#include "utils/strings.h"

namespace silicore {

static constexpr ExecutionPolicy POLICIES[] = {
    {"fast", "async", 8, 10000, 1, 1, 1, 120},
    {"balanced", "hybrid", 20, 20000, 2, 2, 2, 200},
    {"deep", "hybrid", 35, 35000, 3, 3, 3, 300},
    {"max", "hybrid", 50, 50000, 4, 4, 4, 400},
};

const ExecutionPolicy& load_policy(std::string_view name) {
    std::string key = utils::to_lower(std::string(name));
    if (key == "safe") key = "fast";
    if (key == "standard") key = "balanced";
    if (key == "aggressive") key = "max";

    for (const auto& policy : POLICIES) {
        if (policy.name == key) {
            return policy;
        }
    }
    return POLICIES[1];
}

} // namespace silicore

