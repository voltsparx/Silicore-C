#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_buymeacoffee() {
    PlatformConfig cfg;
    cfg.name = "BuyMeACoffee";
    cfg.url = "https://buymeacoffee.com/{username}";
    cfg.url_probe = "https://buymeacoffee.com/{username}";
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
    cfg.regex_check = "^[A-Za-z0-9_-]{1,50}$";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.78;
    return cfg;
}

} // namespace silicore::collect

