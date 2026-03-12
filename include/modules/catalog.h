#pragma once

#include <string>
#include <vector>

namespace silicore::modules {

struct ModuleEntry {
    std::string id;
    std::string title;
    std::string kind;
    std::vector<std::string> tags;
    int power_score = 0;
};

std::vector<ModuleEntry> plugin_modules();
std::vector<ModuleEntry> filter_modules();
std::vector<ModuleEntry> all_modules();
const ModuleEntry* find_module(const std::string& id);

} // namespace silicore::modules
