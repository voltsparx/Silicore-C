#include <nlohmann/json.hpp>

namespace silicore::plugins::intel {

using json = nlohmann::json;

json profile_plan() {
    return json{
        {"workflow", "profile"},
        {"recommended_plugins", {"identity_fusion_core", "contact_lattice", "email_pattern_inference", "username_impersonation_probe"}},
        {"recommended_filters", {"contact_quality_filter", "triage_priority_filter"}},
        {"notes", "Optimized for username enumeration and identity correlation."}
    };
}

} // namespace silicore::plugins::intel
