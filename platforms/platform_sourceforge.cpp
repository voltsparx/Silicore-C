#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_sourceforge() {
    PlatformConfig cfg;
    cfg.name = "SourceForge";
    cfg.url = "https://sourceforge.net/u/{username}/profile";
    cfg.url_probe = "https://sourceforge.net/u/{username}/profile";
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
    cfg.confidence_weight = 0.7;
    return cfg;
}

} // namespace silicore::collect
