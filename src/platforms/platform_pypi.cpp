#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_pypi() {
    PlatformConfig cfg;
    cfg.name = "PyPI";
    cfg.url = "https://pypi.org/user/{username}/";
    cfg.url_probe = "https://pypi.org/user/{username}/";
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
    cfg.confidence_weight = 0.85;
    return cfg;
}

} // namespace silicore::collect

