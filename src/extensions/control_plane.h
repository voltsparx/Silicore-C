#pragma once

#include "modules/catalog.h"
#include "utils/strings.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace silicore::extensions {

struct ExtensionControlPlan {
    std::string scope;
    std::string scan_mode;
    std::string control_mode;
    std::vector<std::string> plugins;
    std::vector<std::string> filters;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

inline std::string normalize_scan_mode(const std::string& mode) {
    std::string key = utils::to_lower(utils::trim(mode));
    if (key == "safe") return "fast";
    if (key == "quick") return "fast";
    if (key == "standard") return "balanced";
    if (key == "aggressive") return "max";
    if (key.empty()) return "balanced";
    return key;
}

inline std::string merge_scan_modes(const std::string& first, const std::string& second) {
    auto normalize = [](const std::string& value) {
        return normalize_scan_mode(value);
    };
    std::unordered_map<std::string, int> order = {
        {"fast", 1}, {"balanced", 2}, {"deep", 3}, {"max", 4},
    };
    std::string left = normalize(first);
    std::string right = normalize(second);
    return order[left] >= order[right] ? left : right;
}

inline std::string selector_key(const std::string& raw) {
    std::string value = utils::to_lower(utils::trim(raw));
    if (value.empty()) {
        return value;
    }
    std::string normalized;
    normalized.reserve(value.size());
    bool dash = false;
    for (char ch : value) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            normalized.push_back(ch);
            dash = false;
        } else {
            if (!dash) {
                normalized.push_back('-');
                dash = true;
            }
        }
    }
    normalized = utils::trim(normalized, '-');
    return normalized.empty() ? value : normalized;
}

inline std::vector<std::string> selector_keys(const std::string& raw) {
    std::vector<std::string> keys;
    std::string trimmed = utils::trim(raw);
    if (trimmed.empty()) {
        return keys;
    }
    std::string lowered = utils::to_lower(trimmed);
    keys.push_back(lowered);
    std::string normalized = selector_key(trimmed);
    if (normalized != lowered) {
        keys.push_back(normalized);
    }
    return keys;
}

struct ExtensionDescriptor {
    std::string id;
    std::string title;
    std::vector<std::string> scopes;
};

inline std::vector<ExtensionDescriptor> list_descriptors(const std::string& kind, const std::string& scope) {
    std::vector<ExtensionDescriptor> out;
    std::string kind_lower = utils::to_lower(utils::trim(kind));
    std::string scope_lower = utils::to_lower(utils::trim(scope));
    for (const auto& entry : modules::all_modules()) {
        if (!kind_lower.empty() && utils::to_lower(entry.kind) != kind_lower) {
            continue;
        }
        if (!scope_lower.empty() && scope_lower != "all") {
            bool match = false;
            for (const auto& tag : entry.tags) {
                if (utils::to_lower(tag) == scope_lower) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                continue;
            }
        }
        out.push_back({entry.id, entry.title, entry.tags});
    }
    return out;
}

inline void build_lookup(
    const std::vector<ExtensionDescriptor>& descriptors,
    std::unordered_map<std::string, std::string>& lookup,
    std::unordered_set<std::string>& ids
) {
    for (const auto& descriptor : descriptors) {
        std::string id = utils::to_lower(utils::trim(descriptor.id));
        if (id.empty()) {
            continue;
        }
        ids.insert(id);
        for (const auto& key : selector_keys(id)) {
            lookup.emplace(key, id);
        }
        std::string title = utils::trim(descriptor.title);
        if (!title.empty()) {
            for (const auto& key : selector_keys(title)) {
                lookup.emplace(key, id);
            }
        }
    }
}

