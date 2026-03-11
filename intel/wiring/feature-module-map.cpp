#include <nlohmann/json.hpp>

namespace silicore::intel {

using json = nlohmann::json;

json feature_module_map() {
    return json{
        {"async_engine", {"profile", "surface"}},
        {"parallel_workers", {"surface", "plugins"}},
        {"plugin_module_system", {"plugins"}},
        {"rate_limit", {"stabilizer"}},
        {"signal_fusion_lane", {"fusion", "filters"}}
    };
}

} // namespace silicore::intel
