#pragma once

#include "collect/domain_collector.h"
#include "domain/entity.h"
#include "extensions/plugin_loader.h"
#include "extensions/filter_loader.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace silicore::reporting {

using json = nlohmann::json;

json build_profile_results_json(const std::vector<domain::ProfileEntity>& profiles);
json build_domain_result_json(const collect::DomainScanResult& result);
json build_report_payload(
    const std::string& target,
    const std::vector<domain::ProfileEntity>& profiles,
    const collect::DomainScanResult* domain_result,
    const std::vector<extensions::PluginResult>& plugins,
    const json* fusion_result,
    const std::vector<extensions::FilterResult>& filters,
    const std::string& mode
);

std::string render_cli_report(const json& payload);
std::string render_html_report(const json& payload);
std::string render_csv_report(const json& payload);

void write_json_report(const json& payload, const std::filesystem::path& out_path);
void write_text_report(const std::string& text, const std::filesystem::path& out_path);

std::string sanitize_target(const std::string& target);

} // namespace silicore::reporting

