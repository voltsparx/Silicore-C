#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_archive_org() {
    PlatformConfig cfg;
    cfg.name = "Archive.org";
    cfg.url = "https://archive.org/details/@{username}";
    cfg.url_probe = "https://archive.org/details/@{username}?noscript=true";
    cfg.detection_methods = {
        "message"
    };
    cfg.exists_statuses = {
    };
    cfg.not_found_statuses = {
    };
    cfg.error_messages = {
        "could not fetch an account with user item identifier",
        "The resource could not be found",
        "Internet Archive services are temporarily offline"
    };
    cfg.error_url = "";
    cfg.regex_check = "";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.8;
    return cfg;
}

} // namespace silicore::collect

