#pragma once

#include "domain/entity.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace silicore::analyze {

struct Issue {
    std::string scope;
    std::string severity;
    std::string title;
    std::string evidence;
    std::string recommendation;
};

struct IssueSummary {
    int total = 0;
    std::unordered_map<std::string, int> severity_breakdown;
    int risk_score = 0;
};

std::vector<Issue> assess_profile_exposure(const std::vector<domain::ProfileEntity>& results);
std::vector<Issue> assess_domain_exposure(
    const std::string& domain,
    const std::unordered_map<std::string, std::string>& https_headers,
    bool http_redirects_to_https,
    int certificate_transparency_count
);
IssueSummary summarize_issues(const std::vector<Issue>& issues);

} // namespace silicore::analyze
