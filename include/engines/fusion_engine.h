#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>

namespace silicore::engines {

class FusionEngine {
public:
    nlohmann::json fuse_profile_domain(
        const nlohmann::json& profile_data,
        const nlohmann::json& domain_data
    );

    nlohmann::json generate_graph(const nlohmann::json& fused_data);

private:
    std::unordered_map<std::string, nlohmann::json> cache_;

    static int safe_int(const nlohmann::json& value, int fallback = 0);
    static std::string cache_key(const nlohmann::json& profile_data, const nlohmann::json& domain_data);
};

} // namespace silicore::engines
