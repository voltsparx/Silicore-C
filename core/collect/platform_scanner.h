#pragma once

#include "core/collect/platform_schema.h"
#include "core/domain/entity.h"
#include "core/engines/async_engine.h"

#include <string>
#include <vector>

namespace silicore::collect {

struct ProfileScanResult {
    std::vector<domain::ProfileEntity> profiles;
};

class PlatformScanner {
public:
    explicit PlatformScanner(std::vector<PlatformConfig> configs);

    ProfileScanResult scan(
        const std::string& username,
        int concurrency_limit,
        int timeout_ms,
        const std::string& proxy_url
    );

    const std::vector<PlatformConfig>& platforms() const { return configs_; }

private:
    std::vector<PlatformConfig> configs_;
};

std::string classify_profile_status(
    const PlatformConfig& cfg,
    const engines::HttpResponse& response,
    const std::string& username
);

} // namespace silicore::collect
