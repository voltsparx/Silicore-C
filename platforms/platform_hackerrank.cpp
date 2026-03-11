#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_hackerrank() {
    PlatformConfig cfg;
    cfg.name = "HackerRank";
    cfg.url = "https://www.hackerrank.com/{username}";
    cfg.url_probe = "https://www.hackerrank.com/{username}";
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
    cfg.regex_check = "^[A-Za-z0-9_-]{1,30}$";
    cfg.headers = {
    };
    cfg.request_method = "HEAD";
    cfg.confidence_weight = 0.82;
    return cfg;
}

} // namespace silicore::collect
