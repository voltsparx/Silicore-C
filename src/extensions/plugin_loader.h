#pragma once

#include "plugins/plugin_api.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace silicore::extensions {

struct PluginResult {
    std::string id;
    std::string title;
    std::string version;
    int severity = 0;
    std::string output_json;
};

class PluginHandle {
public:
    explicit PluginHandle(const std::filesystem::path& so_path);
    ~PluginHandle();

    const PluginSpec& spec() const { return spec_; }
    PluginResult run(const std::string& json_context) const;

private:
    void* handle_ = nullptr;
    PluginSpec spec_{};
    PluginOutput (*fn_run_)(const PluginContext*) = nullptr;
    void (*fn_free_)(PluginOutput*) = nullptr;
};

class PluginManager {
public:
    void load_all(const std::filesystem::path& plugin_dir);
    std::vector<PluginResult> run_scope(const std::string& scope, const std::string& json_context) const;
    const std::vector<std::unique_ptr<PluginHandle>>& plugins() const { return plugins_; }

private:
    std::vector<std::unique_ptr<PluginHandle>> plugins_;
};

} // namespace silicore::extensions
