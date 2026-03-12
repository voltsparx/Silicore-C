#pragma once

#include "filters/filter_api.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace silicore::extensions {

struct FilterResult {
    std::string id;
    std::string title;
    std::string version;
    int severity = 0;
    std::string output_json;
};

class FilterHandle {
public:
    explicit FilterHandle(const std::filesystem::path& so_path);
    ~FilterHandle();

    const FilterSpec& spec() const { return spec_; }
    FilterResult run(const std::string& json_context) const;

private:
    void* handle_ = nullptr;
    FilterSpec spec_{};
    FilterOutput (*fn_run_)(const FilterContext*) = nullptr;
    void (*fn_free_)(FilterOutput*) = nullptr;
};

class FilterManager {
public:
    void load_all(const std::filesystem::path& filter_dir);
    std::vector<FilterResult> run_scope(const std::string& scope, const std::string& json_context) const;
    const std::vector<std::unique_ptr<FilterHandle>>& filters() const { return filters_; }

private:
    std::vector<std::unique_ptr<FilterHandle>> filters_;
};

} // namespace silicore::extensions
