#include "plugins/plugin_util.h"

#include <string>

using json = silicore::plugins::json;

static const PluginSpec SPEC = {
    "crypto_rot13",
    "ROT13 Encoder",
    "1.0",
    "profile,surface,fusion"
};

static std::string rot13(std::string input) {
    for (auto& ch : input) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>('a' + (ch - 'a' + 13) % 26);
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>('A' + (ch - 'A' + 13) % 26);
        }
    }
    return input;
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
    out["rot13"] = rot13(target);
    out["notes"] = "ROT13 transform for lightweight obfuscation.";
    return silicore::plugins::make_output(out, 1);
}

void silicore_plugin_free_output(PluginOutput* out) {
    silicore::plugins::free_output(out);
}

} // extern "C"
