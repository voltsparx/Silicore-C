#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_codeberg() {
    PlatformConfig cfg;
    cfg.name = "Codeberg";
    cfg.url = "https://codeberg.org/{username}";
    cfg.url_probe = "https://codeberg.org/{username}";
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
    cfg.confidence_weight = 0.83;
    return cfg;
}

} // namespace silicore::collect

