#pragma once

#include "interface/cli_parser.h"

#include <functional>

namespace silicore::interface {

using CommandHandler = std::function<int(const CliArgs&)>;

int run_prompt(const CommandHandler& handler);

} // namespace silicore::interface

