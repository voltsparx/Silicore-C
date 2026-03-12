#include <nlohmann/json.hpp>

namespace silicore::intel {

using json = nlohmann::json;

json parallel_workers_details() {
    return json{
        {"id", "parallel_workers"},
        {"focus", "parallel task scheduling"},
        {"controls", {"concurrency", "preset"}},
        {"notes", "Used for domain surface tasks and plugin execution."}
    };
}

} // namespace silicore::intel
