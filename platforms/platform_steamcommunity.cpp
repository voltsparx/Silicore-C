#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_steamcommunity() {
    PlatformConfig cfg;
    cfg.name = "SteamCommunity";
    cfg.url = "https://steamcommunity.com/id/{username}";
    cfg.url_probe = "https://steamcommunity.com/id/{username}";
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
    cfg.regex_check = "^[A-Za-z0-9_-]{2,32}$";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.8;
    return cfg;
}

} // namespace silicore::collect
