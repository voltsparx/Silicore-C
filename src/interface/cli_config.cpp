#include "interface/cli_config.h"

#include "utils/strings.h"

namespace silicore::interface {

const std::unordered_map<std::string, ProfilePreset>& profile_presets() {
    static const std::unordered_map<std::string, ProfilePreset> presets = {
        {"safe", {10, 8, "fast", 25}},
        {"fast", {10, 8, "fast", 25}},
        {"quick", {12, 10, "fast", 25}},
        {"balanced", {20, 20, "balanced", 45}},
        {"deep", {35, 35, "deep", 60}},
        {"aggressive", {50, 50, "max", 70}},
        {"max", {50, 50, "max", 70}},
    };
    return presets;
}

const std::unordered_map<std::string, SurfacePreset>& surface_presets() {
    static const std::unordered_map<std::string, SurfacePreset> presets = {
        {"quick", {10, 60}},
        {"balanced", {20, 250}},
        {"deep", {30, 700}},
    };
    return presets;
}

const std::unordered_set<std::string>& extension_control_modes() {
    static const std::unordered_set<std::string> modes = {"auto", "manual", "hybrid"};
    return modes;
}

bool is_valid_extension_control(const std::string& mode) {
    std::string key = utils::to_lower(utils::trim(mode));
    return extension_control_modes().count(key) > 0;
}

std::string normalize_preset_name(const std::string& name) {
    std::string key = utils::to_lower(utils::trim(name));
    if (key == "standard") {
        key = "balanced";
    }
    if (key.empty()) {
        key = "balanced";
    }
    return key;
}

const std::unordered_map<std::string, std::unordered_set<std::string>>& prompt_keywords() {
    static const std::unordered_map<std::string, std::unordered_set<std::string>> keywords = {
        {"profile", {"profile", "scan", "social", "persona", "identity", "username", "handle", "account"}},
        {"surface", {"surface", "domain", "asset", "infra", "recon", "footprint"}},
        {"fusion", {"fusion", "full", "combo", "allscan", "stack"}},
        {"orchestrate", {"orchestrate", "orch", "pipeline", "orchestration"}},
        {"anonymity", {"anonymity", "anon", "privacy", "routing", "tor"}},
        {"live", {"live", "dashboard", "watch", "monitor"}},
        {"keywords", {"keywords", "keyword", "verbs", "commands", "lexicon"}},
        {"plugins", {"plugins", "plugin", "addons", "extensions"}},
        {"filters", {"filters", "filter", "sanitize", "pii", "redact"}},
        {"modules", {"modules", "module-catalog", "catalog", "source-intel"}},
        {"quicktest", {"quicktest", "qtest", "smoke", "smoketest", "demo"}},
        {"history", {"history", "targets", "recent", "scans", "scanned"}},
        {"config", {"config", "settings", "options"}},
        {"wizard", {"wizard", "guide", "guided", "assist"}},
        {"about", {"about", "info", "details"}},
        {"explain", {"explain", "understand", "describe"}},
        {"banner", {"banner"}},
        {"help", {"help", "-h", "--help"}},
        {"exit", {"exit", "quit"}},
    };
    return keywords;
}

} // namespace silicore::interface
