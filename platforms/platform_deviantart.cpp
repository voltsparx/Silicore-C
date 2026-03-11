#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_deviantart() {
    PlatformConfig cfg;
    cfg.name = "DeviantArt";
    cfg.url = "https://www.deviantart.com/{username}";
    cfg.url_probe = "https://www.deviantart.com/{username}";
    cfg.detection_methods = {
        "status_code"
    };
    cfg.exists_statuses = {
        200
    };
    cfg.not_found_statuses = {
        404
    };
    cfg.error_messages = {
    };
    cfg.error_url = "";
    cfg.regex_check = "";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.81;
    return cfg;
}

} // namespace silicore::collect
