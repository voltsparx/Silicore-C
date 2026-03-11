#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_medium() {
    PlatformConfig cfg;
    cfg.name = "Medium";
    cfg.url = "https://medium.com/@{username}";
    cfg.url_probe = "https://medium.com/@{username}";
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
    cfg.confidence_weight = 0.8;
    return cfg;
}

} // namespace silicore::collect
