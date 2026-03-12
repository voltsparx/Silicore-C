#include "modules/catalog.h"

namespace silicore::modules {

std::vector<ModuleEntry> plugin_modules() {
    return {
        {"account_recovery_exposure_probe", "Account Recovery Exposure Probe", "plugin", {"profile", "fusion"}, 82},
        {"contact_lattice", "Contact Lattice", "plugin", {"profile", "fusion"}, 74},
        {"cross_platform_activity_timeline", "Cross-Platform Activity Timeline", "plugin", {"profile", "fusion"}, 68},
        {"domain_takeover_risk_probe", "Domain Takeover Risk Probe", "plugin", {"surface", "fusion"}, 88},
        {"email_pattern_inference", "Email Pattern Inference", "plugin", {"profile", "fusion"}, 62},
        {"header_hardening_probe", "Header Hardening Probe", "plugin", {"surface", "fusion"}, 79},
        {"identity_fusion_core", "Identity Fusion Core", "plugin", {"fusion"}, 91},
        {"link_outbound_risk_profiler", "Link Outbound Risk Profiler", "plugin", {"profile", "fusion"}, 66},
        {"module_capability_matrix", "Module Capability Matrix", "plugin", {"profile", "surface", "fusion"}, 55},
        {"orbit_link_matrix", "Orbit Link Matrix", "plugin", {"profile", "fusion"}, 63},
        {"rdap_lifecycle_inspector", "RDAP Lifecycle Inspector", "plugin", {"surface", "fusion"}, 72},
        {"security_txt_analyzer", "Security.txt Analyzer", "plugin", {"surface", "fusion"}, 75},
        {"signal_fusion_core", "Signal Fusion Core", "plugin", {"fusion"}, 87},
        {"subdomain_risk_atlas", "Subdomain Risk Atlas", "plugin", {"surface", "fusion"}, 83},
        {"surface_transport_stability_probe", "Surface Transport Stability Probe", "plugin", {"surface", "fusion"}, 78},
        {"threat_conductor", "Threat Conductor", "plugin", {"fusion"}, 92},
        {"username_impersonation_probe", "Username Impersonation Probe", "plugin", {"profile", "fusion"}, 80},
        {"sample_summary", "Sample Summary", "plugin", {"profile", "surface"}, 40},
        {"crypto_aes_attachment", "AES Attachment Encoder", "plugin", {"profile", "surface", "fusion"}, 35},
        {"crypto_rot13", "ROT13 Encoder", "plugin", {"profile", "surface", "fusion"}, 30},
        {"crypto_xor", "XOR Encoder", "plugin", {"profile", "surface", "fusion"}, 32},
    };
}

} // namespace silicore::modules

