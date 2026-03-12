#pragma once

#include "domain/entity.h"

#include <map>
#include <string>
#include <vector>

namespace silicore::analyze {

struct ResponseTimeStats {
    int min_ms = -1;
    int max_ms = -1;
    int avg_ms = -1;
};

struct CorrelationResult {
    std::map<std::string, std::vector<std::string>> shared_bios;
    std::map<std::string, std::vector<std::string>> shared_emails;
    std::map<std::string, std::vector<std::string>> shared_phones;
    std::map<std::string, std::vector<std::string>> shared_links;
    std::map<std::string, std::vector<std::string>> shared_mentions;
    std::map<std::string, std::vector<std::string>> confidence_cluster_map;
    std::vector<std::string> confidence_clusters;
    std::map<std::string, int> status_distribution;
    ResponseTimeStats response_time_stats;
    int average_confidence = 0;
    int identity_overlap_score = 0;
};

CorrelationResult correlate(const std::vector<domain::ProfileEntity>& results);

} // namespace silicore::analyze
