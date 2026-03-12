#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_github() {
    PlatformConfig cfg;
    cfg.name = "GitHub";
    cfg.url = "https://github.com/{username}";
    cfg.url_probe = "https://github.com/{username}";
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
    cfg.regex_check = "^[a-zA-Z0-9](?:[a-zA-Z0-9]|-(?=[a-zA-Z0-9])){0,38}$";
    cfg.headers = {
    };
    cfg.request_method = "HEAD";
    cfg.confidence_weight = 0.95;
    return cfg;
}

} // namespace silicore::collect

