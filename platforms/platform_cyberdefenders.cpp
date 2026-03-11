#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_cyberdefenders() {
    PlatformConfig cfg;
    cfg.name = "CyberDefenders";
    cfg.url = "https://cyberdefenders.org/p/{username}";
    cfg.url_probe = "https://cyberdefenders.org/p/{username}";
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
    cfg.regex_check = "^[^\\\\/:*?\\"<>|@]{3,50}$";
    cfg.headers = {
    };
    cfg.request_method = "GET";
    cfg.confidence_weight = 0.86;
    return cfg;
}

} // namespace silicore::collect
