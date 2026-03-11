#include "extensions/filter_loader.h"

#include "utils/strings.h"

#include <dlfcn.h>
#include <filesystem>
#include <stdexcept>

namespace silicore::extensions {

FilterHandle::FilterHandle(const std::filesystem::path& so_path) {
    handle_ = dlopen(so_path.c_str(), RTLD_NOW);
    if (!handle_) {
        throw std::runtime_error("dlopen failed: " + std::string(dlerror()));
    }

    auto spec_fn = reinterpret_cast<const FilterSpec* (*)()>(dlsym(handle_, "silicore_filter_spec"));
    fn_run_ = reinterpret_cast<FilterOutput (*)(const FilterContext*)>(dlsym(handle_, "silicore_filter_run"));
    fn_free_ = reinterpret_cast<void (*)(FilterOutput*)>(dlsym(handle_, "silicore_filter_free_output"));

    if (!spec_fn || !fn_run_ || !fn_free_) {
        dlclose(handle_);
        handle_ = nullptr;
        throw std::runtime_error("filter missing required exports");
    }

    const FilterSpec* spec_ptr = spec_fn();
    if (!spec_ptr || !spec_ptr->id) {
        dlclose(handle_);
        handle_ = nullptr;
        throw std::runtime_error("filter spec invalid");
    }
    spec_ = *spec_ptr;
}

FilterHandle::~FilterHandle() {
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
    }
}

FilterResult FilterHandle::run(const std::string& json_context) const {
    FilterContext ctx{json_context.c_str(), static_cast<int>(json_context.size())};
    FilterOutput out = fn_run_(&ctx);

    FilterResult result;
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

void FilterManager::load_all(const std::filesystem::path& filter_dir) {
    filters_.clear();
    if (!std::filesystem::is_directory(filter_dir)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(filter_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".so") {
            continue;
        }
        try {
            filters_.push_back(std::make_unique<FilterHandle>(entry.path()));
        } catch (const std::exception&) {
            continue;
        }
    }
}

std::vector<FilterResult> FilterManager::run_scope(const std::string& scope, const std::string& json_context) const {
    std::vector<FilterResult> results;
    auto scope_lower = utils::to_lower(scope);
    for (const auto& filter : filters_) {
        std::string scopes = filter->spec().scopes ? filter->spec().scopes : "";
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
        results.push_back(filter->run(json_context));
    }
    return results;
}

} // namespace silicore::extensions
