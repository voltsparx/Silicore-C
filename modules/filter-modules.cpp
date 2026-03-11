#include "modules/catalog.h"

namespace silicore::modules {

std::vector<ModuleEntry> filter_modules() {
    return {
        {"anomaly_detection_filter", "Anomaly Detection Filter", "filter", {"profile", "surface", "fusion"}, 76},
        {"contact_canonicalizer", "Contact Canonicalizer", "filter", {"profile", "fusion"}, 64},
        {"contact_quality_filter", "Contact Quality Filter", "filter", {"profile", "fusion"}, 70},
        {"disclosure_readiness_filter", "Disclosure Readiness Filter", "filter", {"surface", "fusion"}, 74},
        {"entity_name_resolver", "Entity Name Resolver", "filter", {"profile", "fusion"}, 60},
        {"evidence_consistency_filter", "Evidence Consistency Filter", "filter", {"surface", "fusion"}, 68},
        {"exposure_tier_matrix", "Exposure Tier Matrix", "filter", {"profile", "fusion"}, 72},
        {"link_hygiene_filter", "Link Hygiene Filter", "filter", {"profile", "fusion"}, 65},
        {"mailbox_provider_profiler", "Mailbox Provider Profiler", "filter", {"profile", "fusion"}, 58},
        {"module_filter_router", "Module Filter Router", "filter", {"profile", "surface", "fusion"}, 55},
        {"noise_suppression_filter", "Noise Suppression Filter", "filter", {"profile", "fusion"}, 61},
        {"pii_signal_classifier", "PII Signal Classifier", "filter", {"profile", "surface", "fusion"}, 69},
        {"sensitive_lexicon_guard", "Sensitive Lexicon Guard", "filter", {"surface", "fusion"}, 77},
        {"signal_lane_fusion", "Signal Lane Fusion", "filter", {"fusion"}, 73},
        {"subdomain_attack_path_filter", "Subdomain Attack Path Filter", "filter", {"surface", "fusion"}, 81},
        {"takeover_priority_filter", "Takeover Priority Filter", "filter", {"surface", "fusion"}, 84},
        {"triage_priority_filter", "Triage Priority Filter", "filter", {"profile", "surface", "fusion"}, 79},
    };
}

} // namespace silicore::modules

