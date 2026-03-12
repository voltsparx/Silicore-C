#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_roblox() {
    PlatformConfig cfg;
    cfg.name = "Roblox";
    cfg.url = "https://www.roblox.com/users/profile?username={username}";
    cfg.url_probe = "https://www.roblox.com/users/profile?username={username}";
    cfg.detection_methods = {
        "status_code"
    };
    cfg.exists_statuses = {
        200
    };
    cfg.not_found_statuses = {
    };
    cfg.error_messages = {
    };
    cfg.error_url = "";
    cfg.regex_check = "";
    cfg.headers = {
    };
    cfg.request_method = "HEAD";
    cfg.confidence_weight = 0.65;
    return cfg;
}

} // namespace silicore::collect

