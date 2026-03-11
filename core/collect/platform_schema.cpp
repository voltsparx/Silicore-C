#include "core/collect/platform_schema.h"
#include "platforms/platforms.h"

namespace silicore::collect {

std::vector<PlatformConfig> load_platforms(const std::string&) {
    const auto& embedded = embedded_platforms();
    return std::vector<PlatformConfig>(embedded.begin(), embedded.end());
}

} // namespace silicore::collect