inline std::pair<std::vector<std::string>, std::vector<std::string>> resolve_selector_ids(
    const std::vector<std::string>& selectors,
    const std::unordered_map<std::string, std::string>& lookup
) {
    std::vector<std::string> selected;
    std::vector<std::string> unknown;
    std::unordered_set<std::string> seen;
    for (const auto& raw : selectors) {
        auto keys = selector_keys(raw);
        if (keys.empty()) {
            continue;
        }
        std::string matched;
        for (const auto& key : keys) {
            auto it = lookup.find(key);
            if (it != lookup.end()) {
                matched = it->second;
                break;
            }
        }
        if (matched.empty()) {
            unknown.push_back(raw);
            continue;
        }
        if (seen.insert(matched).second) {
            selected.push_back(matched);
        }
    }
    return {selected, unknown};
}

inline std::vector<std::string> unique_list(const std::vector<std::string>& values) {
    std::unordered_set<std::string> seen;
    std::vector<std::string> out;
    for (const auto& value : values) {
        std::string key = utils::to_lower(utils::trim(value));
        if (key.empty()) {
            continue;
        }
        if (seen.insert(key).second) {
            out.push_back(key);
        }
    }
    return out;
}

inline ExtensionControlPlan resolve_extension_control(
    const std::string& scope,
    const std::string& scan_mode,
    const std::string& control_mode,
    const std::vector<std::string>& requested_plugins,
    const std::vector<std::string>& requested_filters,
    bool include_all_plugins,
    bool include_all_filters
) {
    ExtensionControlPlan plan;
    plan.scope = utils::to_lower(utils::trim(scope));
    plan.scan_mode = normalize_scan_mode(scan_mode);
    plan.control_mode = utils::to_lower(utils::trim(control_mode));

    const std::unordered_set<std::string> valid_scopes = {"profile", "surface", "fusion"};
    const std::unordered_set<std::string> valid_controls = {"auto", "manual", "hybrid"};

    if (!valid_scopes.count(plan.scope)) {
        plan.errors.push_back("Unsupported extension scope: " + scope);
    }
    if (!valid_controls.count(plan.control_mode)) {
        plan.errors.push_back("Unsupported extension control mode: " + control_mode);
    }

    auto scoped_plugins = list_descriptors("plugin", plan.scope);
    auto scoped_filters = list_descriptors("filter", plan.scope);
    auto global_plugins = list_descriptors("plugin", "");
    auto global_filters = list_descriptors("filter", "");

    std::unordered_map<std::string, std::string> plugin_lookup;
    std::unordered_map<std::string, std::string> filter_lookup;
    std::unordered_map<std::string, std::string> global_plugin_lookup;
    std::unordered_map<std::string, std::string> global_filter_lookup;
    std::unordered_set<std::string> available_plugins;
    std::unordered_set<std::string> available_filters;
    std::unordered_set<std::string> global_plugins_ids;
    std::unordered_set<std::string> global_filters_ids;
    build_lookup(scoped_plugins, plugin_lookup, available_plugins);
    build_lookup(scoped_filters, filter_lookup, available_filters);
    build_lookup(global_plugins, global_plugin_lookup, global_plugins_ids);
    build_lookup(global_filters, global_filter_lookup, global_filters_ids);

    if (include_all_plugins && !requested_plugins.empty()) {
        plan.errors.push_back("Cannot combine --all-plugins with --plugin selectors.");
    }
    if (include_all_filters && !requested_filters.empty()) {
        plan.errors.push_back("Cannot combine --all-filters with --filter selectors.");
    }
    if (plan.control_mode == "auto" &&
        (include_all_plugins || include_all_filters || !requested_plugins.empty() || !requested_filters.empty())) {
        plan.errors.push_back(
            "Auto extension control cannot be combined with manual plugin/filter flags. "
            "Use --extension-control hybrid or manual."
        );
    }

    std::vector<std::string> manual_plugins;
    std::vector<std::string> manual_filters;
    if (include_all_plugins) {
        manual_plugins.assign(available_plugins.begin(), available_plugins.end());
        std::sort(manual_plugins.begin(), manual_plugins.end());
    } else {
        auto resolved = resolve_selector_ids(requested_plugins, plugin_lookup);
        manual_plugins = resolved.first;
        for (const auto& unknown : resolved.second) {
            bool incompatible = false;
            for (const auto& key : selector_keys(unknown)) {
                if (global_plugin_lookup.count(key)) {
                    incompatible = true;
                    break;
                }
            }
            if (incompatible) {
                plan.errors.push_back(
                    "Incompatible plugin selector for scope '" + plan.scope + "': " + unknown
                );
            } else {
                plan.errors.push_back("Unknown plugin selector: " + unknown);
            }
        }
    }

    if (include_all_filters) {
        manual_filters.assign(available_filters.begin(), available_filters.end());
        std::sort(manual_filters.begin(), manual_filters.end());
    } else {
        auto resolved = resolve_selector_ids(requested_filters, filter_lookup);
        manual_filters = resolved.first;
        for (const auto& unknown : resolved.second) {
            bool incompatible = false;
            for (const auto& key : selector_keys(unknown)) {
                if (global_filter_lookup.count(key)) {
                    incompatible = true;
                    break;
                }
            }
            if (incompatible) {
                plan.errors.push_back(
                    "Incompatible filter selector for scope '" + plan.scope + "': " + unknown
                );
            } else {
                plan.errors.push_back("Unknown filter selector: " + unknown);
            }
        }
    }

    struct AutoConfig {
        std::vector<std::string> plugins;
        std::vector<std::string> filters;
    };

    static const std::unordered_map<std::string, std::unordered_map<std::string, AutoConfig>> auto_matrix = {
        {"profile", {
            {"fast", {{"threat_conductor"}, {"noise_suppression_filter"}}},
            {"balanced", {{"threat_conductor", "orbit_link_matrix"}, {"noise_suppression_filter", "exposure_tier_matrix"}}},
            {"deep", {
                {"threat_conductor", "orbit_link_matrix", "contact_lattice", "account_recovery_exposure_probe",
                 "link_outbound_risk_profiler"},
                {"noise_suppression_filter", "exposure_tier_matrix", "contact_canonicalizer", "entity_name_resolver",
                 "triage_priority_filter", "contact_quality_filter", "link_hygiene_filter", "evidence_consistency_filter"},
            }},
            {"max", {
                {"threat_conductor", "orbit_link_matrix", "contact_lattice", "cross_platform_activity_timeline",
                 "identity_fusion_core", "module_capability_matrix", "account_recovery_exposure_probe",
                 "link_outbound_risk_profiler", "username_impersonation_probe"},
                {"noise_suppression_filter", "exposure_tier_matrix", "contact_canonicalizer", "entity_name_resolver",
                 "module_filter_router", "signal_lane_fusion", "pii_signal_classifier", "triage_priority_filter",
                 "contact_quality_filter", "link_hygiene_filter", "evidence_consistency_filter"},
            }},
        }},
        {"surface", {
            {"fast", {{"header_hardening_probe"}, {"noise_suppression_filter"}}},
            {"balanced", {{"header_hardening_probe", "subdomain_risk_atlas"}, {"noise_suppression_filter", "exposure_tier_matrix"}}},
            {"deep", {
                {"header_hardening_probe", "subdomain_risk_atlas", "domain_takeover_risk_probe", "module_capability_matrix",
                 "rdap_lifecycle_inspector", "surface_transport_stability_probe"},
                {"noise_suppression_filter", "exposure_tier_matrix", "takeover_priority_filter",
                 "disclosure_readiness_filter", "triage_priority_filter", "subdomain_attack_path_filter",
                 "evidence_consistency_filter"},
            }},
            {"max", {
                {"header_hardening_probe", "subdomain_risk_atlas", "domain_takeover_risk_probe", "security_txt_analyzer",
                 "threat_conductor", "module_capability_matrix", "rdap_lifecycle_inspector",
                 "surface_transport_stability_probe"},
                {"noise_suppression_filter", "exposure_tier_matrix", "takeover_priority_filter",
                 "disclosure_readiness_filter", "module_filter_router", "signal_lane_fusion",
                 "triage_priority_filter", "subdomain_attack_path_filter", "evidence_consistency_filter"},
            }},
        }},
        {"fusion", {
            {"fast", {{"signal_fusion_core"}, {"signal_lane_fusion"}}},
            {"balanced", {{"signal_fusion_core", "threat_conductor"}, {"signal_lane_fusion", "exposure_tier_matrix"}}},
            {"deep", {
                {"signal_fusion_core", "threat_conductor", "email_pattern_inference", "module_capability_matrix",
                 "account_recovery_exposure_probe", "link_outbound_risk_profiler", "rdap_lifecycle_inspector",
                 "surface_transport_stability_probe"},
                {"signal_lane_fusion", "exposure_tier_matrix", "contact_canonicalizer", "mailbox_provider_profiler",
                 "triage_priority_filter", "contact_quality_filter", "link_hygiene_filter", "evidence_consistency_filter"},
            }},
            {"max", {
                {"signal_fusion_core", "threat_conductor", "email_pattern_inference", "module_capability_matrix",
                 "subdomain_risk_atlas", "cross_platform_activity_timeline", "account_recovery_exposure_probe",
                 "link_outbound_risk_profiler", "username_impersonation_probe", "rdap_lifecycle_inspector",
                 "surface_transport_stability_probe"},
                {"signal_lane_fusion", "exposure_tier_matrix", "contact_canonicalizer", "mailbox_provider_profiler",
                 "module_filter_router", "pii_signal_classifier", "triage_priority_filter",
                 "contact_quality_filter", "link_hygiene_filter", "subdomain_attack_path_filter",
                 "evidence_consistency_filter"},
            }},
        }},
    };

    auto matrix_scope = auto_matrix.find(plan.scope);
    const auto& scope_matrix = (matrix_scope != auto_matrix.end()) ? matrix_scope->second : auto_matrix.at("profile");
    auto matrix_mode = scope_matrix.find(plan.scan_mode);
    const auto& mode_config = (matrix_mode != scope_matrix.end()) ? matrix_mode->second : scope_matrix.at("balanced");

    std::vector<std::string> auto_plugins;
    std::vector<std::string> auto_filters;
    for (const auto& plugin : mode_config.plugins) {
        if (available_plugins.count(plugin)) {
            auto_plugins.push_back(plugin);
        }
    }
    for (const auto& filter : mode_config.filters) {
        if (available_filters.count(filter)) {
            auto_filters.push_back(filter);
        }
    }

    std::vector<std::string> resolved_plugins;
    std::vector<std::string> resolved_filters;
    if (plan.control_mode == "manual") {
        resolved_plugins = manual_plugins;
        resolved_filters = manual_filters;
    } else if (plan.control_mode == "hybrid") {
        resolved_plugins = unique_list([&]() {
            std::vector<std::string> merged = auto_plugins;
            merged.insert(merged.end(), manual_plugins.begin(), manual_plugins.end());
            return merged;
        }());
        resolved_filters = unique_list([&]() {
            std::vector<std::string> merged = auto_filters;
            merged.insert(merged.end(), manual_filters.begin(), manual_filters.end());
            return merged;
        }());
    } else {
        resolved_plugins = unique_list(auto_plugins);
        resolved_filters = unique_list(auto_filters);
    }

    auto apply_conflict = [&](const std::string& keep, const std::string& drop, const std::string& reason, bool cross) {
        auto in_plugins = std::find(resolved_plugins.begin(), resolved_plugins.end(), keep) != resolved_plugins.end();
        auto in_filters = std::find(resolved_filters.begin(), resolved_filters.end(), drop) != resolved_filters.end();
        if (cross) {
            if (in_plugins && in_filters) {
                if (plan.control_mode == "manual") {
                    plan.errors.push_back("Plugin/filter conflict: " + keep + " + " + drop + ". " + reason);
                } else {
                    resolved_filters.erase(
                        std::remove(resolved_filters.begin(), resolved_filters.end(), drop),
                        resolved_filters.end()
                    );
                    plan.warnings.push_back("Plugin/filter conflict resolved: removed filter " + drop + ". " + reason);
                }
            }
            return;
        }
        bool keep_in_plugins = std::find(resolved_plugins.begin(), resolved_plugins.end(), keep) != resolved_plugins.end();
        bool drop_in_plugins = std::find(resolved_plugins.begin(), resolved_plugins.end(), drop) != resolved_plugins.end();
        if (keep_in_plugins && drop_in_plugins) {
            if (plan.control_mode == "manual") {
                plan.errors.push_back("Plugin conflict: " + keep + " + " + drop + ". " + reason);
            } else {
                resolved_plugins.erase(
                    std::remove(resolved_plugins.begin(), resolved_plugins.end(), drop),
                    resolved_plugins.end()
                );
                plan.warnings.push_back("Plugin conflict resolved: removed " + drop + ". " + reason);
            }
        }
    };

    // Conflicts.
    apply_conflict("signal_fusion_core", "identity_fusion_core",
                   "Both plugins provide fusion-core aggregation; select only one.", false);

    auto apply_filter_conflict = [&](const std::string& keep, const std::string& drop, const std::string& reason) {
        bool keep_in_filters = std::find(resolved_filters.begin(), resolved_filters.end(), keep) != resolved_filters.end();
        bool drop_in_filters = std::find(resolved_filters.begin(), resolved_filters.end(), drop) != resolved_filters.end();
        if (keep_in_filters && drop_in_filters) {
            if (plan.control_mode == "manual") {
                plan.errors.push_back("Filter conflict: " + keep + " + " + drop + ". " + reason);
            } else {
                resolved_filters.erase(
                    std::remove(resolved_filters.begin(), resolved_filters.end(), drop),
                    resolved_filters.end()
                );
                plan.warnings.push_back("Filter conflict resolved: removed " + drop + ". " + reason);
            }
        }
    };
    apply_filter_conflict("pii_signal_classifier", "sensitive_lexicon_guard",
                          "Both filters enforce overlapping sensitivity suppression; select one.");

    apply_conflict("module_capability_matrix", "module_filter_router",
                   "Module matrix plugin conflicts with module router filter in strict/manual mode.", true);

    const std::unordered_map<std::string, int> plugin_budget = {
        {"fast", 2}, {"balanced", 4}, {"deep", 8}, {"max", 99},
    };
    const std::unordered_map<std::string, int> filter_budget = {
        {"fast", 2}, {"balanced", 4}, {"deep", 8}, {"max", 99},
    };
    int max_plugins = plugin_budget.at(plan.scan_mode);
    int max_filters = filter_budget.at(plan.scan_mode);
    if (static_cast<int>(resolved_plugins.size()) > max_plugins &&
        (plan.control_mode == "manual" || plan.control_mode == "hybrid")) {
        plan.errors.push_back(
            "Mode '" + plan.scan_mode + "' allows at most " + std::to_string(max_plugins) +
            " plugins, got " + std::to_string(resolved_plugins.size()) + "."
        );
    }
    if (static_cast<int>(resolved_filters.size()) > max_filters &&
        (plan.control_mode == "manual" || plan.control_mode == "hybrid")) {
        plan.errors.push_back(
            "Mode '" + plan.scan_mode + "' allows at most " + std::to_string(max_filters) +
            " filters, got " + std::to_string(resolved_filters.size()) + "."
        );
    }

    plan.plugins = unique_list(resolved_plugins);
    plan.filters = unique_list(resolved_filters);
    return plan;
}

} // namespace silicore::extensions
