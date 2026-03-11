#include "core/extensions/plugin_loader.h"

#include "core/utils/strings.h"

#include <dlfcn.h>
#include <filesystem>
#include <stdexcept>

namespace silicore::extensions {

PluginHandle::PluginHandle(const std::filesystem::path& so_path) {
    handle_ = dlopen(so_path.c_str(), RTLD_NOW);
    if (!handle_) {
        throw std::runtime_error("dlopen failed: " + std::string(dlerror()));
    }

    auto spec_fn = reinterpret_cast<const PluginSpec* (*)()>(dlsym(handle_, "silicore_plugin_spec"));
    fn_run_ = reinterpret_cast<PluginOutput (*)(const PluginContext*)>(dlsym(handle_, "silicore_plugin_run"));
    fn_free_ = reinterpret_cast<void (*)(PluginOutput*)>(dlsym(handle_, "silicore_plugin_free_output"));

    if (!spec_fn || !fn_run_ || !fn_free_) {
        dlclose(handle_);
        handle_ = nullptr;
        throw std::runtime_error("plugin missing required exports");
    }

    const PluginSpec* spec_ptr = spec_fn();
    if (!spec_ptr || !spec_ptr->id) {
        dlclose(handle_);
        handle_ = nullptr;
        throw std::runtime_error("plugin spec invalid");
    }
    spec_ = *spec_ptr;
}

PluginHandle::~PluginHandle() {
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
    }
}

PluginResult PluginHandle::run(const std::string& json_context) const {
    PluginContext ctx{json_context.c_str(), static_cast<int>(json_context.size())};
    PluginOutput out = fn_run_(&ctx);

    PluginResult result;
    result.id = spec_.id ? spec_.id : "";
    result.title = spec_.title ? spec_.title : "";
    result.version = spec_.version ? spec_.version : "";
    result.severity = out.severity;
    if (out.json_output && out.json_length > 0) {
        result.output_json.assign(out.json_output, out.json_length);
    }
    fn_free_(&out);
    return result;
}

void PluginManager::load_all(const std::filesystem::path& plugin_dir) {
    plugins_.clear();
    if (!std::filesystem::is_directory(plugin_dir)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(plugin_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".so") {
            continue;
        }
        try {
            plugins_.push_back(std::make_unique<PluginHandle>(entry.path()));
        } catch (const std::exception&) {
            continue;
        }
    }
}

std::vector<PluginResult> PluginManager::run_scope(const std::string& scope, const std::string& json_context) const {
    std::vector<PluginResult> results;
    auto scope_lower = utils::to_lower(scope);
    for (const auto& plugin : plugins_) {
        std::string scopes = plugin->spec().scopes ? plugin->spec().scopes : "";
        auto parts = utils::split(scopes, ',');
        bool allowed = false;
        for (auto& part : parts) {
            if (utils::to_lower(utils::trim(part)) == scope_lower) {
                allowed = true;
                break;
            }
        }
        if (!allowed) {
            continue;
        }
        results.push_back(plugin->run(json_context));
    }
    return results;
}

} // namespace silicore::extensions
