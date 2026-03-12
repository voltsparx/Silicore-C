#include <nlohmann/json.hpp>

namespace silicore::plugins::intel {

using json = nlohmann::json;

json plugin_index() {
    json out;
    out["version"] = "1.0";
    out["theme"] = "Crystal Lattice";
    out["workflows"] = {"profile", "surface", "fusion"};
    out["plugins"] = json::array({
        json{{"id", "account_recovery_exposure_probe"}, {"focus", "account recovery exposure"}},
        json{{"id", "contact_lattice"}, {"focus", "contact signal fusion"}},
        json{{"id", "cross_platform_activity_timeline"}, {"focus", "timeline stitching"}},
        json{{"id", "domain_takeover_risk_probe"}, {"focus", "takeover hints"}},
        json{{"id", "email_pattern_inference"}, {"focus", "email heuristics"}},
        json{{"id", "header_hardening_probe"}, {"focus", "header posture"}},
        json{{"id", "identity_fusion_core"}, {"focus", "identity correlation"}},
        json{{"id", "link_outbound_risk_profiler"}, {"focus", "link safety"}},
        json{{"id", "module_capability_matrix"}, {"focus", "capability matrix"}},
        json{{"id", "orbit_link_matrix"}, {"focus", "link graphing"}},
        json{{"id", "rdap_lifecycle_inspector"}, {"focus", "registrar lifecycle"}},
        json{{"id", "security_txt_analyzer"}, {"focus", "security.txt analysis"}},
        json{{"id", "signal_fusion_core"}, {"focus", "signal fusion"}},
        json{{"id", "subdomain_risk_atlas"}, {"focus", "subdomain risk"}},
        json{{"id", "surface_transport_stability_probe"}, {"focus", "transport stability"}},
        json{{"id", "threat_conductor"}, {"focus", "threat alignment"}},
        json{{"id", "username_impersonation_probe"}, {"focus", "impersonation risk"}},
        json{{"id", "sample_summary"}, {"focus", "report summary"}}
    });
    out["crypto_plugins"] = {"aes_plugin", "rot13_plugin", "xor_plugin"};
    return out;
}

} // namespace silicore::plugins::intel
