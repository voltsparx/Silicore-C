#include <nlohmann/json.hpp>

namespace silicore::intel {

using json = nlohmann::json;

json plugin_module_system_details() {
    return json{
        {"id", "plugin_module_system"},
        {"focus", "dlopen/dlsym plugin ABI"},
        {"inputs", {"json context"}},
        {"outputs", {"json output", "severity"}}
    };
}

} // namespace silicore::intel
