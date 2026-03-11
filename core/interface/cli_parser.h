#pragma once

#include <string>
#include <vector>

namespace silicore::interface {

struct CliArgs {
    std::string command;
    std::vector<std::string> targets;
    std::string preset = "balanced";
    std::vector<std::string> plugins;
    bool all_plugins = false;
    bool html_output = false;
    bool json_output = true;
    bool tor_enabled = false;
    std::string proxy_url;
    int timeout_ms = 0;
    int concurrency = 0;
    bool prompt_mode = false;
};

CliArgs parse_args(int argc, char* argv[]);
CliArgs parse_line(const std::string& line);

} // namespace silicore::interface
