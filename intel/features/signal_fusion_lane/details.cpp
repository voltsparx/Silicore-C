#include <nlohmann/json.hpp>

namespace silicore::intel {

using json = nlohmann::json;

json signal_fusion_lane_details() {
    return json{
        {"id", "signal_fusion_lane"},
        {"focus", "combining profile + surface + plugin signals"},
        {"outputs", {"confidence", "priority"}},
        {"notes", "Backed by fusion workflow and filters."}
    };
}

} // namespace silicore::intel
