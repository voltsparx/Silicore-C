#include <nlohmann/json.hpp>

namespace silicore::plugins::intel {

using json = nlohmann::json;

json surface_plan() {
    return json{
        {"workflow", "surface"},
        {"recommended_plugins", {"security_txt_analyzer", "rdap_lifecycle_inspector", "domain_takeover_risk_probe", "surface_transport_stability_probe"}},
        {"recommended_filters", {"subdomain_attack_path_filter", "takeover_priority_filter"}},
        {"notes", "Optimized for domain surface and security posture."}
    };
}

} // namespace silicore::plugins::intel
