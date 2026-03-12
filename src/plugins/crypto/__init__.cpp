#include <string>
#include <vector>

namespace silicore::plugins::crypto {

std::vector<std::string> crypto_plugin_ids() {
    return {"aes_plugin", "rot13_plugin", "xor_plugin"};
}

std::string crypto_init_summary() {
    return "Silicore-C crypto plugin set ready.";
}

} // namespace silicore::plugins::crypto
