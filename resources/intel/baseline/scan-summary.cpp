#include <nlohmann/json.hpp>

namespace silicore::intel {

using json = nlohmann::json;

json baseline_scan_summary() {
    return json{
        {"version", "1.0"},
        {"theme", "Crystal Lattice"},
        {"platforms", 70},
        {"engines", {"async", "parallel", "thread", "stabilizer"}},
        {"outputs", {"txt", "json", "html"}}
    };
}

} // namespace silicore::intel
