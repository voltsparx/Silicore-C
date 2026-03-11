#include <nlohmann/json.hpp>

namespace silicore::intel {

using json = nlohmann::json;

json workflow_feature_map() {
    return json{
        {"profile", {"async_engine", "plugin_module_system"}},
        {"surface", {"async_engine", "parallel_workers", "rate_limit"}},
        {"fusion", {"signal_fusion_lane", "plugin_module_system"}}
    };
}

} // namespace silicore::intel
