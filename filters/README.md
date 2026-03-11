Silicore-C filters

Filters are lightweight post-processing modules that read the JSON report payload and emit derived signals.
They are compiled as shared libraries into build/filters and can be loaded with --filters or --all-filters.

Core filters included:
- anomaly_detection_filter
- contact_canonicalizer
- contact_quality_filter
- disclosure_readiness_filter
- entity_name_resolver
- evidence_consistency_filter
- exposure_tier_matrix
- link_hygiene_filter
- mailbox_provider_profiler
- module_filter_router
- noise_suppression_filter
- pii_signal_classifier
- sensitive_lexicon_guard
- signal_lane_fusion
- subdomain_attack_path_filter
- takeover_priority_filter
- triage_priority_filter
