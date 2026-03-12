#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_producthunt() {
    PlatformConfig cfg;
    cfg.name = "ProductHunt";
    cfg.url = "https://www.producthunt.com/@{username}";
    cfg.url_probe = "https://www.producthunt.com/@{username}";
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
    cfg.regex_check = "^[A-Za-z0-9_-]{1,40}$";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.79;
    return cfg;
}

} // namespace silicore::collect

