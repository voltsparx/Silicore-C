#pragma once

#include "analyze/correlator.h"
#include "analyze/exposure.h"
#include "collect/domain_collector.h"
#include "domain/entity.h"

#include <string>
#include <vector>

namespace silicore::analyze {

std::string build_nano_brief(
    const std::string& username,
    const std::vector<domain::ProfileEntity>& profile_results,
    const std::string& domain,
    const collect::DomainScanResult* domain_result,
    const std::vector<Issue>& issues,
    const IssueSummary& issue_summary,
    const CorrelationResult& correlation
);

} // namespace silicore::analyze
