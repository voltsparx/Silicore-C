#pragma once

#include <string>
#include <vector>

namespace silicore::interface {

struct CliArgs {
    std::string command;
    std::vector<std::string> targets;
    std::string preset = "balanced";
    std::string profile_preset;
    std::string surface_preset;
    std::string extension_control;
    std::vector<std::string> plugins;
    bool all_plugins = false;
    bool list_plugins = false;
    std::vector<std::string> filters;
    bool all_filters = false;
    bool list_filters = false;
    bool list_modules = false;
    std::string secondary_target;
    std::string source_profile;
    int max_platforms = 0;
    double min_confidence = 0.25;
    bool html_output = false;
    bool json_output = false;
    bool text_output = false;
    bool csv_output = false;
    std::string output_dir;
    bool tor_enabled = false;
    bool no_tor = false;
    std::string proxy_url;
    bool no_proxy = false;
    bool check_only = false;
    bool prompt_only = false;
    int timeout_ms = 0;
    int concurrency = 0;
    int max_subdomains = 0;
    bool include_ct = true;
    bool include_rdap = true;
    bool live = false;
    int live_port = 0;
    bool no_browser = false;
    bool stats_only = false;
    std::string scope = "all";
    std::string kind = "all";
    std::vector<std::string> framework;
    std::vector<std::string> tag;
    int min_score = 0;
    std::string sort_by = "framework";
    bool descending = false;
    int offset = 0;
    bool modules_sync = false;
    bool modules_validate = false;
    bool modules_json = false;
    std::string search;
    int limit = 0;
    bool profile_phase = true;
    bool surface_phase = true;
    bool fusion_phase = true;
    std::vector<std::string> usernames;
    std::string domain;
    bool sync_modules = false;
    bool list_templates = false;
    std::string template_id;
    int seed = 0;
    bool seed_provided = false;
    bool prompt_mode = false;
};

CliArgs parse_args(int argc, char* argv[]);
CliArgs parse_line(const std::string& line);

} // namespace silicore::interface
