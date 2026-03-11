#include "plugins/plugin_util.h"

#include <string>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "crypto_aes_attachment",
    "AES Attachment Encoder",
    "1.0",
    "profile,surface,fusion"
};

static std::string pseudo_aes_hex(const std::string& input, const std::string& key) {
    static const char* hex = "0123456789abcdef";
    if (key.empty()) {
        return "";
    }
    std::string out;
    out.reserve(input.size() * 2);
    for (size_t i = 0; i < input.size(); ++i) {
        unsigned char v = static_cast<unsigned char>(input[i]) ^ static_cast<unsigned char>(key[i % key.size()]);
        out.push_back(hex[(v >> 4) & 0xF]);
        out.push_back(hex[v & 0xF]);
    }
    return out;
}

extern "C" {

const PluginSpec* silicore_plugin_spec(void) {
    return &SPEC;
}

PluginOutput silicore_plugin_run(const PluginContext* ctx) {
    auto payload = silicore::plugins::parse_context(ctx);
    std::string target = silicore::plugins::target_value(payload);
    if (target.empty()) {
        target = "silicore-c";
    }
    const std::string key = "Silicore-C";

    json out;
    out["input"] = target;
    out["key_hint"] = "Silicore-C";
    out["cipher_hex"] = pseudo_aes_hex(target, key);
    out["notes"] = "Pseudo-AES transform (lightweight placeholder).";
    return silicore::plugins::make_output(out, 2);
}

void silicore_plugin_free_output(PluginOutput* out) {
    silicore::plugins::free_output(out);
}

} // extern "C"
