#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_replit() {
    PlatformConfig cfg;
    cfg.name = "Replit";
    cfg.url = "https://replit.com/@{username}";
    cfg.url_probe = "https://replit.com/@{username}";
    cfg.detection_methods = {
        "status_code"
    };
    cfg.exists_statuses = {
        200,
        301,
        302
    };
    cfg.not_found_statuses = {
        404
    };
    cfg.error_messages = {
    };
    cfg.error_url = "";
    cfg.regex_check = "^[A-Za-z0-9_-]{1,32}$";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.83;
    return cfg;
}

} // namespace silicore::collect
