#include "plugins/plugin_util.h"

#include <string>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "crypto_xor",
    "XOR Encoder",
    "1.0",
    "profile,surface,fusion"
};

static std::string xor_hex(const std::string& input, unsigned char key) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char ch : input) {
        unsigned char v = ch ^ key;
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

    json out;
    out["input"] = target;
    out["key"] = "0x5A";
    out["cipher_hex"] = xor_hex(target, 0x5A);
    out["notes"] = "XOR encoding for lightweight transforms.";
    return silicore::plugins::make_output(out, 1);
}

void silicore_plugin_free_output(PluginOutput* out) {
    silicore::plugins::free_output(out);
}

} // extern "C"
