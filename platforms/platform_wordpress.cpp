#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_wordpress() {
    PlatformConfig cfg;
    cfg.name = "WordPress";
    cfg.url = "https://{username}.wordpress.com";
    cfg.url_probe = "https://{username}.wordpress.com";
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
