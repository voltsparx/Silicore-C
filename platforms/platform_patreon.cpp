#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_patreon() {
    PlatformConfig cfg;
    cfg.name = "Patreon";
    cfg.url = "https://www.patreon.com/{username}";
    cfg.url_probe = "https://www.patreon.com/{username}";
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
    cfg.confidence_weight = 0.75;
    return cfg;
}

} // namespace silicore::collect

