#pragma once

#include "analyze/correlator.h"
#include "analyze/exposure.h"
#include "collect/domain_collector.h"
#include "domain/entity.h"
#include "extensions/filter_loader.h"
#include "extensions/plugin_loader.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace silicore::reporting {

using json = nlohmann::json;

json build_profile_results_json(const std::vector<domain::ProfileEntity>& profiles);
json build_domain_result_json(const collect::DomainScanResult& result);

struct ReportInputs {
    std::string target;
    std::vector<domain::ProfileEntity> profiles;
    const collect::DomainScanResult* domain_result = nullptr;
    analyze::CorrelationResult correlation;
    std::vector<analyze::Issue> issues;
    analyze::IssueSummary issue_summary;
    std::vector<extensions::PluginResult> plugins;
    std::vector<std::string> plugin_errors;
    std::vector<extensions::FilterResult> filters;
    std::vector<std::string> filter_errors;
    json fused_intel;
    json fusion_graph;
    json intelligence_bundle;
    std::string narrative;
    std::string mode = "profile";
};

json build_report_payload(const ReportInputs& input);

std::string render_cli_report(const json& payload);
std::string render_html_report(const json& payload);
std::string render_csv_report(const json& payload);
void write_csv_reports(const json& payload, const std::filesystem::path& cli_path);

void write_json_report(const json& payload, const std::filesystem::path& out_path);
void write_text_report(const std::string& text, const std::filesystem::path& out_path);

std::string sanitize_target(const std::string& target);

} // namespace silicore::reporting

