#pragma once

#include <string>

namespace silicore::collect {

bool is_tor_running(int timeout_ms = 600);
bool ensure_tor_running(bool allow_install);
std::string tor_proxy_url();

} // namespace silicore::collect
