#include "core/orchestrator.h"

#include <algorithm>

namespace silicore {

Orchestrator::Orchestrator(std::vector<collect::PlatformConfig> platforms)
    : scanner_(std::move(platforms)) {}

ProfileRunResult Orchestrator::run_profile(
    const std::string& username,
    const ExecutionPolicy&,
    int timeout_ms,
    int concurrency,
    const std::string& proxy_url
) {
    ProfileRunResult result;
    int adjusted = stabilizer_.adjust_concurrency(concurrency, health_monitor_.snapshot());
    result.scan_result = scanner_.scan(username, adjusted, timeout_ms, proxy_url);
    for (const auto& profile : result.scan_result.profiles) {
        engines::EngineResult er;
        er.name = profile.platform;
        er.execution_time = static_cast<double>(profile.response_time_ms) / 1000.0;
        er.status = profile.http_status > 0 ? engines::EngineStatus::Success : engines::EngineStatus::Failed;
        if (er.status == engines::EngineStatus::Failed) {
            er.error = profile.context;
        }
        health_monitor_.record(er);
    }
    return result;
}

SurfaceRunResult Orchestrator::run_surface(
    const std::string& domain,
    const ExecutionPolicy& policy,
    int timeout_ms,
    const std::string& proxy_url
) {
    SurfaceRunResult result;
    collect::DomainScanOptions options;
    options.timeout_ms = timeout_ms;
    options.include_ct = true;
    options.include_rdap = true;
    options.max_subdomains = 250;
    options.proxy_url = proxy_url;
    int adjusted = stabilizer_.adjust_concurrency(policy.concurrency, health_monitor_.snapshot());
    options.concurrency = std::max(4, std::min(adjusted, 64));
    result.scan_result = collect::collect_domain_surface(domain, options);
    engines::EngineResult er;
    er.name = "surface";
    er.execution_time = static_cast<double>(result.scan_result.https.elapsed_ms) / 1000.0;
    er.status = result.scan_result.target_domain.empty() ? engines::EngineStatus::Failed : engines::EngineStatus::Success;
    if (er.status == engines::EngineStatus::Failed) {
        er.error = "invalid_domain";
    }
    health_monitor_.record(er);
    return result;
}

} // namespace silicore
