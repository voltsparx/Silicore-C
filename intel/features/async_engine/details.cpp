#include <nlohmann/json.hpp>

namespace silicore::intel {

using json = nlohmann::json;

json async_engine_details() {
    return json{
        {"id", "async_engine"},
        {"focus", "libcurl multi with curl_multi_wait"},
        {"signals", {"status_code", "headers", "final_url", "elapsed_ms"}},
        {"controls", {"timeout", "concurrency"}}
    };
}

} // namespace silicore::intel
