#pragma once

#include "collect/domain_collector.h"
#include "collect/platform_scanner.h"
#include "execution_policy.h"
#include "engines/health_monitor.h"
#include "engines/stabilizer_engine.h"

namespace silicore {

struct ProfileRunResult {
    collect::ProfileScanResult scan_result;
};

struct SurfaceRunResult {
    collect::DomainScanResult scan_result;
};

class Orchestrator {
public:
    explicit Orchestrator(std::vector<collect::PlatformConfig> platforms);

    ProfileRunResult run_profile(
        const std::string& username,
        const ExecutionPolicy& policy,
        int timeout_ms,
        int concurrency,
        const std::string& proxy_url
    );

    SurfaceRunResult run_surface(
        const std::string& domain,
        const ExecutionPolicy& policy,
        int timeout_ms,
        const std::string& proxy_url
    );

private:
    collect::PlatformScanner scanner_;
    engines::EngineHealthMonitor health_monitor_;
    engines::StabilizerEngine stabilizer_;
};

} // namespace silicore

