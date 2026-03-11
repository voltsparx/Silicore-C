#include "modules/catalog.h"

namespace silicore::modules {

size_t module_count() {
    return all_modules().size();
}

} // namespace silicore::modules

