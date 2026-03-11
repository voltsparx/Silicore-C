#include <nlohmann/json.hpp>

namespace silicore::plugins::intel {

using json = nlohmann::json;

json fusion_plan() {
    return json{
        {"workflow", "fusion"},
        {"recommended_plugins", {"signal_fusion_core", "threat_conductor", "orbit_link_matrix", "module_capability_matrix"}},
        {"recommended_filters", {"signal_lane_fusion", "triage_priority_filter"}},
        {"notes", "Optimized for correlation and prioritization."}
    };
}

} // namespace silicore::plugins::intel
