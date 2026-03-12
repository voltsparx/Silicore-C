#include "collect/platform_schema.h"

namespace silicore::collect {

PlatformConfig build_platform_tryhackme() {
    PlatformConfig cfg;
    cfg.name = "TryHackMe";
    cfg.url = "https://tryhackme.com/p/{username}";
    cfg.url_probe = "https://tryhackme.com/p/{username}";
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

