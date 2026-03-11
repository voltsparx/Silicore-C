#include <string>
#include <vector>

namespace silicore::plugins {

std::vector<std::string> builtin_plugin_ids() {
    return {
        "account_recovery_exposure_probe",
        "contact_lattice",
        "cross_platform_activity_timeline",
        "domain_takeover_risk_probe",
        "email_pattern_inference",
        "header_hardening_probe",
        "identity_fusion_core",
        "link_outbound_risk_profiler",
        "module_capability_matrix",
        "orbit_link_matrix",
        "rdap_lifecycle_inspector",
        "security_txt_analyzer",
        "signal_fusion_core",
        "subdomain_risk_atlas",
        "surface_transport_stability_probe",
        "threat_conductor",
        "username_impersonation_probe",
        "sample_summary"
    };
}

std::vector<std::string> crypto_plugin_ids() {
    return {"aes_plugin", "rot13_plugin", "xor_plugin"};
}

std::string plugins_init_summary() {
    size_t count = builtin_plugin_ids().size() + crypto_plugin_ids().size();
    return "Silicore-C plugin pack loaded (" + std::to_string(count) + " modules).";
}

} // namespace silicore::plugins
