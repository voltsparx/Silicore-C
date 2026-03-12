#pragma once

#include "domain/entity.h"

#include <map>
#include <string>
#include <vector>

namespace silicore::analyze {

struct ErrorDetail {
    std::string platform;
    std::string status;
    std::string url;
    int http_status = 0;
    long response_time_ms = 0;
    std::string context;
};

struct TargetSnapshot {
    int total_results = 0;
    int found_count = 0;
    int error_count = 0;
    double coverage_ratio = 0.0;
    double avg_found_confidence = 0.0;
    double avg_found_response_time_ms = 0.0;
    double avg_error_response_time_ms = 0.0;
    std::map<std::string, int> status_breakdown;
    std::vector<std::string> found_platforms;
    std::vector<std::string> profile_links;
    std::vector<std::string> emails;
    std::vector<std::string> email_domains;
    std::vector<std::string> phones;
    std::vector<std::string> names;
    std::vector<std::string> mentions;
    std::vector<std::string> external_links;
    std::vector<std::string> external_link_domains;
    std::vector<std::string> bios;
    std::vector<ErrorDetail> errors;
};

std::vector<domain::ProfileEntity> focused_profile_rows(const std::vector<domain::ProfileEntity>& results);
std::vector<domain::ProfileEntity> found_profile_rows(const std::vector<domain::ProfileEntity>& results);
std::vector<domain::ProfileEntity> error_profile_rows(const std::vector<domain::ProfileEntity>& results);
TargetSnapshot summarize_target_intel(const std::vector<domain::ProfileEntity>& results);

} // namespace silicore::analyze
