#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_quora() {
    PlatformConfig cfg;
    cfg.name = "Quora";
    cfg.url = "https://www.quora.com/profile/{username}";
    cfg.url_probe = "https://www.quora.com/profile/{username}";
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
    cfg.regex_check = "^[A-Za-z0-9-]{2,60}$";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.76;
    return cfg;
}

} // namespace silicore::collect

