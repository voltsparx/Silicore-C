#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_bitbucket() {
    PlatformConfig cfg;
    cfg.name = "Bitbucket";
    cfg.url = "https://bitbucket.org/{username}";
    cfg.url_probe = "https://bitbucket.org/{username}";
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
    cfg.regex_check = "^[a-zA-Z0-9-_]{1,30}$";
    cfg.headers = {
    };
    cfg.request_method = "HEAD";
    cfg.confidence_weight = 0.85;
    return cfg;
}

} // namespace silicore::collect
