#include "modules/catalog.h"

#include <algorithm>

namespace silicore::modules {

const ModuleEntry* find_module(const std::string& id) {
    static std::vector<ModuleEntry> cache = all_modules();
    auto it = std::find_if(cache.begin(), cache.end(), [&](const ModuleEntry& entry) {
        return entry.id == id;
    });
    if (it == cache.end()) {
        return nullptr;
    }
    return &(*it);
}

} // namespace silicore::modules

