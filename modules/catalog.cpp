#include "modules/catalog.h"

namespace silicore::modules {

std::vector<ModuleEntry> all_modules() {
    auto plugins = plugin_modules();
    auto filters = filter_modules();
    std::vector<ModuleEntry> out;
    out.reserve(plugins.size() + filters.size());
    out.insert(out.end(), plugins.begin(), plugins.end());
    out.insert(out.end(), filters.begin(), filters.end());
    return out;
}

} // namespace silicore::modules

