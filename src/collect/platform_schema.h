#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace silicore::collect {

struct PlatformConfig {
    std::string name;
    std::string url;
    std::string url_probe;
    std::vector<std::string> detection_methods;
    std::vector<int> exists_statuses;
    std::vector<int> not_found_statuses;
    std::vector<std::string> error_messages;
    std::string error_url;
    std::string regex_check;
    std::unordered_map<std::string, std::string> headers;
    std::string request_method;
    double confidence_weight = 0.7;
};

class PlatformValidationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

std::vector<PlatformConfig> load_platforms(const std::string& platform_dir = "platforms");

} // namespace silicore::collect
