#pragma once

#include <string>

namespace silicore::interface {

std::string read_line(const std::string& prompt);
void add_history(const std::string& line);
bool last_read_eof();

} // namespace silicore::interface
