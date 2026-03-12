#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_gitlab() {
    PlatformConfig cfg;
    cfg.name = "GitLab";
    cfg.url = "https://gitlab.com/{username}";
    cfg.url_probe = "https://gitlab.com/api/v4/users?username={username}";
    cfg.detection_methods = {
        "message"
    };
    cfg.exists_statuses = {
    };
    cfg.not_found_statuses = {
    };
    cfg.error_messages = {
        "["
    };
    cfg.error_url = "";
    cfg.regex_check = "";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.9;
    return cfg;
}

} // namespace silicore::collect

