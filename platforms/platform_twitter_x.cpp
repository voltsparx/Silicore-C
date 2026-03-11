#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_twitter_x() {
    PlatformConfig cfg;
    cfg.name = "Twitter/X";
    cfg.url = "https://x.com/{username}";
    cfg.url_probe = "https://x.com/{username}";
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
    cfg.confidence_weight = 0.9;
    return cfg;
}

} // namespace silicore::collect
