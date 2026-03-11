#include "core/engines/fusion_engine.h"

#include <algorithm>
#include <numeric>

namespace silicore::engines {

int FusionEngine::safe_int(const nlohmann::json& value, int fallback) {
    if (value.is_boolean()) {
        return value.get<bool>() ? 1 : 0;
    }
    if (value.is_number_integer()) {
        return value.get<int>();
    }
    if (value.is_number_float()) {
        return static_cast<int>(value.get<double>());
    }
    if (value.is_string()) {
        try {
            return std::stoi(value.get<std::string>());
        } catch (const std::exception&) {
            return fallback;
        }
    }
    return fallback;
}

std::string FusionEngine::cache_key(const nlohmann::json& profile_data, const nlohmann::json& domain_data) {
    const auto profile_target = profile_data.value("target", "");
    std::string domain_target;
    if (domain_data.contains("domain_result") && domain_data["domain_result"].is_object()) {
        domain_target = domain_data["domain_result"].value("target", "");
    } else {
        domain_target = domain_data.value("target", "");
    }
    return profile_target + "|" + domain_target;
}

nlohmann::json FusionEngine::fuse_profile_domain(
    const nlohmann::json& profile_data,
    const nlohmann::json& domain_data
) {
    const auto key = cache_key(profile_data, domain_data);
    auto cached = cache_.find(key);
    if (cached != cache_.end()) {
        return cached->second;
    }

    const auto results = profile_data.value("results", nlohmann::json::array());
    std::vector<int> confidence_values;
    int found_count = 0;
    for (const auto& item : results) {
        if (item.value("status", "") == "FOUND") {
            found_count++;
            confidence_values.push_back(safe_int(item.value("confidence", 0)));
        }
    }

    int avg_confidence = 0;
    if (!confidence_values.empty()) {
        int sum = std::accumulate(confidence_values.begin(), confidence_values.end(), 0);
        avg_confidence = sum / static_cast<int>(confidence_values.size());
    }

    int overlap_score = 0;
    if (profile_data.contains("correlation")) {
        overlap_score = safe_int(profile_data["correlation"].value("identity_overlap_score", 0));
    }

    nlohmann::json domain_result;
    if (domain_data.contains("domain_result") && domain_data["domain_result"].is_object()) {
        domain_result = domain_data["domain_result"];
    } else {
        domain_result = domain_data;
    }

    int subdomain_count = 0;
    if (domain_result.contains("subdomains") && domain_result["subdomains"].is_array()) {
        subdomain_count = static_cast<int>(domain_result["subdomains"].size());
    }

    int resolved_ip_count = 0;
    if (domain_result.contains("resolved_addresses") && domain_result["resolved_addresses"].is_array()) {
        resolved_ip_count = static_cast<int>(domain_result["resolved_addresses"].size());
    }

    int https_status = safe_int(domain_result.value("https", nlohmann::json::object()).value("status", 0));

    int risk_score = 0;
    if (profile_data.contains("issue_summary")) {
        risk_score = safe_int(profile_data["issue_summary"].value("risk_score", 0));
    }
    if (risk_score == 0 && domain_data.contains("issue_summary")) {
        risk_score = safe_int(domain_data["issue_summary"].value("risk_score", 0));
    }

    int confidence_score = std::clamp(
        static_cast<int>(
            avg_confidence * 0.45
            + overlap_score * 0.35
            + std::min(subdomain_count, 50) * 0.3
            + std::min(resolved_ip_count, 10) * 0.5
            - risk_score * 0.25
        ),
        0,
        100
    );

    nlohmann::json anomalies = nlohmann::json::array();
    if (found_count > 0 && overlap_score < 12) {
        anomalies.push_back("weak_identity_overlap");
    }
    if (subdomain_count >= 100) {
        anomalies.push_back("broad_attack_surface");
    }
    if (https_status != 200 && https_status != 301 && https_status != 302) {
        anomalies.push_back("unstable_https_surface");
    }
    if (risk_score >= 60) {
        anomalies.push_back("high_exposure_risk");
    }

    nlohmann::json fused = {
        {"target", {
            {"username", profile_data.value("target", "")},
            {"domain", domain_result.value("target", "")},
        }},
        {"profile", {
            {"found_profiles", found_count},
            {"average_confidence", avg_confidence},
            {"identity_overlap_score", overlap_score},
        }},
        {"domain", {
            {"subdomain_count", subdomain_count},
            {"resolved_ip_count", resolved_ip_count},
            {"https_status", https_status},
        }},
        {"risk", {
            {"risk_score", risk_score},
        }},
        {"confidence_score", confidence_score},
        {"anomalies", anomalies},
    };

    cache_[key] = fused;
    return fused;
}

nlohmann::json FusionEngine::generate_graph(const nlohmann::json& fused_data) {
    const auto target = fused_data.value("target", nlohmann::json::object());
    const auto username = target.value("username", "unknown-user");
    const auto domain = target.value("domain", "unknown-domain");
    const auto risk_score = fused_data.value("risk", nlohmann::json::object()).value("risk_score", 0);

    nlohmann::json nodes = nlohmann::json::array({
        {{"id", "user:" + username}, {"label", username}, {"type", "username"}},
        {{"id", "domain:" + domain}, {"label", domain}, {"type", "domain"}},
        {{"id", "risk:" + std::to_string(risk_score)}, {"label", "risk=" + std::to_string(risk_score)}, {"type", "risk"}},
    });

    nlohmann::json edges = nlohmann::json::array({
        {{"source", "user:" + username}, {"target", "domain:" + domain}, {"relation", "correlated_with"}},
        {{"source", "domain:" + domain}, {"target", "risk:" + std::to_string(risk_score)}, {"relation", "assessed_as"}},
    });

    for (const auto& anomaly : fused_data.value("anomalies", nlohmann::json::array())) {
        const auto label = anomaly.get<std::string>();
        const auto anomaly_id = "anomaly:" + label;
        nodes.push_back({{"id", anomaly_id}, {"label", label}, {"type", "anomaly"}});
        edges.push_back({{"source", "domain:" + domain}, {"target", anomaly_id}, {"relation", "flagged"}});
    }

    return {
        {"nodes", nodes},
        {"edges", edges},
    };
}

} // namespace silicore::engines
