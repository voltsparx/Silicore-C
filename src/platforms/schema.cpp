#include <nlohmann/json.hpp>

namespace silicore::collect {

using json = nlohmann::json;

json platform_schema_overview() {
    return json{
        {"required", {"name", "url", "url_probe", "detection_methods", "request_method"}},
        {"optional", {"exists_statuses", "not_found_statuses", "error_messages", "error_url", "regex_check", "headers", "confidence_weight"}},
        {"example", {
            {"name", "Example"},
            {"url", "https://example.com/{username}"},
            {"url_probe", "https://example.com/{username}"},
            {"detection_methods", {"status_code"}},
            {"request_method", "HEAD"},
            {"confidence_weight", 0.7}
        }}
    };
}

} // namespace silicore::collect
