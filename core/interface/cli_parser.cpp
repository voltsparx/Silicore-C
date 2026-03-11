#include "core/interface/cli_parser.h"

#include "core/utils/strings.h"

#include <sstream>

namespace silicore::interface {

namespace {

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        out.push_back(token);
    }
    return out;
}

CliArgs parse_tokens(const std::vector<std::string>& tokens) {
    CliArgs args;
    if (tokens.empty()) {
        args.prompt_mode = true;
        return args;
    }

    size_t i = 0;
    if (!tokens[i].empty() && tokens[i][0] != '-') {
        args.command = tokens[i++];
    }

    for (; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token == "--preset" && i + 1 < tokens.size()) {
            args.preset = tokens[++i];
        } else if (token == "--timeout" && i + 1 < tokens.size()) {
            args.timeout_ms = std::stoi(tokens[++i]);
        } else if (token == "--concurrency" && i + 1 < tokens.size()) {
            args.concurrency = std::stoi(tokens[++i]);
        } else if (token == "--proxy" && i + 1 < tokens.size()) {
            args.proxy_url = tokens[++i];
        } else if (token == "--tor") {
            args.tor_enabled = true;
        } else if (token == "--html") {
            args.html_output = true;
        } else if (token == "--json") {
            args.json_output = true;
        } else if (token == "--plugins" && i + 1 < tokens.size()) {
            auto list = utils::split(tokens[++i], ',');
            for (auto& item : list) {
                auto trimmed = utils::trim(item);
                if (!trimmed.empty()) {
                    args.plugins.push_back(trimmed);
                }
            }
        } else if (token == "--all-plugins") {
            args.all_plugins = true;
        } else if (!token.empty() && token[0] != '-') {
            args.targets.push_back(token);
        }
    }

    return args;
}

} // namespace

CliArgs parse_args(int argc, char* argv[]) {
    std::vector<std::string> tokens;
    for (int i = 1; i < argc; ++i) {
        tokens.emplace_back(argv[i]);
    }
    if (tokens.empty()) {
        CliArgs args;
        args.prompt_mode = true;
        return args;
    }
    return parse_tokens(tokens);
}

CliArgs parse_line(const std::string& line) {
    return parse_tokens(tokenize(line));
}

} // namespace silicore::interface
