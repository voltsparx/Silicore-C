#include "core/collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_dockerhub() {
    PlatformConfig cfg;
    cfg.name = "DockerHub";
    cfg.url = "https://hub.docker.com/u/{username}";
    cfg.url_probe = "https://hub.docker.com/v2/users/{username}/";
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
    cfg.confidence_weight = 0.75;
    return cfg;
}

} // namespace silicore::collect
