#include <string>
#include <vector>

namespace silicore::filters {

std::vector<std::string> builtin_filter_ids() {
    return {
        "anomaly_detection_filter",
        "contact_canonicalizer",
        "contact_quality_filter",
        "disclosure_readiness_filter",
        "entity_name_resolver",
        "evidence_consistency_filter",
        "exposure_tier_matrix",
        "link_hygiene_filter",
        "mailbox_provider_profiler",
        "module_filter_router",
        "noise_suppression_filter",
        "pii_signal_classifier",
        "sensitive_lexicon_guard",
        "signal_lane_fusion",
        "subdomain_attack_path_filter",
        "takeover_priority_filter",
        "triage_priority_filter"
    };
}

std::string filters_init_summary() {
    return "Silicore-C filter pack loaded (" + std::to_string(builtin_filter_ids().size()) + " modules).";
}

} // namespace silicore::filters
