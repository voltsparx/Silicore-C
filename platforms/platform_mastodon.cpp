#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_mastodon() {
    PlatformConfig cfg;
    cfg.name = "Mastodon";
    cfg.url = "https://mastodon.social/@{username}";
    cfg.url_probe = "https://mastodon.social/@{username}";
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
    cfg.confidence_weight = 0.82;
    return cfg;
}

} // namespace silicore::collect
