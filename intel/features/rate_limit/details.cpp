#include <nlohmann/json.hpp>

namespace silicore::intel {

using json = nlohmann::json;

json rate_limit_details() {
    return json{
        {"id", "rate_limit"},
        {"focus", "request pacing and backoff"},
        {"controls", {"timeout", "stabilizer"}},
        {"notes", "Limits bursts to keep results stable."}
    };
}

} // namespace silicore::intel
