#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_crowdin() {
    PlatformConfig cfg;
    cfg.name = "Crowdin";
    cfg.url = "https://crowdin.com/profile/{username}";
    cfg.url_probe = "https://crowdin.com/profile/{username}";
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
    cfg.regex_check = "^[a-zA-Z0-9._-]{2,255}$";
    cfg.headers = {
    };
    cfg.request_method = "HEAD";
    cfg.confidence_weight = 0.86;
    return cfg;
}

} // namespace silicore::collect

