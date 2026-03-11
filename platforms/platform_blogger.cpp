#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_blogger() {
    PlatformConfig cfg;
    cfg.name = "Blogger";
    cfg.url = "https://{username}.blogspot.com";
    cfg.url_probe = "https://{username}.blogspot.com";
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
