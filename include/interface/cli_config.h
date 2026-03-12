#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace silicore::interface {

struct ProfilePreset {
    int timeout_seconds = 0;
    int max_concurrency = 0;
    std::string source_profile;
    int max_platforms = 0;
};

struct SurfacePreset {
    int timeout_seconds = 0;
    int max_subdomains = 0;
};

const std::unordered_map<std::string, ProfilePreset>& profile_presets();
const std::unordered_map<std::string, SurfacePreset>& surface_presets();
const std::unordered_set<std::string>& extension_control_modes();

bool is_valid_extension_control(const std::string& mode);
std::string normalize_preset_name(const std::string& name);

const std::unordered_map<std::string, std::unordered_set<std::string>>& prompt_keywords();

} // namespace silicore::interface
